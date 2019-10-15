#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

String Version("Sep 8 2019");

// #define HOME 1

#ifdef HOME
const char *ssid = "goofmeisters";
const char *password = "mineshaftgap";
// Set your Static IP address
IPAddress local_IP(192, 168, 1, 6);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
#else
const char *ssid = "MySpectrumWiFief-2G";
const char *password = "statuedegree702";
#endif

// Create the Web Server listening on port 80 (http)
WebServer server(80);

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
bool homing[2] = {true, true};
bool backingOff[2] = {false, false};

// PWM pins and values for opening / closing
// int pulsePin[2] = {9, 10};
int pulsePin[2] = {32, 33};
int doorOpening[2] = {HIGH, LOW};
int doorClosing[2] = {LOW, HIGH};
// Motor direction pins
int dirPin[2] = {25, 26};
// Limit switch read pins
int limitOpenPin[2] = {34, 35};
// joystick pins
// int upDownPin = 14;
// int leftRightPin = 27;
int upDownPin = 39;
int leftRightPin = 36;
int lowerLimit = 100;
int upperLimit = 3692;
bool leftRightMotion = false;

// Steps from opened to closed
int stepLimit[2] = {18900, 18900};
int steps[2] = {0, 0};

// rate of travel (inverse)
int rates[] = {8, 6, 4, 4, 2, 2, 1, 1, 1, 1};
int speed = 40;

String displayStatus();
String openDoors();
String closeDoors();

String positionToString(int position)
{
  switch (position)
  {
  case OPENED:
    return String("OPENED");
  case CLOSED:
    return String("CLOSED");
  case AJAR:
    return String("AJAR");
  }
  return String("???");
}

String displayStatus()
{
  String status = String("S/W Version: ") + Version + "\n";

  for (int i = 0; i < 2; i++)
  {
    status += doorName[i];
    status += String(" is ") + String(state[i] == MOVING ? "MOVING" : "STOPPED");
    status += String(", ") + String(positionToString(position[i]));
    status += String(", ") + String(opening[i] ? "OPENING" : "CLOSING");
    status += String(", Steps: ") + String(steps[i]);
    status += homing[i] ? ", Homing" : "";
    status += "\n";
  }

  Serial.println(status);
  return status;
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
    // Serial.println(doorName[door] + " setDirection, opening");
    digitalWrite(dirPin[door], doorOpening[door]);
  }
  else
  {
    // Serial.println(doorName[door] + "setDirection, closing");
    digitalWrite(dirPin[door], doorClosing[door]);
  }
}

bool isLimitSwitchOpen(int door)
{
  return digitalRead(limitOpenPin[door]) == LOW;
}

void homeDoor(int door)
{
  state[door] = MOVING;
  homing[door] = true;

  if (isLimitSwitchOpen(door))
  {
    // IF already on the limit switch then just backoff
    opening[door] = false;
    backingOff[door] = true;
    Serial.println("Backing off");
  }
  else
  {
    opening[door] = true;
    backingOff[door] = false;
    Serial.println("Start homing");
  }
  setDirection(door);
}

int getSpeed(int door)
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

  if (opening[door] && (steps[door] < 500 || steps[door] > stepLimit[door] - 500))
  {
    return speed * 8;
  }
  else if (!opening[door] && (steps[door] < 1000 || steps[door] > stepLimit[door] - 1000))
  {
    return speed * 4;
  }
  return speed;
}

void moveDoor(int door)
{
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
      backingOff[door] = isLimitSwitchOpen(door);
      if (!backingOff[door])
      {
        Serial.println("Backing off complete");
        stopDoor(door);
        position[door] = OPENED;
        // reset position after backing off
        steps[door] = 0;
        homing[door] = false;
      }
    }
    else if (opening[door])
    {
      // Let limit switch take priority
      if (isLimitSwitchOpen(door))
      {
        opening[door] = false;
        setDirection(door);
        Serial.println("Backing off");
        backingOff[door] = true;
      }
      else if (!homing[door])
      {
        if (steps[door] <= 0)
        {
          opening[door] = false;
          setDirection(door);
          stopDoor(door);
          position[door] = OPENED;
        }
      }
    }
    else if (steps[door] > stepLimit[door])
    {
      opening[door] = true;
      setDirection(door);
      stopDoor(door);
      position[door] = CLOSED;
    }
  }
}

void handleRoot()
{
  server.send(200, "text/plain", String("ESP32: ") + String(Version));
}

void handleStop()
{
  state[0] = state[1] = STOPPED;
  backingOff[0] = backingOff[1] = false;
  server.send(200, "text/plain", displayStatus());
}

void handleStatus()
{
  server.send(200, "text/plain", displayStatus());
}

void handleOpen()
{
  if (position[0] != OPENED)
  {
    opening[0] = opening[1] = true;
    state[0] = state[1] = MOVING;
  }
  server.send(200, "text/plain", displayStatus());
}

void handleClose()
{
  if (position[0] != CLOSED)
  {
    opening[0] = opening[1] = false;
    state[0] = state[1] = MOVING;
  }
  server.send(200, "text/plain", displayStatus());
}

void handleHome()
{
  homeDoor(0);
  homeDoor(1);

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

#ifdef HOME
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }
#endif

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
  server.on("/home", handleHome);

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

  setDirection(0);
  setDirection(1);

  homeDoor(0);
  homeDoor(1);
}

void handleJoystick()
{
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
}

void loop()
{
  // read sensor
  // transition between moving and stopped
  // handleJoystick();
  moveDoor(0);
  moveDoor(1);
  server.handleClient();
}
