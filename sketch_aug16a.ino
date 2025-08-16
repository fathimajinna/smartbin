#include <Servo.h>
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>

// 🔹 Your WiFi credentials
#define WIFI_SSID "moto g64 5G_3586"
#define WIFI_PASSWORD "fathimaaa"

// 🔹 Firebase credentials
#define FIREBASE_HOST "binbuddy-d1372-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "pMMwHqMhUzd79uR5S2d9ftNR0I59b4fixJ6vBrzl"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Global variables
Servo lidServo;

// Pin definitions
#define IR_PIN D5        // IR sensor pin to detect presence
#define TRIG_PIN D6      // Ultrasonic sensor Trig pin
#define ECHO_PIN D7      // Ultrasonic sensor Echo pin
#define BUZZER_PIN D8    // Buzzer pin for alerts
#define SERVO_PIN D4     // Servo motor pin for lid control
#define GREEN_LED D3     // Green LED to indicate motion detection
#define RED_LED D0       // Red LED to indicate bin is full

// Function prototypes
void setupWiFi();
void setupFirebase();
int getBinLevel();
void handleLid();
void checkBinStatus(int binLevel);
void updateFirebase(int binLevel, int irState);

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize WiFi and Firebase
  setupWiFi();
  setupFirebase();

  // Pin setup
  pinMode(IR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);

  // Attach servo to its pin and ensure lid is closed
  lidServo.attach(SERVO_PIN);
  lidServo.write(0); // closed position
  delay(500);
  lidServo.detach(); // detach to save power
}

void loop() {
  // Read sensor data
  int irState = digitalRead(IR_PIN);
  int binLevel = getBinLevel();

  // Print sensor data to Serial Monitor for debugging
  Serial.print("IR State: ");
  Serial.println(irState);
  Serial.print("Bin Level (cm): ");
  Serial.println(binLevel);

  // Control the lid based on IR sensor
  if (irState == LOW) {
    handleLid();
  }

  // Check and update bin status based on ultrasonic sensor
  checkBinStatus(binLevel);

  // Upload data to Firebase
  updateFirebase(binLevel, irState);

  // Update every second to prevent flooding Firebase with requests
  delay(1000);
}

// 🔹 Helper functions

// Function to set up WiFi connection
void setupWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

// Function to set up Firebase connection
void setupFirebase() {
  config.host = FIREBASE_HOST;
  auth.token.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// Function to get the distance from the ultrasonic sensor
int getBinLevel() {
  // Clear the trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  // Set the trigger pin to HIGH for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echo pin's duration
  long duration = pulseIn(ECHO_PIN, HIGH);
  // Calculate distance in cm (speed of sound: 0.034 cm/us)
  int distance = duration * 0.034 / 2;
  return distance;
}

// Function to handle the opening and closing of the lid
void handleLid() {
  Serial.println("IR sensor triggered. Opening lid...");
  digitalWrite(GREEN_LED, HIGH);
  lidServo.attach(SERVO_PIN);
  lidServo.write(90); // open position
  delay(3000); // lid stays open for 3 seconds
  lidServo.write(0); // close position
  delay(500);
  lidServo.detach(); // detach to save power
  digitalWrite(GREEN_LED, LOW);
  Serial.println("Lid closed.");
}

// Function to check the bin's fill status and trigger alerts
void checkBinStatus(int binLevel) {
  if (binLevel < 10) {
    // Bin is considered full if less than 10 cm from the top
    Serial.println("Warning: Bin is FULL!");
    digitalWrite(RED_LED, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    if (Firebase.ready()) {
      Firebase.setString(fbdo, "/bin/status", "FULL");
    }
  } else {
    // Bin is not full
    digitalWrite(RED_LED, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    if (Firebase.ready()) {
      Firebase.setString(fbdo, "/bin/status", "OK");
    }
  }
}

// Function to upload sensor data to Firebase
void updateFirebase(int binLevel, int irState) {
  Serial.print("Firebase status: ");
  Serial.println(Firebase.ready() ? "Connected" : "Not connected");

  if (Firebase.ready()) {
    if (!Firebase.setInt(fbdo, "/bin/level", binLevel)) {
      Serial.print("Firebase FAILED: ");
      Serial.println(fbdo.errorReason());
    }
    if (!Firebase.setInt(fbdo, "/bin/irState", irState)) {
      Serial.print("Firebase FAILED: ");
      Serial.println(fbdo.errorReason());
    }
  }
}
