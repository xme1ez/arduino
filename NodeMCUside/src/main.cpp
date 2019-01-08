#include <Thread.h>
#include <ESP8266WiFi.h>
#include <Wire.h>

//------------------------------------------------------------------------------------
  //Control DC Motor pins
#define RightMotorSpeed 5
#define RightMotorDir   0  //B
#define LeftMotorSpeed  4  //A
#define LeftMotorDir    2
//------------------------------------------------------------------------------------
  // Network Name and Password
  char*       net_ssid = "Storehouse 1.0";              // WIFI NAME
  char*       net_pass = "12345678";          // PASSWORD
//------------------------------------------------------------------------------------

#define     MAXSC     6           // MAXIMUM NUMBER OF CLIENTS

WiFiServer  daServer(1987);
WiFiClient  daClient[MAXSC];



Thread ardu_connection = Thread();

int mDirection[] = {1,2,3,4};
int startDirection = 1;
int direction = startDirection;
int route[3];


int positionDir[20][9] = {
  {0, 0, 0, 0, 1, 10, 0, 0, 0}, //1
  {0, 0, 0, 0, 2, 13, 0, 0, 0}, //2
  {0, 0, 0, 0, 3, 16, 0, 0, 0}, //3
  {0, 0, 0, 0, 4, 19, 0, 0, 0}, //4
  {0, 17, 0, 0, 5, 0, 0, 0, 0}, //5
  {0, 0, 0, 20, 6, 0, 0, 0, 0}, //6
  {0, 0, 0, 18, 7, 0, 0, 0, 0}, //7
  {0, 0, 0, 15, 8, 0, 0, 0, 0}, //8
  {0, 0, 0, 12, 9, 0, 0, 0, 0}, //9
  {0, 0, 0, 1, 10, 11, 0, 13, 0}, //10
  {0, 0, 0, 10, 11, 12, 0, 14, 0}, //11
  {0, 0, 0, 11, 12, 9, 0, 15, 0}, //12
  {0, 10, 0, 2, 13, 14, 0, 16, 0}, //13
  {0, 11, 0, 13, 14, 15, 0, 17, 0}, //14
  {0, 12, 0, 14, 15, 8, 0, 18, 0}, //15
  {0, 13, 0, 3, 16, 17, 0, 19, 0}, //16
  {0, 14, 0, 16, 17, 18, 0, 5, 0}, //17
  {0, 15, 0, 17, 18, 7, 0, 20, 0}, //18
  {0, 16, 0, 4, 19, 0, 0, 0, 0}, //19
  {0, 18, 0, 0, 20, 6, 0, 0, 0} //20
};

int routeTo1[9][7] = {{1}, //1
            {1, 10, 13, 2}, //2
            {1, 10, 13, 16, 3}, //3
            {1, 10, 13, 16, 19, 4}, //4
            {1, 10, 11, 14, 17, 5}, //5
            {1, 10, 11, 12, 15, 18, 20}, //6
            {1, 10, 11, 12, 15, 18, 7}, //7
            {1, 10, 11, 12, 15, 8}, //8
            {1, 10, 11, 12, 12, 9} //9
          };


int *path;// = {19, 16, 3};

boolean movementFlag = false;
boolean modeFlag = false;
boolean set_route = false;

void read_sensor_input();
void SetWifi(char * Name, char * Password);
void AvailableClients();
void AvailableMessage();

int minSpeed = 650;
int maxSpeed = 1023;
String tmp = "";

char d1 = '0';
char d2 = '0';
char d3 = '0';
char d4 = '0';
char d5 = '0';

void setup() {
  Serial.begin(9600); /* begin serial for debug */
  Wire.begin(D5, D6); /* join i2c bus with SDA=D5 and SCL=D6 of NodeMCU */
  pinMode(RightMotorSpeed, OUTPUT); //right spped
  pinMode(LeftMotorSpeed, OUTPUT); //left speed
  pinMode(RightMotorDir, OUTPUT); //right direction
  pinMode(LeftMotorDir, OUTPUT); //left direction
  ardu_connection.onRun(read_sensor_input);
  ardu_connection.setInterval(0);

  // Setting Wifi Access Point
  SetWifi("Storehouse 1.0", "12345678");
}

void read_sensor_input() {
  d1 = '0';
  d2 = '0';
  d3 = '0';
  d4 = '0';
  d5 = '0';
  Wire.beginTransmission(8); // begin with device address 8
  Wire.requestFrom(8, 13); // request & read data of size 13 from slave
  int i = 0;
  while (Wire.available()) {
    char c = Wire.read();
    //Serial.print(c);
    if (i == 0) d1 = c;
    if (i == 2) d2 = c;
    if (i == 4) d3 = c;
    if (i == 6) d4 = c;
    if (i == 8) d5 = c;
    i++;
  }
  //Serial.println();
}

void forward() {
  digitalWrite(RightMotorDir, LOW);
  digitalWrite(LeftMotorDir, HIGH);
  analogWrite(RightMotorSpeed, minSpeed);
  analogWrite(LeftMotorSpeed, minSpeed);
}

void backward() {
  digitalWrite(RightMotorDir, HIGH);
  digitalWrite(LeftMotorDir, LOW);
  analogWrite(RightMotorSpeed, minSpeed);
  analogWrite(LeftMotorSpeed, minSpeed);
}

void right() {
  digitalWrite(RightMotorDir, LOW);
  digitalWrite(LeftMotorDir, LOW);
  analogWrite(RightMotorSpeed, minSpeed);
  analogWrite(LeftMotorSpeed, LOW);
}

void left() {
  digitalWrite(RightMotorDir, HIGH);
  digitalWrite(LeftMotorDir, HIGH);
  analogWrite(RightMotorSpeed, LOW);
  analogWrite(LeftMotorSpeed, minSpeed);
}

void left_bothWheels() {
  digitalWrite(RightMotorDir, HIGH);
  digitalWrite(LeftMotorDir, HIGH);
  analogWrite(RightMotorSpeed, minSpeed);
  analogWrite(LeftMotorSpeed, minSpeed);
}

void right_bothWheels() {
  digitalWrite(RightMotorDir, LOW);
  digitalWrite(LeftMotorDir, LOW);
  analogWrite(RightMotorSpeed, minSpeed);
  analogWrite(LeftMotorSpeed, minSpeed);
}

void stop() {
  digitalWrite(RightMotorDir, LOW);
  digitalWrite(LeftMotorDir, LOW);
  analogWrite(RightMotorSpeed, LOW);
  analogWrite(LeftMotorSpeed, LOW);
}

int elemPosition(int row, int elemName) {
  int i = 0;
  int pos = 0;
  for (auto x: positionDir[row - 1]) {
    if (x == elemName) {
      pos = i;
      break;
    }
    i++;
  }
  return pos;
}

int dirToNextPos(int nextPosIndex) {
  int directionToNextPosition = 0;
  if (nextPosIndex == 1) {
    //Serial.println("1");
    directionToNextPosition = 1;
  }
  if (nextPosIndex == 3) {
    //Serial.println("2");
    directionToNextPosition = 2;
  }
  if (nextPosIndex == 5) {
    //Serial.println("3");
    directionToNextPosition = 3;
  }
  if (nextPosIndex == 7) {
    //Serial.println("4");
    directionToNextPosition = 4;
  }
  return directionToNextPosition;
}

int sideForRotation(int direction, int directionToNextPosition) {
  int rotationTo = 0;

  if (direction == 1 && directionToNextPosition == 1) rotationTo = 0;
  if (direction == 1 && directionToNextPosition == 2) rotationTo = 1;
  if (direction == 1 && directionToNextPosition == 3) rotationTo = 2;
  if (direction == 1 && directionToNextPosition == 4) rotationTo = 2;

  if (direction == 2 && directionToNextPosition == 1) rotationTo = 2;
  if (direction == 2 && directionToNextPosition == 2) rotationTo = 0;
  if (direction == 2 && directionToNextPosition == 3) rotationTo = 1;
  if (direction == 2 && directionToNextPosition == 4) rotationTo = 1;

  if (direction == 3 && directionToNextPosition == 1) rotationTo = 1;
  if (direction == 3 && directionToNextPosition == 2) rotationTo = 1;
  if (direction == 3 && directionToNextPosition == 3) rotationTo = 0;
  if (direction == 3 && directionToNextPosition == 4) rotationTo = 2;

  if (direction == 4 && directionToNextPosition == 1) rotationTo = 1;
  if (direction == 4 && directionToNextPosition == 2) rotationTo = 2;
  if (direction == 4 && directionToNextPosition == 3) rotationTo = 1;
  if (direction == 4 && directionToNextPosition == 4) rotationTo = 0;

  return rotationTo;
}

int rotAngle(int direction, int directionToNextPosition) {
  int rotationAngle = 0;

  if (direction == 1 && directionToNextPosition == 1) rotationAngle = 0;
  if (direction == 1 && directionToNextPosition == 2) rotationAngle = 90;
  if (direction == 1 && directionToNextPosition == 3) rotationAngle = 90;
  if (direction == 1 && directionToNextPosition == 4) rotationAngle = 180;

  if (direction == 2 && directionToNextPosition == 1) rotationAngle = 90;
  if (direction == 2 && directionToNextPosition == 2) rotationAngle = 0;
  if (direction == 2 && directionToNextPosition == 3) rotationAngle = 180;
  if (direction == 2 && directionToNextPosition == 4) rotationAngle = 90;

  if (direction == 3 && directionToNextPosition == 1) rotationAngle = 90;
  if (direction == 3 && directionToNextPosition == 2) rotationAngle = 180;
  if (direction == 3 && directionToNextPosition == 3) rotationAngle = 0;
  if (direction == 3 && directionToNextPosition == 4) rotationAngle = 90;

  if (direction == 4 && directionToNextPosition == 1) rotationAngle = 180;
  if (direction == 4 && directionToNextPosition == 2) rotationAngle = 90;
  if (direction == 4 && directionToNextPosition == 3) rotationAngle = 90;
  if (direction == 4 && directionToNextPosition == 4) rotationAngle = 0;

  return rotationAngle;
}

void rotation(int rotationTo, int rotationAngle) {
  if (rotationAngle == 90) {
    if (rotationTo == 1) {
      forward();
      delay(700);
      left_bothWheels();
      delay(500);
      ardu_connection.run();
      while ((d2 != '1') && (d3 != '1')) {
        ardu_connection.run();
        left_bothWheels();
      }
      left_bothWheels();
      delay(300);
      backward();
      delay(300);

      Serial.println("90-1");
    } else if (rotationTo == 2) {
      forward();
      delay(700);
      right_bothWheels();
      delay(300);
      ardu_connection.run();
      //Serial.printf("Outside: d1 = %c ; d2 = %c; d3 = %c; d4 = %c ", d1, d2, d3, d4);
      Serial.println();
      while ((d2 != '1') && (d3 != '1')) {
        ardu_connection.run();
        //Serial.printf("Iside: d1 = %c ; d2 = %c; d3 = %c; d4 = %c ", d1, d2, d3, d4);
        Serial.println();

        right_bothWheels();
      }
      right_bothWheels();
      delay(500);
      //Serial.println("90-2");
      backward();
      delay(300);
    }
  }
  if (rotationAngle == 180) {
    if (rotationTo == 1) {
      forward();
      delay(500);
      left_bothWheels();
      delay(2500);
      while ((d2 != '1') && (d3 != '1')) {
        ardu_connection.run();
        left_bothWheels();
      }
      //stop();
      //delay(500);
      Serial.println("180-1");
    } else if (rotationTo == 2) {
      forward();
      delay(500);
      right_bothWheels();
      delay(2500);
      while ((d2 != '1') && (d3 != '1')) {
        ardu_connection.run();
        right_bothWheels();
      }
    }
    //stop();
    //delay(500);
    Serial.println("180-2");
  }
}

void straightMove() {
  boolean flStop = true;
  ardu_connection.run();
  Serial.printf("Outside: d1 = %c ; d2 = %c; d3 = %c; d4 = %c ", d1, d2, d3, d4);
  Serial.println();
  while (flStop) {
    Serial.printf("Iside: d1 = %c ; d2 = %c; d3 = %c; d4 = %c ", d1, d2, d3, d4);
    ardu_connection.run();
    if (d1 == '0' && d2 == '1' && d3 == '1' && d4 == '0') {
      forward();
    }
    if (d1 == '0' && d2 == '1' && d3 == '0' && d4 == '0') {
      left();
    }
    if (d1 == '0' && d2 == '0' && d3 == '1' && d4 == '0') {
      right();
    }
    if (d1 == '1' && d2 == '1' && d3 == '1' && d4 == '1') flStop = false;
    if (d1 == '0' && d2 == '1' && d3 == '1' && d4 == '1') flStop = false;
    if (d1 == '1' && d2 == '1' && d3 == '1' && d4 == '0') flStop = false;
    Serial.println();
  }
}

void movement(int nextPosIndex, int directionToNextPosition, int rotationTo, int rotationAngle) {

  if (rotationAngle != 0) {
    rotation(rotationTo, rotationAngle);
  } else if (rotationAngle == 0) {
    forward();
    delay(500);
  }
  straightMove();
}

boolean route_is_built = false;

void selectRoute(){
  if (route[0] == 1){
    int k = 0;
    for (int numb: routeTo1[route[2]-1]){
      k++;
      Serial.println(k);
    }
    path = new int[k];
    path = routeTo1[route[2]-1];

  }
}

void loop() {
  int nextPosIndex = 0;
  int directionToNextPosition = 0;
  int rotationAngle = 0;
  int rotationTo = 0; //1 - left; 2 - right
  stop();

  if (!route_is_built) {
    // Checking For Available Clients
    AvailableClients();
    // Checking For Available Client Messages
    AvailableMessage();
  }

  //delay(1000);
  //movementFlag = true;

  if (route_is_built) {

    Serial.println("-------");
    selectRoute();
    Serial.println(sizeof(path) / sizeof(path[0]) - 1);
    for(int i = 0; i < sizeof(path) / sizeof(path[0]) - 1; i++){
     Serial.print(i);
    }
    Serial.println("-------");
    if (movementFlag && !modeFlag) {


       for (int i = 0; i < sizeof(path) / sizeof(path[0]) - 1; i++) {
      //for (int i = 0; i < sizeof(routeTo1) / sizeof(routeTo1[route[i]]) - 1; i++) {

        nextPosIndex = elemPosition(path[i], path[i + 1]); //индекс следующей позиции
        //nextPosIndex = elemPosition(routeTo1[route[i]], routeTo1[route[i+1]]); //индекс следующей позиции
        directionToNextPosition = dirToNextPos(nextPosIndex); //направление к следующей позиции
        rotationTo = sideForRotation(direction, directionToNextPosition); //сторона поворота
        rotationAngle = rotAngle(direction, directionToNextPosition); //угол поворота

        Serial.print("current direction: ");
        Serial.print(direction);
        Serial.println();
        Serial.print("directionToNextPosition: ");
        Serial.print(directionToNextPosition);
        Serial.println();
        Serial.print("rotationTo: ");
        Serial.print(rotationTo);
        Serial.println();
        Serial.print("rotationAngle: ");
        Serial.print(rotationAngle);
        Serial.println();
        direction = directionToNextPosition;
        movement(nextPosIndex, directionToNextPosition, rotationTo, rotationAngle);
        Serial.println();
      }
      movementFlag = false;
      modeFlag = true;
    }

    if (!movementFlag && modeFlag) {
      Wire.beginTransmission(8); // begin with device address 8
      Wire.write('1'); // sends hello string
      Wire.endTransmission(); // stop transmitting
      //test = false;
    }
    while (d5 != '1') {
      stop();
      delay(20);
      ardu_connection.run();
    }
    if ((d5 == '1') && (modeFlag)) {
      delay(1000);
      backward();
      delay(1000);
      modeFlag = false;
      Wire.beginTransmission(8); // begin with device address 8
      Wire.write('0'); // sends hello string
      Wire.endTransmission(); // stop transmitting
    }
    stop();
  }
}

//====================================================================================

void SetWifi(char * Name, char * Password) {
  // Stop Any Previous WIFI
  WiFi.disconnect();

  // Setting The Wifi Mode
  WiFi.mode(WIFI_AP_STA);
  Serial.println("WIFI Mode : AccessPoint Station");

  // Setting The AccessPoint Name & Password
  net_ssid = Name;
  net_pass = Password;

  // Starting The Access Point
  WiFi.softAP(net_ssid, net_pass);
  Serial.println("WIFI << " + String(net_ssid) + " >> has Started");

  // Wait For Few Seconds
  delay(2000);

  // Getting Server IP
  IPAddress IP = WiFi.softAPIP();

  // Printing The Server IP Address
  Serial.print("Server IP : ");
  Serial.println(IP);

  // Starting Server
  daServer.begin();
  daServer.setNoDelay(true);
  Serial.println("Server Started, you can connect from the JAVA client");
}

//====================================================================================

void AvailableClients() {
  if (daServer.hasClient()) {

    for (uint8_t i = 0; i < MAXSC; i++) {
      //find free/disconnected spot
      if (!daClient[i] || !daClient[i].connected()) {
        // Checks If Previously The Client Is Taken
        if (daClient[i]) {
          daClient[i].stop();
        }

        // Checks If Clients Connected To The Server
        if (daClient[i] = daServer.available()) {
          Serial.println("New Client: " + String(i));
        }

        // Continue Scanning
        continue;
      }
    }

    //no free/disconnected spot so reject
    WiFiClient daClient = daServer.available();
    daClient.stop();
  }
}

//====================================================================================

void AvailableMessage() {
  //check clients for data
  for (uint8_t i = 0; i < MAXSC; i++) {
    if (daClient[i] && daClient[i].connected() && daClient[i].available()) {
      while (daClient[i].available()) {
        String Message = daClient[i].readStringUntil('!');
        //Serial.println(Message);
        //tmp = tmp + Message;
        daClient[i].flush();
        //int cmd=48+i;
        Serial.println(Message);
        if (Message == "0" && !set_route) {
          set_route = true;
        } else if (Message == "0" && set_route) {
          set_route = false;
          //int cnt = 0;
          for (int i = 1; i < 4; i ++) {
            //Serial.print(tmp[i]);
            if (tmp[i] == '1') route[i-1] = 1;
            if (tmp[i] == '2') route[i-1] = 2;
            if (tmp[i] == '3') route[i-1] = 3;
            if (tmp[i] == '4') route[i-1] = 4;
            if (tmp[i] == '5') route[i-1] = 5;
            if (tmp[i] == '6') route[i-1] = 6;
            if (tmp[i] == '7') route[i-1] = 7;
            if (tmp[i] == '8') route[i-1] = 8;
            if (tmp[i] == '9') route[i-1] = 9;
          }
          tmp = "";
          for (int i = 0; i<3; i++){
            Serial.print(route[i]);
          }
          route_is_built = true;
        }

        if (set_route) {
          tmp = tmp + Message;
          //Serial.println(character);
        }
        //Serial.println(sizeof(tmp)/sizeof(' '));

        /*for(int i=0;i<7;i++)
        {
          int cmd=48+i;
          if((int)Message[0]==cmd)
          {
          //  digitalWrite(outputIndex[i],HIGH);
            break;
          }
          cmd=64+i+7;
          if((int)Message[0]==cmd)
          {
          //  digitalWrite(outputIndex[i],LOW);
            break;
          }
        }*/
      }
    }
  }
}
//====================================================================================
