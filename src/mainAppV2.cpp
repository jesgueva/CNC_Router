/*
   By Jesus Esgueva
   Contact: jesgueva@gmail.com

   CNC Router table.
   Using Driver TB6600
   Technologies:
*/

#include <AccelStepper.h>

#define motorInterfaceType 1
//X Axis
//Motor
#define xDirPin 3
#define xStepPin 4
//Switch
#define xHallSensor 7

//Y Axis
//Motor
#define yDirPin 5
#define yStepPin 6
//Switch
#define yHallSensor 2

// Switches
#define turnOn 8

//General Specs
#define motorMax 1000
#define accel 1000
//1/8 microstepping
#define rev 200 * 8
#define debug false

//declarations
void printDebug();
void runHome();
void mySetup();
struct Sensor
{
    int previous = 1;
    int output = 1;
    int count = 1;
    int threshold = 50;

    int inputValidator(int current)
    {
        if (this->previous == current)
        {
            this->count++;
        }
        else
        {
            this->count = 1;
        }
        if (this->count == this->threshold)
        {
            this->output = current;
        }
        this->previous = current;
        return this->output;
    }

    void setOutput(int value){
        this->previous = value;
        this->output = value;
    }
};


// Define a stepper and the pins it will use
AccelStepper xMotor = AccelStepper(motorInterfaceType, xStepPin, xDirPin);
AccelStepper yMotor = AccelStepper(motorInterfaceType, yStepPin, yDirPin);
struct Sensor x_axis;
struct Sensor y_axis;
struct Sensor turnOn_input;


bool runProgram, yStatus, xDir, runY, runX, stopTriggered;
long xStep, yStep, yStepTarget, xStepTarget;

void setup()
{
    mySetup();
}
void loop()
{
    if (debug)
    {
        printDebug();
    }
    runProgram = turnOn_input.inputValidator(!digitalRead(turnOn));
    if (runProgram)
    {

        bool x_sensor = x_axis.inputValidator(digitalRead(xHallSensor));
        bool y_sensor = y_axis.inputValidator(digitalRead(yHallSensor));

        if (xMotor.distanceToGo() == 0 && xDir && !yStatus) // Top reached
        {
            xStep = xMotor.currentPosition();
            if (debug)
            {
                Serial.println("X finished---TOP");
            }
            // if (runX) {
            //     Serial.println("Slow Down baby");
            //     xMotor.setSpeed(0);
            //     xMotor.stop();}
            runY = true;
            runX = false;
            xDir = false;
        }

        if (x_sensor == 0 && !stopTriggered){
            if (debug)
            {
                Serial.println("Sensor X");
            }
            xMotor.stop();
            stopTriggered = true;
        }

        // X go up
        if (runX)
        {
            if (debug)
            {
                Serial.println("Run X");
            }
            xMotor.run();
            yMotor.stop();
        }

        if (runY)
        {
            if (debug)
            {
                Serial.println("End X, Run Y");
            }
            // xMotor.moveTo(xMotor.currentPosition());
            yMotor.moveTo(yStepTarget); // TODO: move to top reached and bottom;
            yMotor.run();
            
        }

        // Y reaches position
        if (yMotor.distanceToGo() == 0 && runY && !yStatus)
        {
            runY = false;
            yStatus = true;
            if (debug)
            {
                Serial.println("Stop Y, Run X");
            }
            yMotor.stop();
            yStepTarget = yMotor.currentPosition() + yStep; // update y target
            if (x_sensor == 0  || xMotor.currentPosition() > 1)
            { //if on top
                if (debug)
                {
                    Serial.println("Start X--Down");
                }
                xStepTarget = 1;
                xMotor.moveTo(xStepTarget);
                runX = true;
            }
            else
            {
                if (debug)
                {
                    Serial.println("Start X--UP");
                }
                xMotor.moveTo(xStep);
                yStatus = false;
                runX = true;
                xDir = true;
                stopTriggered = false;
            }
        }

        // Bottom bracket
        if (xMotor.currentPosition() <= 1 && yStatus)
        {
            if (debug)
            {
                Serial.println("Bottom X");
            }

            if (xDir)
            { // up
                if (debug)
                {
                    Serial.println("Start X--UP");
                }
                xMotor.move(xStep);
                yStatus = false;
                
            }
            else
            { //down
                if (debug)
                {
                    Serial.println("Run Y Bottom");
                }
                runY = true;
                yStatus = false;
                runX = false;
            }
        }

        // Reached end of table
        if (y_sensor == 0)
        {
            if (debug)
            {
                Serial.println("End Y");
            }

            y_axis.setOutput(1);
            runHome();
        }
    }
    if (debug)
    {
        delay(500);
    }
}

void runHome()
{
    xMotor.moveTo(1);
    yMotor.moveTo(1);
    while (runProgram)
    {
        runProgram = turnOn_input.inputValidator(!digitalRead(turnOn));
        if(debug){Serial.println("Go Home");}
        

        if (xMotor.distanceToGo() == 0)
        {
            xMotor.stop();
        }
        else
        {
            xMotor.run();
        }
        if (yMotor.distanceToGo() == 0)
        {
            yMotor.stop();
        }
        else
        {
            yMotor.run();
        }

        if (debug)
    {
        delay(500);
    }
    }
    mySetup();
}

void printDebug()
{
    Serial.print("X-sensor: ");
    Serial.print(digitalRead(xHallSensor));
    Serial.print(", Y-sensor: ");
    Serial.print(digitalRead(yHallSensor));
    Serial.print(" X-val: ");
    Serial.print(x_axis.output);
    Serial.print(", Y-val: ");
    Serial.print(y_axis.output);
    Serial.print(", X: ");
    Serial.print(xMotor.currentPosition());
    Serial.print(", Y: ");
    Serial.print(yMotor.currentPosition());
    Serial.print(", xT: ");
    Serial.print(xMotor.targetPosition());
    Serial.print(", yT: ");
    Serial.print(yMotor.targetPosition());
    Serial.print(", Dir: ");
    Serial.print(xDir);
    Serial.print(", yStatus: ");
    Serial.println(yStatus);
}

void mySetup(){
    if (debug)
    {
        Serial.begin(250000);
    }

    xMotor.setMaxSpeed(motorMax);
    xMotor.setAcceleration(accel);

    yMotor.setMaxSpeed(motorMax);
    yMotor.setAcceleration(accel);

    pinMode(xHallSensor, INPUT);
    pinMode(yHallSensor, INPUT);
    pinMode(turnOn, INPUT_PULLUP);

    // Initialize variables
    xStep = 19500;
    yStep = rev * 0.5; //steps needed
    yStepTarget = yStep;
    turnOn_input.setOutput(0);
    xStepTarget = 0;
    yStatus = true;
    xDir = true;
    runY = false;
    runX = true;
    stopTriggered = false;
}
