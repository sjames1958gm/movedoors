#include <Arduino.h>

// Door names
String doorName[2] = {"right", "left"};

// Door state, stopped, opening, backing off
const int STOPPED = 0;
const int MOVING = 1;

const int OPENED = 0;
const int CLOSED = 1;
const int AJAR = 2;

int position[2] = {AJAR, AJAR};
int state[2] = {STOPPED, STOPPED};
bool opening[2] = {true, true};
bool backingOff[2] = {false, false};

// PWM pins and values for opening / closing
int pulsePin[2] = {9, 10};
int doorOpening[2] = {HIGH, LOW};
int doorClosing[2] = {LOW, HIGH};
// Motor direction pins
int dirPin[2] = {2, 3};
// Limit switch read pins
int limitOpenPin[2] = {4, 5};
// joystick pins
int upDownPin = 0;
int leftRightPin = 1;
int lowerLimit = 100;
int upperLimit = 923;
bool leftRightMotion = false;

// Steps from opened to closed
int stepLimit[2] = {18977, 19327};
int steps[2] = {0, 0};

// rate of travel (inverse)
int speed = 200;

// Testing values
int selectedDoor = 0;
bool testMode = false;
bool dualMode = false;

void displayStatus()
{
  Serial.println(String("Selected door is: ") + doorName[selectedDoor]);
  for (int i = 0; i < 2; i++)
  {
    Serial.print(doorName[i] + String(" is "));
    Serial.print(String(state[i] == MOVING ? "MOVING" : "STOPPED"));
    Serial.println(String(", ") + String(opening[i] ? "OPENING" : "CLOSING"));
  }
}

void stopDoor(int door)
{
  Serial.println(doorName[door] + String("Position: ") + String(steps[door]));
  state[door] = STOPPED;
}

void setDirection(int door)
{
  if (opening[door])
  {
    Serial.println(doorName[door] + " setDirection, opening");
    digitalWrite(dirPin[door], doorOpening[door]);
  }
  else
  {
    Serial.println(doorName[door] + "setDirection, closing");
    digitalWrite(dirPin[door], doorClosing[door]);
  }
}

bool isLimitSwitchOpen(int door)
{
  return digitalRead(limitOpenPin[door]) == LOW;
}

void moveDoor(int door)
{
  if (state[door] == MOVING)
  {
    digitalWrite(pulsePin[door], HIGH);
    delayMicroseconds(speed);
    digitalWrite(pulsePin[door], LOW);
    delayMicroseconds(speed);

    steps[door] += opening[door] ? -1 : 1;
    if (backingOff[door])
    {
      backingOff[door] = isLimitSwitchOpen(door);
      if (!backingOff[door])
      {
        Serial.println("Backing off complete");
        stopDoor(door);
        position[door] = OPENED;
        // reset position after backing off
        steps[door] = 0;
      }
    }
    else if (opening[door])
    {
      if (isLimitSwitchOpen(door))
      {
        opening[door] = false;
        setDirection(door);
        Serial.println("Backing off");
        backingOff[door] = true;
      }
    }
    else
    {
      if (steps[door] > stepLimit[door])
      {
        opening[door] = true;
        stopDoor(door);
        position[door] = CLOSED;
      }
    }
  }
}

// Simple menu for various commands
void getJobMenu()
{
  Serial.println("GETJOB():  enter the item number to run");
  Serial.println("0. display menu");
  Serial.println("1. display status");
  Serial.println("2. move door");
  Serial.println("3. stop door");
  Serial.println("4. change direction");
  Serial.println("5. read limit switch");
  Serial.println("6. select door");
  Serial.println("7. home");
  Serial.println("8. mode switch");
}

void doJobs()
{
  int jobNumber;
  getJobMenu();
  bool done = false;
  while (!done)
  {
    while (!Serial.available())
    {
      if (dualMode)
      {
        moveDoor(0);
        moveDoor(1);
      }
      else
      {
        moveDoor(selectedDoor);
      }
    }
    delay(10);
    while (Serial.available())
    {
      jobNumber = Serial.parseInt();
      if (Serial.read() != '\n')
      {
        // Serial.println("going to " + String(jobNumber));
      }
    }

    switch (jobNumber)
    {
    case 0:
      getJobMenu();
      break;
    case 1:
      displayStatus();
      break;
    case 2:
      if (dualMode)
      {
        Serial.println(String("Moving both doors"));
        state[0] = MOVING;
        state[1] = MOVING;
      }
      else
      {
        Serial.println(String("Moving door: ") + String(selectedDoor));
        state[selectedDoor] = MOVING;
      }
      break;
    case 3:
      if (dualMode)
      {
        stopDoor(0);
        stopDoor(1);
      }
      else
      {
        stopDoor(selectedDoor);
      }
      break;
    case 4:
      if (dualMode)
      {
        opening[0] = !opening[0];
        setDirection(0);
        opening[1] = !opening[1];
        setDirection(1);
      }
      else
      {
        opening[selectedDoor] = !opening[selectedDoor];
        setDirection(selectedDoor);
        Serial.print(String("Door ") + doorName[selectedDoor] + String(" is "));
        Serial.println(opening[selectedDoor] ? "opening" : "closing");
      }
      break;
    case 5:
      Serial.print(String("Door ") + doorName[selectedDoor] + String(" limit switch is "));
      Serial.println(isLimitSwitchOpen(selectedDoor) ? "on" : "off");
      break;
    case 6:
      selectedDoor = (selectedDoor + 1) % 2;
      Serial.print(String("Selected door: "));
      Serial.println(selectedDoor);
      break;
    case 7:
      Serial.println("Homing - opening to limit switch");
      opening[selectedDoor] = true;
      state[selectedDoor] = MOVING;
      setDirection(selectedDoor);
      break;
    case 8:
      dualMode = !dualMode;
      Serial.println(dualMode ? "Dual Mode" : "Single Mode");
      break;
    } // end of switch
  }
  Serial.println("DONE in getJob().");
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  pinMode(limitOpenPin[0], INPUT);
  pinMode(limitOpenPin[1], INPUT);
  pinMode(dirPin[0], OUTPUT);
  pinMode(dirPin[1], OUTPUT);
  pinMode(pulsePin[0], OUTPUT);
  pinMode(pulsePin[1], OUTPUT);
  digitalWrite(pulsePin[0], LOW);
  digitalWrite(pulsePin[1], HIGH);

  setDirection(0);
  setDirection(1);

  // Wait about 3 seconds for input to go into test mode
  int count = 6;
  while (!Serial.available() && (count-- > 0))
  {
    delay(500);
  }

  if (Serial.available())
  {
    testMode = true;
  }
  else
  {
    // Live mode start by opening the doors
    state[0] = state[1] = MOVING;
    opening[0] = opening[1] = true;
  }
}

void handleJoystick()
{
  const int upDown = analogRead(upDownPin);
  const int leftRight = analogRead(leftRightPin);
  if (upDown > upperLimit)
  {
    if (position[0] != OPENED)
    {
      opening[0] = opening[1] = true;
      state[0] = state[1] = MOVING;
      leftRightMotion = false;
    }
  }
  else if (upDown < lowerLimit)
  {
    if (position[1] != CLOSED)
    {
      opening[0] = opening[1] = false;
      state[0] = state[1] = MOVING;
      leftRightMotion = false;
    }
  }
  else if (leftRight > upperLimit)
  {
    if (position[0] != OPENED)
    {
      opening[0] = opening[1] = true;
      state[0] = state[1] = MOVING;
      leftRightMotion = true;
    }
  }
  else if (leftRight < lowerLimit)
  {
    if (position[0] != CLOSED)
    {
      opening[0] = opening[1] = true;
      state[0] = state[1] = MOVING;
      leftRightMotion = true;
    }
  }
  else if (leftRightMotion)
  {
    state[0] = state[1] = STOPPED;
    leftRightMotion = false;
  }
}

void doLive()
{
  // read sensor
  // transition between moving and stopped
  handleJoystick();
  moveDoor(0);
  moveDoor(1);
}

void loop()
{
  if (testMode)
  {
    doJobs();
  }
  else
  {
    doLive();
  }
}