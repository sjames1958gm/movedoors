#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

String Version("Sep 1 2019");

const char *ssid = "MySpectrumWiFief-2G";
const char *password = "statuedegree702";

// Create the Web Server listening on port 80 (http)
WebServer server(80);

enum STATE
{
  HOMING,
  OPENING,
  CLOSING,
  BACKINGOFF,
  OPENED,
  CLOSED,
  AJAR
};

typedef struct doorState_s
{
  String name;
  int stepLimit;
  STATE state;
  double speed;
  int acceleration;
  int position;
} doorState_t;

doorState_t doors[2] = {{"right", 18900, HOMING, 0, 0, 0}, {"left", 18900, HOMING, 0, 0, 0}};

// PWM pins and values for opening / closing
int pulsePin[2] = {32, 33};
int doorOpening[2] = {HIGH, LOW};
int doorClosing[2] = {LOW, HIGH};
// Motor direction pins
int dirPin[2] = {25, 26};
// Limit switch read pins
int limitOpenPin[2] = {34, 35};
// joystick pins
int upDownPin = 39;
int leftRightPin = 36;
int lowerLimit = 100;
int upperLimit = 3692;
bool leftRightMotion = false;

// rate of travel (inverse)
int rates[] = {8, 6, 4, 4, 2, 2, 1, 1, 1, 1};
int speed = 25;

String displayStatus();
String openDoors();
String closeDoors();

String stateToString(int door)
{
  switch (doors[door].state)
  {
  case HOMING:
    return String("HOMING");
  case OPENING:
    return String("OPENING");
  case CLOSING:
    return String("CLOSING");
  case BACKINGOFF:
    return String("BACKINGOFF");
  case OPENED:
    return String("OPENED");
  case CLOSED:
    return String("CLOSED");
  case AJAR:
    return String("AJAR");
  }
  return String("???");
}

void stopDoor(int i)
{
  doorState_t *door = &doors[i];
  Serial.println(door->name + String("Position: ") + String(door->position));
  // state[door] = STOPPED;
  //TODO: State transition
}

String displayStatus()
{
  String status = "";

  for (int i = 0; i < 2; i++)
  {
    doorState_t *door = &doors[i];

    status += door->name;
    status += String(" is ") + stateToString(door->state);
    status += String(", Position: ") + String(door->position);
    status += "\n";
  }

  Serial.println(status);
  return status;
}

void setDirection(int i)
{
  doorState_t *door = &doors[i];
  if ((door->state == OPENING) || (door->state == HOMING))
  {
    // Serial.println(doorName[door] + " setDirection, opening");
    digitalWrite(dirPin[i], doorOpening[i]);
  }
  else
  {
    // Serial.println(doorName[door] + "setDirection, closing");
    digitalWrite(dirPin[i], doorClosing[i]);
  }
}

bool isMoving(int i)
{

  doorState_t *door = &doors[i];
  if (door->state == HOMING || door->state == CLOSING || door->state == OPENING)
  {
    return true;
  }
  return false;
}

bool isLimitSwitchClosed(int door)
{
  return digitalRead(limitOpenPin[door]) == LOW;
}

int getSpeed(int i)
{
  // int entries = 10;
  // static int c = 0;
  // double slo2 = stepLimit[door] / 2.0;
  // double sd = steps[door];
  // int rateIndex = 0;
  // if (!opening[door])
  // {
  //   if (sd < slo2)
  //   {
  //     rateIndex = (sd / slo2) * entries;
  //   }
  //   else
  //   {
  //     rateIndex = (entries - 1) - (((sd - slo2) / slo2) * entries);
  //   }
  //   return rates[rateIndex] * speed;
  // }

  doorState_t *door = &doors[i];

  if (door->state == OPENING && (door->position < 500 || door->position > door->stepLimit - 500))
  {
    return speed * 8;
  }
  else if (door->state == CLOSING && (door->position < 1000 || door->position > door->stepLimit - 1000))
  {
    return speed * 4;
  }

  return door->state == HOMING ? speed : 0;
}

void moveDoor(int door)
{
  /*
  if (state[door] == MOVING)
  {
    position[door] = AJAR;
    digitalWrite(pulsePin[door], HIGH);
    delayMicroseconds(getSpeed(door));
    digitalWrite(pulsePin[door], LOW);
    delayMicroseconds(getSpeed(door));

    steps[door] += opening[door] ? -1 : 1;
    if (backingOff[door])
    {
      backingOff[door] = isLimitSwitchClosed(door);
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
      if (isLimitSwitchClosed(door))
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
  */
}

void handleRoot()
{
  server.send(200, "text/plain", String("ESP32: ") + String(Version));
}

void handleStop()
{
  stopDoor(0);

  server.send(200, "text/plain", displayStatus());
}

void handleStatus()
{
  server.send(200, "text/plain", displayStatus());
}

void handleOpen()
{
  //
  server.send(200, "text/plain", displayStatus());
}

void handleClose()
{
  server.send(200, "text/plain", displayStatus());
}

void handleNotFound()
{
  server.send(404, "application/json", "{\"message\": \"Not Found\"}");
}

void setup()
{
  Serial.begin(115200);
  delay(10);

  Serial.println(String("In setup: ") + Version);

  Serial.println();
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  WiFi.disconnect(true);
  delay(1000);
  WiFi.mode(WIFI_STA);
  delay(1000);
  Serial.print("WIFI status = ");
  Serial.println(WiFi.getMode());
  // End silly stuff !!!
  WiFi.begin(ssid, password);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    yield();
    Serial.print(".");
    count++;
    if (count > 10)
    {
      WiFi.disconnect(true);
      delay(1000);
      WiFi.mode(WIFI_STA);
      delay(1000);
      WiFi.begin(ssid, password);
      count = 0;
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");

  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this IP to connect: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/open", handleOpen);
  server.on("/close", handleClose);
  server.on("/stop", handleStop);
  server.on("/home", handleRoot);

  server.onNotFound(handleNotFound);

  pinMode(2, OUTPUT);
  pinMode(limitOpenPin[0], INPUT);
  pinMode(limitOpenPin[1], INPUT);
  pinMode(dirPin[0], OUTPUT);
  pinMode(dirPin[1], OUTPUT);
  pinMode(pulsePin[0], OUTPUT);
  pinMode(pulsePin[1], OUTPUT);
  digitalWrite(pulsePin[0], LOW);
  digitalWrite(pulsePin[1], HIGH);

  for (int door = 0; door < 2; door++)
  {
    if (isLimitSwitchClosed(door))
    {
      doors[door].state = BACKINGOFF;
    }
    else
    {
      doors[door].state = HOMING;
    }

    setDirection(door);
  }
}

void handleJoystick()
{
  /*
  static int counter = 0;
  const int upDown = analogRead(upDownPin);
  const int leftRight = analogRead(leftRightPin);

  if (counter++ % 3000 == 0)
  {
    // Serial.println(String("JS: ") + String(upDown) + String(" ") + String(leftRight));
    //     Serial.println(String("ST: ") + String(steps[0]) + String("/") + String(steps[1]));
  }
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
      opening[0] = opening[1] = false;
      state[0] = state[1] = MOVING;
      leftRightMotion = true;
    }
  }
  else if (leftRightMotion)
  {
    state[0] = state[1] = STOPPED;
    leftRightMotion = false;
  }

  setDirection(0);
  setDirection(1);
  */
}

void loop()
{
  handleJoystick();
  moveDoor(0);
  moveDoor(1);
  server.handleClient();
}
