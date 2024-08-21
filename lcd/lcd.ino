#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Fingerprint.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "HTTPSRedirect.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#if (defined(AVR) || defined(ESP8266)) && !defined(AVR_ATmega2560)
#define FINGERPRINT_RX_PIN 12  // GPIO12 (D6) for RX on NodeMCU
#define FINGERPRINT_TX_PIN 13  // GPIO13 (D7) for TX on NodeMCU

#include <SoftwareSerial.h>
SoftwareSerial mySerial(FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN);

#define RELAY_PIN 16  // Define the pin connected to the relay module

// Object initialization
LiquidCrystal_I2C lcd(0x27, 16, 2);

#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Enter network credentials:
const char *ssid = "nrayqn_";
const char *password = "khatijah";


// Enter Google Script Deployment ID:
const char *GScriptId = "AKfycbxKQQ7LbcP8WAh6kiuqs6dLe3cLk_7410tg2ACHgbbnC6bxtO8ZbN-heceVD0MfTnFmdQ";

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base = "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";

// Google Sheets setup (do not edit)
const char *host = "script.google.com";
const int httpsPort = 443;
const char *fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec";
HTTPSRedirect *client = nullptr;

// Declare variables that will be published to Google Sheets
String user = "";
String id = "";
String status = "";

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup()
{
  Serial.begin(9600);
  Serial.println('\n');
  Serial.println("System initialized");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());

  // Use HTTPSRedirect class to create a new TLS connection
  client = new HTTPSRedirect(httpsPort);
  client->setInsecure();
  client->setPrintResponseBody(true);
  client->setContentTypeHeader("application/json");

  Serial.print("Connecting to ");
  Serial.println(host);

  // Try to connect for a maximum of 5 times
  bool flag = false;
  for (int i = 0; i < 5; i++)
  {
    int retval = client->connect(host, httpsPort);
    if (retval == 1)
    {
      flag = true;
      Serial.println("Connected");
      break;
    }
    else
    {
      Serial.println("Connection failed. Retrying...");
    }
  }
  if (!flag)
  {
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    return;
  }

  // Initialize LCD
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();

  // Initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.clearDisplay();
  display.setTextColor(WHITE);
  delay(10);

  while (!Serial)
  {
    ; // For Yun/Leo/Micro/Zero/...
  }
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
  }
  else
  {
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
    {
      delay(1);
    }
  }
  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0)
  {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else
  {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
  }

  // Initialize Relay pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure relay is off initially
}

void loop()
{
  // Display waiting message
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 10);
  display.print("Waiting...");
  display.display();

  // Get fingerprint ID
  getFingerprintID();
}

void getFingerprintID()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image taken");
    break;
  case FINGERPRINT_NOFINGER:
    Serial.println("No finger detected");
    return;
  case FINGERPRINT_IMAGEFAIL:
    Serial.println("Imaging error");
    return;
  default:
    Serial.println("Unknown error");
    return;
  }

  // OK success!
  p = finger.image2Tz();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return;
  default:
    Serial.println("Unknown error");
    return;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Found a print match!");
    // Update user and id based on the fingerprint ID
    if (finger.fingerID == 1)
    {
      user = "Aleeya";
      id = "1";
    }
    else if (finger.fingerID == 2)
    {
      user = "Amira";
      id = "2";
    }
    else if (finger.fingerID == 3)
    {
      user = "Asyiqin";
      id = "3";
    }
    else if (finger.fingerID == 4)
    {
      user = "Qasrina";
      id = "4";
    }
    else if (finger.fingerID == 5)
    {
      user = "Syiqin";
      id = "5";
    }
     else if (finger.fingerID == 7)
    {
      user = "Putri";
      id = "7";
    }
    else{
      user = "Unknown";
      id = String(finger.fingerID);
    }
    status = "Successful";
    updatesheet(user, id, status);

    // Unlock the solenoid
    digitalWrite(RELAY_PIN, HIGH);

    // Display success message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SUCCESSFUL");
    lcd.setCursor(0, 1);
    lcd.print("THANKS");

    delay(10000); // Display the message for 10 seconds

    // Lock the solenoid automatically
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Solenoid locked automatically.");

    lcd.clear();
  }
  else
  {
    Serial.println("Did not find a match or unknown error");
    // Ensure the solenoid is locked
    digitalWrite(RELAY_PIN, LOW);

    // Update user and id for unknown fingerprint
    user = "Unknown";
    id = "0";
    status = "Failed";
    updatesheet(user, id, status);

    // Display unsuccessful message on LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("UNSUCCESSFUL");
    lcd.setCursor(0, 1);
    lcd.print("TRY AGAIN");
    delay(10000); // Display the message for 10 seconds
    lcd.clear();
  }
}

void updatesheet(String user, String id, String status)
{
  if (client == nullptr)
  {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (!client->connected())
  {
    client->connect(host, httpsPort);
  }

  // Create JSON object string to send to Google Sheets
  payload = payload_base + "\"" + user + "," + id + "," + status + "\"}";

  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if (client->POST(url, host, payload))
  {
    // Do something if needed after successful POST
  }
  else
  {
    Serial.println("Failed to publish data");
  }
}