#include <Arduino.h>
#include <WiFiS3.h>
#include <FirebaseClient.h>
#include <WiFiSSLClient.h>
#include <Servo.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Stepper.h>

#include "HUSKYLENS.h"

HUSKYLENS huskylens;

#define FIREBASE_HOST "https://fishfeeder-xxxx-default-rtdb.firebaseio.com/"
#define WIFI_SSID "<SSID>"
#define WIFI_PASSWORD "<PASS>"


// Defines the number of steps per rotation
const int stepsPerRevolution = 2038/2;

// Pins entered in sequence IN1-IN3-IN2-IN4 for proper step sequence
Stepper myStepper = Stepper(stepsPerRevolution, 11, 10, 9, 8);

Servo servo;

WiFiSSLClient ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult timer, feed;
NoAuth noAuth;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);

String stimer;
String Str[] = {"00:00", "00:00", "00:00"};
int i, feednow = 0;
String oldDoorStatus;
bool status;


// Twilio credentials
const char* accountSid = "<SID>";
const char* authToken = "<authToken>";

// Twilio API endpoint
const char* twilioUrl = "api.twilio.com";
const int httpsPort = 443;

// Recipient phone number
const char* toNumber = "<toNumber>";

// Message body
const char* messageBody = "Predator Detected!";

WiFiSSLClient cclient;

void printError(int code, const String &msg)
{
    Serial.print("Error, msg: ");
    Serial.print(msg);
    Serial.print(", code: ");
    Serial.println(code);
}

void setup()
{
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Wire.begin();
    while (!huskylens.begin(Wire))
    {
        Serial.println(F("Begin failed!"));
        delay(100);
    }

    timeClient.begin();
    
    // Initialize Firebase
    initializeApp(client, app, getAuth(noAuth));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(FIREBASE_HOST);

    // Set async result
    client.setAsyncResult(timer);
    client.setAsyncResult(feed);

    // Attach servo to pin D5
    servo.attach(D6);
    oldDoorStatus = Database.get<String>(client, "/doorStatus");
}

void sendTwilioMessage() {
  if (WiFi.status() == WL_CONNECTED) {
    // Connect to Twilio server
    if (!cclient.connect(twilioUrl, httpsPort)) {
      Serial.println("Connection to Twilio failed!");
      return;
    }

    // Create the HTTP POST request
    String postData = "To=" + String(toNumber) + "&Body=" + String(messageBody);
    String auth = String(accountSid) + ":" + String(authToken);
    String encodedAuth = "<encoded>";

    cclient.println("POST /2010-04-01/Accounts/" + String(accountSid) + "/Messages.json HTTP/1.1");
    cclient.println("Host: " + String(twilioUrl));
    cclient.println("Authorization: Basic " + encodedAuth);
    cclient.println("Content-Type: application/x-www-form-urlencoded");
    cclient.println("Content-Length: " + String(postData.length()));
    cclient.println();
    cclient.println(postData);

    // Read the response
    while (cclient.connected()) {
      String line = cclient.readStringUntil('\n');
      if (line == "\r") {
        break;
      }
    }

    // Print response body
    String responseBody = cclient.readString();
    Serial.println("Response:");
    Serial.println(responseBody);
  } else {
    Serial.println("WiFi not connected");
  }
}

void openDoor(){
  // Rotate CCW quickly at 10 RPM
  myStepper.setSpeed(5);
  myStepper.step(-stepsPerRevolution);
  delay(1000);
}

void closeDoor(){
  // Rotate CW slowly at 5 RPM
  myStepper.setSpeed(5);
  myStepper.step(stepsPerRevolution);
  delay(1000);
}

void loop()
{   
    huskylens.request();
    HUSKYLENSResult result = huskylens.read();
    if (result.ID > 0){
      sendTwilioMessage();
    }
    String doorStatus = Database.get<String>(client, "/doorStatus"); 
    if (doorStatus == "Open" && oldDoorStatus =="Close"){
      //status = Database.set<String>(client, "/doorStatus", "Close");
      Serial.println("Door Opening");
      openDoor();
      oldDoorStatus = "Open";
    }
    else if (doorStatus == "Close" && oldDoorStatus == "Open"){
      //status = Database.set<String>(client, "/doorStatus", "Open");
      Serial.println("Door Closing");
      closeDoor();
      oldDoorStatus = "Close";
    }
    else{
      timeClient.update();
      String currentTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
      String openingTime_str = Database.get<String>(client, "/OpeningTime");
      String openingTime = openingTime_str.substring(9,14);
      String closingTime_str = Database.get<String>(client, "/ClosingTime");
      String closingTime = closingTime_str.substring(9,14);

      if (currentTime == openingTime){
        openDoor();
      }
      else if (currentTime == closingTime){
        closeDoor();
      }
    }

    feednow = Database.get<int>(client, "/feednow"); //feed.to<int>();
    Serial.println(feednow);
    if (feednow == 1) // Direct Feeding
    {
        servo.writeMicroseconds(1000); // rotate clockwise
        delay(700); // allow to rotate for n milliseconds, you can change this to your need
        servo.writeMicroseconds(1500); // stop rotation
        feednow = 0;
        bool status = Database.set<int>(client, "/feednow", feednow);
        Serial.println("Fed");
    }
    else // Scheduling feed
    {
        for (i = 0; i < 3; i++)
        {
            String path = "timers/timer" + String(i);
            stimer = Database.get<String>(client, path);//timer.to<String>();
            Str[i] = stimer.substring(9, 14);
        }
        timeClient.update();
        String currentTime = String(timeClient.getHours()) + ":" + String(timeClient.getMinutes());
        if (Str[0] == currentTime || Str[1] == currentTime || Str[2] == currentTime)
        {
            servo.writeMicroseconds(1000); // rotate clockwise
            delay(700); // allow to rotate for n milliseconds, you can change this to your need
            servo.writeMicroseconds(1500); // stop rotation
            Serial.println("Success");
            delay(60000);
        }
    }
    Str[0] = "00:00";
    Str[1] = "00:00";
    Str[2] = "00:00";
}
