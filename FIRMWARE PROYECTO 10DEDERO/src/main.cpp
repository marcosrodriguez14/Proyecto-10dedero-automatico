#include <HTTPClient.h>
#include "ArduinoJson.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <DHTesp.h>
#include "Stepper_28BYJ_48.h"

// Replace 0 by ID of this current device
const int DEVICE_ID = 1;

int test_delay = 1000; // so we don't spam the API
boolean describe_tests = true;

// Replace 0.0.0.0 by your server local IP (ipconfig [windows] or ifconfig [Linux o MacOS] gets IP assigned to your PC)
String serverName = "http://192.168.43.18:81/";
HTTPClient http;

// Replace WifiName and WifiPassword by your WiFi credentials
#define STASSID "Red"     //"Your_Wifi_SSID"
#define STAPSK "echedeyy" //"Your_Wifi_PASSWORD"

// NTP (Net time protocol) settings
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// MQTT configuration
WiFiClient espClient;
PubSubClient client(espClient);

// Server IP, where de MQTT broker is deployed
const char *MQTT_BROKER_ADRESS = "192.168.43.18";
const uint16_t MQTT_PORT = 1883;

// Name for this MQTT client
const char *MQTT_CLIENT_NAME = "10dedero";

// Pinout settings
const int dhtPin = 4;
const int stepperPin1 = 19;
const int stepperPin2 = 18;
const int stepperPin3 = 5;
const int stepperPin4 = 17;

/////////////////////////////////////////////////////////// ACTUADOR MOTOR PASO A PASO ////////////////////////////////////////////////////////////////

Stepper_28BYJ_48 stepper(stepperPin1, stepperPin2, stepperPin3, stepperPin4);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////// SENSOR TEMPERATURA ////////////////////////////////////////////////////////////////////
DHTesp dht;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////77/

unsigned long timestamp;

String response;

String serializeSensorValueBody(int idSensor, long timestamp, float value)
{
  // StaticJsonObject allocates memory on the stack, it can be
  // replaced by DynamicJsonDocument which allocates in the heap.
  //
  DynamicJsonDocument doc(2048);

  // Add values in the document
  //
  doc["idSensor"] = idSensor;
  doc["timestamp"] = timestamp;
  doc["value"] = value;
  doc["removed"] = false;

  // Generate the minified JSON and send it to the Serial port.
  //
  String output;
  serializeJson(doc, output);
  Serial.println(output);

  return output;
}

String serializeActuatorStatusBody(float status, bool statusBinary, int idActuator, long timestamp)
{
  DynamicJsonDocument doc(2048);

  doc["status"] = status;
  doc["statusBinary"] = statusBinary;
  doc["idActuator"] = idActuator;
  doc["timestamp"] = timestamp;
  doc["removed"] = false;

  String output;
  serializeJson(doc, output);
  return output;
}

String serializeDeviceBody(String deviceSerialId, String name, String mqttChannel, int idGroup)
{
  DynamicJsonDocument doc(2048);

  doc["deviceSerialId"] = deviceSerialId;
  doc["name"] = name;
  doc["mqttChannel"] = mqttChannel;
  doc["idGroup"] = idGroup;

  String output;
  serializeJson(doc, output);
  return output;
}

void deserializeActuatorStatusBody(String responseJson)
{
  if (responseJson != "")
  {
    DynamicJsonDocument doc(2048);

    // Deserialize the JSON document
    DeserializationError error = deserializeJson(doc, responseJson);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // Fetch values.
    int idActuatorState = doc["idActuatorState"];
    float status = doc["status"];
    bool statusBinary = doc["statusBinary"];
    int idActuator = doc["idActuator"];
    long timestamp = doc["timestamp"];

    Serial.println(("Actuator status deserialized: [idActuatorState: " + String(idActuatorState) + ", status: " + String(status) + ", statusBinary: " + String(statusBinary) + ", idActuator" + String(idActuator) + ", timestamp: " + String(timestamp) + "]").c_str());
  }
}

void deserializeDeviceBody(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    int idDevice = doc["idDevice"];
    String deviceSerialId = doc["deviceSerialId"];
    String name = doc["name"];
    String mqttChannel = doc["mqttChannel"];
    int idGroup = doc["idGroup"];

    Serial.println(("Device deserialized: [idDevice: " + String(idDevice) + ", name: " + name + ", deviceSerialId: " + deviceSerialId + ", mqttChannel" + mqttChannel + ", idGroup: " + idGroup + "]").c_str());
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void deserializeSensorsFromDevice(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    // allocate the memory for the document
    DynamicJsonDocument doc(ESP.getMaxAllocHeap());

    // parse a JSON array
    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // extract the values
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject sensor : array)
    {
      int idSensor = sensor["idSensor"];
      String name = sensor["name"];
      String sensorType = sensor["sensorType"];
      int idDevice = sensor["idDevice"];

      Serial.println(("Sensor deserialized: [idSensor: " + String(idSensor) + ", name: " + name + ", sensorType: " + sensorType + ", idDevice: " + String(idDevice) + "]").c_str());
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

int deserializeSensorsFromDevice2(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    // allocate the memory for the document
    DynamicJsonDocument doc(ESP.getMaxAllocHeap());

    // parse a JSON array
    DeserializationError error = deserializeJson(doc, responseJson);

    // if (error)
    // {
    //   Serial.print(F("deserializeJson() failed: "));
    //   Serial.println(error.f_str());
    //   return null;
    // }

    // extract the values
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject sensor : array)
    {
      int idSensor = sensor["idSensor"];
      String name = sensor["name"];
      String sensorType = sensor["sensorType"];
      int idDevice = sensor["idDevice"];

      return idSensor;
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void deserializeActuatorsFromDevice(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    // allocate the memory for the document
    DynamicJsonDocument doc(ESP.getMaxAllocHeap());

    // parse a JSON array
    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }

    // extract the values
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject sensor : array)
    {
      int idActuator = sensor["idActuator"];
      String name = sensor["name"];
      String actuatorType = sensor["actuatorType"];
      int idDevice = sensor["idDevice"];

      Serial.println(("Actuator deserialized: [idActuator: " + String(idActuator) + ", name: " + name + ", actuatorType: " + actuatorType + ", idDevice: " + String(idDevice) + "]").c_str());
    }
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

int deserializeActuatorsFromDevice2(int httpResponseCode)
{

  if (httpResponseCode > 0)
  {
    String responseJson = http.getString();
    DynamicJsonDocument doc(ESP.getMaxAllocHeap());
    DeserializationError error = deserializeJson(doc, responseJson);
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject Actuator : array)
    {
      int idActuator = Actuator["idActuator"];
      return idActuator;
    }
  }
}

int deserializeActuatorsStatus2(int httpResponseCode)
{
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }

    int idActuatorState = doc["idActuatorState"];
    float status = doc["status"];
    int idActuator = doc["idActuator"];
    long timestamp = doc["timestamp"];
    bool removed = doc["removed"];
    bool statusBinary = doc["statusBinary"];

    return status;
    // Serial.println(("Device deserialized: [idDevice: " + String(idDevice) + ", name: " + name + ", deviceSerialId: " + deviceSerialId + ", mqttChannel" + mqttChannel + ", idGroup: " + idGroup + "]").c_str());
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

bool deserializeActuatorsStatus3(int httpResponseCode)
{
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String responseJson = http.getString();
    DynamicJsonDocument doc(2048);

    DeserializationError error = deserializeJson(doc, responseJson);

    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    }

    int idActuatorState = doc["idActuatorState"];
    float status = doc["status"];
    int idActuator = doc["idActuator"];
    long timestamp = doc["timestamp"];
    bool removed = doc["removed"];
    bool statusBinary = doc["statusBinary"];

    return statusBinary;
    // Serial.println(("Device deserialized: [idDevice: " + String(idDevice) + ", name: " + name + ", deviceSerialId: " + deviceSerialId + ", mqttChannel" + mqttChannel + ", idGroup: " + idGroup + "]").c_str());
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void test_response(int httpResponseCode)
{
  delay(test_delay);
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
}

void describe(char *description)
{
  if (describe_tests)
    Serial.println(description);
}

//////////////////////////////////////// GET Y POST  //////////////////////////////////////////

// SENSOR
int GetSensorId()
{
  describe("GET sensors from deviceID");
  String serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/sensors";
  http.begin(serverPath.c_str());
  return deserializeSensorsFromDevice2(http.GET());
}

void PostTemperatura(int id, unsigned long timestamp, float temp)
{
  String sensor_value_body = serializeSensorValueBody(id, timestamp, temp);
  describe("Test POST with sensor value");
  String serverPath = serverName + "api/sensor_values";
  http.begin(serverPath.c_str());
  test_response(http.POST(sensor_value_body));
}

// ACTUADOR
int GetActuatorId()
{
  describe("GET actuators from deviceID");
  String serverPath = serverName + "api/devices/" + String(DEVICE_ID) + "/actuators/Motor";
  http.begin(serverPath.c_str());
  return deserializeActuatorsFromDevice2(http.GET());
}

int GetLastActuatorState()
{
  describe("GET actuator from deviceID");
  String serverPath = serverName + "api/actuator_states/" + "1" + "/last";
  http.begin(serverPath.c_str());
  return deserializeActuatorsStatus2(http.GET());
}

bool flag1 = true;
bool flag2 = true;
bool GetLastActuatorStatusBinary()
{
  describe("GET actuator from deviceID");
  String serverPath = serverName + "api/actuator_states/" + "1" + "/last";
  http.begin(serverPath.c_str());
  return deserializeActuatorsStatus3(http.GET());
}

void PostActuatorState(float status, bool statusBinary, int idActuator, long timestamp)
{
  String actuator_states_body = serializeActuatorStatusBody(status, statusBinary, idActuator, timestamp);
  describe("Test POST with actuator state");
  String serverPath = serverName + "api/actuator_states";
  http.begin(serverPath.c_str());
  test_response(http.POST(actuator_states_body));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
String status;
bool FlagModoManual; // FLAG MODO MANUAL

///////////////////////// MODO MANUAL //////////////////////////
void manual()
{
  Serial.print("Estado flag modo manual: ");
  Serial.println(FlagModoManual);
  if (FlagModoManual == true)
  {
    if (GetLastActuatorState() == 1 && flag1)
    {
      Serial.println("salir");
      stepper.step(-1000);
      flag1 = false;
      if (flag1 == false)
      {
        flag2 = true;
      }
    }

    else if (GetLastActuatorState() == 0 && flag2)
    {
      Serial.println("entrar");
      stepper.step(1000);
      flag2 = false;
      if (flag2 == false)
      {
        flag1 = true;
      }
    }
  }
}

// callback a ejecutagr cuando se recibe un mensaje
// en este ejemplo, muestra por serial el mensaje recibido
void OnMqttReceived(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Received on ");
  Serial.print(topic);
  Serial.print(": ");

  String content = "";
  for (size_t i = 0; i < length; i++)
  {
    content.concat((char)payload[i]);
  }

  /////////////////////// Movimiento motor de paso ////////////////////////

  FlagModoManual = GetLastActuatorStatusBinary();
  Serial.print("Estado flag modo manual: ");
  Serial.println(FlagModoManual);

  if (FlagModoManual == false)
  {
    /////////////////// MODO AUTOMATICO /////////////////////////////
    if (content == "Salir" && GetLastActuatorState() == 0)
    {
      stepper.step(-1000);
      PostActuatorState(1.0, false, GetActuatorId(), timestamp);
      flag1 = true;
      flag2 = true;
    }

    else if (content == "Entrar" && GetLastActuatorState() == 1)
    {
      stepper.step(1000);
      PostActuatorState(0.0, false, GetActuatorId(), timestamp);
      flag1 = true;
      flag2 = true;
    }
    else
    {
      stepper.step(0);
      flag1 = true;
      flag2 = true;
    }
  }
  else
  {
    manual();
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// inicia la comunicacion MQTT
// inicia establece el servidor y el callback al recibir un mensaje
void InitMqtt()
{
  client.setServer(MQTT_BROKER_ADRESS, MQTT_PORT);
  client.setCallback(OnMqttReceived);
}

// Setup
void setup()
{
  Serial.begin(9600);
  Serial.println();
  Serial.println(F("  __  _____ ______ _____  ______ ______  ____  _____ "));
  Serial.println(F(" /_ |/ _  /|  __ /|  ____|  __ /|  ____ / __ // __  |"));
  Serial.println(F("  | | | | | |  | | |__  | |  | | |__  | |__) | |  | |"));
  Serial.println(F("  | | | | | |  | |  __| | |  | |  __| |  _  /| |  | |"));
  Serial.println(F("  | | |_| | |__| | |____| |__| | |____| | | || |__| |"));
  Serial.println(F("  |_||___/|_____/|______|_____/|______|_|  |_\\____/ "));
  Serial.print("Connecting to ");
  Serial.println(STASSID);

  /* Explicitly set the ESP32 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  InitMqtt();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup!");

  // Configure pin modes for actuators (output mode) and sensors (input mode). Pin numbers should be described by GPIO number (https://www.upesy.com/blogs/tutorials/esp32-pinout-reference-gpio-pins-ultimate-guide)
  // For ESP32 WROOM 32D https://uelectronics.com/producto/esp32-38-pines-esp-wroom-32/
  // You must find de pinout for your specific board version
  // pinMode(actuatorPin, OUTPUT);

  // pinMode(dhtPin, INPUT);

  // Init and get the time
  timeClient.begin();

  dht.setup(dhtPin, DHTesp::DHT11);
  timeClient.update();
  timestamp = timeClient.getEpochTime();
  // Inicializar valor del actuador
  PostActuatorState(0., false, 1, timestamp);
}

// conecta o reconecta al MQTT
// consigue conectar -> suscribe a topic y publica un mensaje
// no -> espera 5 segundos
void ConnectMqtt()
{
  Serial.print("Starting MQTT connection...");
  if (client.connect(MQTT_CLIENT_NAME))
  {
    client.subscribe("10dedero");
    client.publish("10dedero", "Bienvenido");
  }
  else
  {
    Serial.print("Failed MQTT connection, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");

    delay(5000);
  }
}

// gestiona la comunicación MQTT
// comprueba que el cliente está conectado
// no -> intenta reconectar
// si -> llama al MQTT loop
void HandleMqtt()
{
  if (!client.connected())
  {
    ConnectMqtt();
  }
  client.loop();
}
int i = 0;
///////////////////////////////////////////////////////////////// Run the tests! //////////////////////////////////////////////////////////////////
void loop()
{

  // Update current time using NTP protocol
  timeClient.update();

  // Obtén el tiempo actual en formato de timestamp
  timestamp = timeClient.getEpochTime();

  // Convierte el timestamp a una cadena de caracteres
  String timestampString = String(timestamp);

  // Espera 1 segundo antes de actualizar el tiempo nuevamente [si no se repiten valores]
  delay(1000);

  // Depending on the current second (even or odd), write in digital actuator pin HIGH or LOW value
  // if (timeClient.getSeconds() % 2 == 1)
  // {
  //   digitalWrite(actuatorPin, HIGH);
  //   Serial.println("ON");
  // }
  // else
  // {
  //   digitalWrite(actuatorPin, LOW);
  //   Serial.println("OFF");
  // }

  // Reads digital sensor value and print ON or OFF by serial monitor depending on the sensor status (binary)
  int digitalValue = digitalRead(dhtPin);

  if (digitalValue == HIGH)
  {
    if (i == 5)
    {
      float temp = dht.getTempAndHumidity().temperature;
      PostTemperatura(GetSensorId(), timestamp, temp);
      i = 0;
    }
  }
  else
  {
    Serial.println("Digital sensor value : OFF");
  }

  i++;

  HandleMqtt();
}