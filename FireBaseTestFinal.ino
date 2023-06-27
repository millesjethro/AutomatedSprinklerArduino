#include <Firebase_ESP_Client.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <EEPROM.h>
#include <DHT.h>
#include <Wire.h>
#include "time.h"
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <AutoConnect.h>
#define DHTTYPE DHT11
#define API_KEY "AIzaSyAlJ8HdEH_Cwhkfhpv0ASyAG13WwAvEL8I"                                           //API key for Firebase
#define DATABASE_URL "https://thesisproj-134de-default-rtdb.asia-southeast1.firebasedatabase.app/"  //URL of the Firebase

WebServer Server;
AutoConnect Portal(Server);

#define Valve1 26  //Valve 1 Pin
#define Valve2 27  //Valve 2 Pin
#define DHT211 15  //DHT 1 Pin
#define DHT212 16  //DHT 2 Pin

DHT dht(DHT211, DHTTYPE);   //Declaring dht var as DHT 1
DHT dht1(DHT212, DHTTYPE);  //Declaring dht1 var as DHT 2


FirebaseData fbdo;
FirebaseAuth authentication;
FirebaseConfig config;
// FirebaseJson json;

#define USER_EMAIL "administrator@gmail.com"
#define USER_PASSWORD "RootAdmin"

const int moisturePin = 36;         //VP or VN
const int moisturePin1 = 39;        //VP or VN
static String DeviceID = "003176";  // Device ID
static int Password = 5623;         //Device Password

unsigned long sendDataPrevMillis = 0;
String uid;          //Users Identification
int temp = 0;      //Variable temperature
int humid = 0;     //Variable Humidity
int moisture = 0;  //Variable moiisture

int temp1 = 0;      //Variable temperature
int humid1 = 0;     //Variable Humidity
int moisture1 = 0;  //Variable moiisture

int operationNum = 0;  //Variable Operation number

int humlim = 0;  //Variable humidity Limit
int moilim = 0;  //Varible Moisture Limit

int setMin = 0;
int setHour = 0;
int setDelay = 0;

String valve_1 = "OFF";  //Variable valve 1 Status
String valve_2 = "OFF";  //Variable valve 2 Status
int OnceOper = 1;

bool HourIsOK = false;
bool ValveON = false;
bool Valve1ON = false;
bool Valve2ON = false;

bool NoInternetMode = false;

const char* ntpServer = "asia.pool.ntp.org";
const long gmtOffset_sec = 25200;
const int daylightOffset_sec = 3600;

void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  dht1.begin();
  pinMode(Valve1, OUTPUT);
  pinMode(Valve2, OUTPUT);
  digitalWrite(Valve1, HIGH);
  digitalWrite(Valve2, HIGH);
  Server.on("/", rootPage);
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }

  Serial.println("Connected with IP: ");
  Serial.println(WiFi.localIP());  // Local Wifi IP ADDRESS

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  config.api_key = API_KEY;            // Configuring the APIKEY
  config.database_url = DATABASE_URL;  // Configuring database url
  //Authentication For the accounts that is in the database
  //To be use for saving datas
  authentication.user.email = USER_EMAIL;
  authentication.user.password = USER_PASSWORD;
  Firebase.reconnectWiFi(true);
  //Bit size for the response
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  //Firebase.begin is like signing in and checking if database exist and the account
  Firebase.begin(&config, &authentication);
  Serial.println("Getting User UID");
  //While uid is not found
  while ((authentication.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  //Printing the UID of the account
  uid = authentication.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);
}

void loop() {
  Portal.handleClient();
  if (OnceOper == 1) {
    sendOnce();
    OnceOper++;
  }

  //Operation on the device of which it will follow
  chooseOperation();
  if (operationNum == 1) {
    sendData();
    operation1();
  } else if (operationNum == 2) {
    sendData();
    operation2();
  } else if (operationNum == 3) {
    sendData();
    operation3();
  } else if (operationNum == 0) {
    sendData();
    Serial.println("Just Sending Data");
  }

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Lost Connection");
    TurnOffValves();
  }

  delay(2000);
}

void sendOnce() {
  //Checking if the firebase is still working
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    //Setting data on the path device/DeviceID/Password/ and the value
    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/password/", Password)) {
      Serial.print("Sent: ");
      Serial.println(Password);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }
}

void operation1() {
  //Getting a piece of data in the database
  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/motor/Valve1/")) {
    if (fbdo.dataType() == "string") {
      valve_1 = fbdo.stringData();
      Serial.println("Vavle 1:" + valve_1);
      if (valve_1 == "ON") {
        digitalWrite(Valve1, LOW);
      } else {
        digitalWrite(Valve1, HIGH);
      }
    }
  } else {
    Serial.print("FAILED: " + fbdo.errorReason());
  }
  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/motor/Valve2/")) {
    if (fbdo.dataType() == "string") {
      valve_2 = fbdo.stringData();
      Serial.println("Vavle 2:" + valve_2);
      if (valve_2 == "ON") {
        digitalWrite(Valve2, LOW);
      } else {
        digitalWrite(Valve2, HIGH);
      }
    }
  } else {
    Serial.print("FAILED: " + fbdo.errorReason());
  }
}

void operation2(){
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  char timeHour[3];
  strftime(timeHour, 3, "%H", &timeinfo);
  Serial.println(timeHour);
  String H = timeHour;
  int h = H.toInt();
  char timeMins[3];
  strftime(timeMins, 3, "%M", &timeinfo);
  Serial.println(timeMins);
  String M = timeMins;
  int m = M.toInt();

  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/operation/clockhour/")) {
    if (fbdo.dataType() == "string") {
      setHour = fbdo.stringData().toInt();
      Serial.print("Hours Set: ");
      Serial.println(setHour);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }
  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/operation/clockmin/")) {
    if (fbdo.dataType() == "string") {
      setMin = fbdo.stringData().toInt();
      Serial.print("Minute Set: ");
      Serial.println(setMin);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }
  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/operation/timedelay/")) {
    if (fbdo.dataType() == "int") {
      setDelay = fbdo.intData();
      Serial.print("Delay Minutes Set: ");
      Serial.println(setDelay);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }
  
  
  if (setHour == 26 && setMin == 62){
    ValveON = false;
  } else if(setDelay >= 60){
    int maxDelay = setDelay + setMin;
    int hourDelay = maxDelay/60;
    int newHour = setHour + (maxDelay/60);
    int newMin = setMin + (maxDelay - (hourDelay*60));
    if (h == setHour && m == setMin) {
      ValveON = true;
    } else if (h == newHour && m == newMin) {
      ValveON = false;
    }
  } else if (setDelay <= 59 || setDelay >= 1) {
    int maxDelay = setDelay + setMin;
    if(maxDelay >= 60){
      int newHour = setHour + (maxDelay/60);
      int newMin = maxDelay - setMin;
      if (h == setHour && m == setMin) {
        ValveON = true;
      } else if (h == newHour && m == newMin) {
        ValveON = false;
      }
    } else if(maxDelay <= 59){
      if (h == setHour && m == setMin) {
        ValveON = true;
      } else if (h == setHour && m == maxDelay) {
        ValveON = false;
      }
    }
  } else {
    ValveON = false;
  }

  if (ValveON == true) {
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve1/", "ON")) {
      Serial.println("Vavle 1: ON");
      digitalWrite(Valve1, LOW);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve2/", "ON")) {
      Serial.println("Vavle 2: ON");
      digitalWrite(Valve2, LOW);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  } else {
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve1/", "OFF")) {
      Serial.println("Vavle 1: OFF");
      digitalWrite(Valve1, HIGH);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve2/", "OFF")) {
      Serial.println("Vavle 2: OFF");
      digitalWrite(Valve2, HIGH);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }
}

void operation3() {
  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/operation/humiditylimit/")) {
    if (fbdo.dataType() == "string") {
      humlim = fbdo.stringData().toInt();
      Serial.print("Humidity Limit:");
      Serial.println(humlim);
    }
  } else {
    Serial.print("FAILED: " + fbdo.errorReason());
  }

  if (Firebase.RTDB.getString(&fbdo, "/device/" + DeviceID + "/operation/moisturelimit/")) {
    if (fbdo.dataType() == "string") {
      moilim = fbdo.stringData().toInt();
      Serial.print("Moisture Limit:");
      Serial.println(moilim); 
    }
  } else {
    Serial.print("FAILED: " + fbdo.errorReason());
  }

  if(moilim == 101 && humlim == 101){
    Valve1ON = false;
    Valve2ON = false;
  } else if(moilim <= 100 && humlim >= 101){
    if (moisture <= moilim) {
      Valve1ON = true;
    } else if (moisture > moilim){
      Valve1ON = false;
    }
  } else if(moilim >= 101 && humlim <= 100){
    if (humid <= humlim) {
      Valve1ON = true;
    } else if (humid > humlim){
      Valve1ON = false;
    }
  } else if(moilim <= 100 && humlim <= 100){
    if (humid <= humlim && moisture <= moilim) {
      Valve1ON = true;
    } else if (humid > humlim && moisture > moilim){
      Valve1ON = false;
    }
  }

  if(moilim >= 101 && humlim >= 101){
    Valve1ON = false;
    Valve2ON = false;
  } else if(moilim <= 100 && humlim >= 101){
    if (moisture1 <= moilim) {
      Valve2ON = true;
    } else if (moisture1 > moilim){
      Valve2ON = false;
    }
  } else if(moilim >= 101 && humlim <= 100){
    if (humid1 <= humlim) {
      Valve2ON = true;
    } else if (humid1 > humlim){
      Valve2ON = false;
    }
  } else if(moilim <= 100 && humlim <= 100){
    if (humid1 <= humlim && moisture1 <= moilim) {
      Valve2ON = true;
    } else if (humid1 > humlim && moisture1 > moilim){
      Valve2ON = false;
    }
  } 

  if (Valve1ON == true) {
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve1/", "ON")) {
      Serial.println("Vavle 1: ON");
      digitalWrite(Valve1, LOW);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  } else {
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve1/", "OFF")) {
      Serial.println("Vavle 1: OFF");
      digitalWrite(Valve1, HIGH);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }

  if (Valve2ON == true) {
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve2/", "ON")) {
      Serial.println("Vavle 2: ON");
      digitalWrite(Valve2, LOW);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  } else {
    if (Firebase.RTDB.setString(&fbdo, "device/" + DeviceID + "/motor/Valve2/", "OFF")) {
      Serial.println("Vavle 2: OFF");
      digitalWrite(Valve2, HIGH);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }
  }
}

void sendData() {
  if (Firebase.ready() && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    moisture = (100.00 - ((analogRead(moisturePin) / 4095.00) * 100.00));
    temp = dht.readTemperature();
    humid = dht.readHumidity();

    moisture1 = (100.00 - ((analogRead(moisturePin1) / 4095.00) * 100.00));
    temp1 = dht1.readTemperature();
    humid1 = dht1.readHumidity();

    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/sensors/humidity/", humid)) {
      Serial.print("Sent: ");
      Serial.println(humid);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/sensors/temperature/", temp)) {
      Serial.print("Sent: ");
      Serial.println(temp);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/sensors/moisture/", moisture)) {
      Serial.print("Sent: ");
      Serial.println(moisture);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/sensors/humidity1/", humid1)) {
      Serial.print("Sent Humid1: ");
      Serial.println(humid1);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/sensors/temperature1/", temp1)) {
      Serial.print("Sent Temp1: ");
      Serial.println(temp1);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }

    if (Firebase.RTDB.setInt(&fbdo, "device/" + DeviceID + "/sensors/moisture1/", moisture1)) {
      Serial.print("Sent Moisture1: ");
      Serial.println(moisture1);
    } else {
      Serial.print("FAILED: " + fbdo.errorReason());
    }

  }
}

void chooseOperation() {
  if (Firebase.RTDB.getInt(&fbdo, "/device/" + DeviceID + "/operation/number/")) {
    if (fbdo.dataType() == "int") {
      operationNum = fbdo.intData();
      Serial.print("Operation Number:");
      Serial.println(operationNum);
    }
  } else {
    Serial.print("FAILED: " + fbdo.errorReason());
  }
}

void TurnOffValves() {
  digitalWrite(Valve1, HIGH);
  digitalWrite(Valve2, HIGH);
}
