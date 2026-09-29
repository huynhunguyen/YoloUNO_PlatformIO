#pragma once

#define LED_PIN 48
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12

#include <WiFi.h>
#include <Arduino_MQTT_Client.h>
#include <ThingsBoard.h>
#include "DHT20.h"
#include "Wire.h"
#include <ArduinoOTA.h>

constexpr char WIFI_SSID[] = "abcd";
constexpr char WIFI_PASSWORD[] = "123456789";
constexpr char TOKEN[] = "7s5pokn2se622pzn1jxu";
constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;
constexpr uint32_t MAX_MESSAGE_SIZE = 1024U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr char BLINKING_INTERVAL_ATTR[] = "blinkingInterval";
constexpr char LED_MODE_ATTR[] = "ledMode";
constexpr char LED_STATE_ATTR[] = "ledState";

volatile bool attributesChanged = false;
volatile int ledMode = 0;
volatile bool ledState = false;

constexpr uint16_t BLINKING_INTERVAL_MS_MIN = 10U;
constexpr uint16_t BLINKING_INTERVAL_MS_MAX = 60000U;
volatile uint16_t blinkingInterval = 1000U;

constexpr int16_t telemetrySendInterval = 10000U;

constexpr std::array<const char *, 2U> SHARED_ATTRIBUTES_LIST = {
  LED_STATE_ATTR,
  BLINKING_INTERVAL_ATTR
};

WiFiClient wifiClient;
Arduino_MQTT_Client mqttClient(wifiClient);
ThingsBoard tb(mqttClient, MAX_MESSAGE_SIZE);
DHT20 dht20;

// Declare Mutex to prevent race conditions during ThingsBoard calls
SemaphoreHandle_t tbMutex;

RPC_Response setLedSwitchState(const RPC_Data &data) {
    bool newState = data;
    digitalWrite(LED_PIN, newState);
    attributesChanged = true;
    return RPC_Response("setLedSwitchValue", newState);
}

const std::array<RPC_Callback, 1U> callbacks = {
  RPC_Callback{ "setLedSwitchValue", setLedSwitchState }
};

void processSharedAttributes(const Shared_Attribute_Data &data) {
  for (auto it = data.begin(); it != data.end(); ++it) {
    if (strcmp(it->key().c_str(), BLINKING_INTERVAL_ATTR) == 0) {
      const uint16_t new_interval = it->value().as<uint16_t>();
      if (new_interval >= BLINKING_INTERVAL_MS_MIN && new_interval <= BLINKING_INTERVAL_MS_MAX) {
        blinkingInterval = new_interval;
      }
    } else if (strcmp(it->key().c_str(), LED_STATE_ATTR) == 0) {
      ledState = it->value().as<bool>();
      digitalWrite(LED_PIN, ledState);
    }
  }
  attributesChanged = true;
}

const Shared_Attribute_Callback attributes_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());
const Attribute_Request_Callback attribute_shared_request_callback(&processSharedAttributes, SHARED_ATTRIBUTES_LIST.cbegin(), SHARED_ATTRIBUTES_LIST.cend());

void InitWiFi_RTOS() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    // Use vTaskDelay instead of delay to avoid blocking CPU in RTOS
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// ----------------------------------------------------
// TASK 1: WIFI MANAGEMENT, NETWORK & MQTT SYNC
// ----------------------------------------------------
void TaskThingsBoard(void *pvParameters) {
  InitWiFi_RTOS();

  while(1) {
    if (WiFi.status() != WL_CONNECTED) {
      InitWiFi_RTOS();
    }
    
    // Acquire Mutex before accessing tb object
    if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
      if (!tb.connected()) {
        if (tb.connect(THINGSBOARD_SERVER, TOKEN, THINGSBOARD_PORT)) {
          tb.sendAttributeData("macAddress", WiFi.macAddress().c_str());
          tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend());
          tb.Shared_Attributes_Subscribe(attributes_callback);
          tb.Shared_Attributes_Request(attribute_shared_request_callback);
        }
      }
      
      if (tb.connected()) {
        tb.loop();
        if (attributesChanged) {
          attributesChanged = false;
          tb.sendAttributeData(LED_STATE_ATTR, digitalRead(LED_PIN));
        }
      }
      // Release Mutex
      xSemaphoreGive(tbMutex);
    }
    
    // Delay 10ms to yield CPU to other tasks
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ----------------------------------------------------
// TASK 2: SENSOR READING AND TELEMETRY REPORTING
// ----------------------------------------------------
void TaskSensor(void *pvParameters) {
  while(1) {
    dht20.read();
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();

    if (!isnan(temperature) && !isnan(humidity)) {
      // Wait for Mutex to send network data
      if (xSemaphoreTake(tbMutex, portMAX_DELAY) == pdTRUE) {
        if (tb.connected()) {
          tb.sendTelemetryData("temperature", temperature);
          tb.sendTelemetryData("humidity", humidity);
          tb.sendAttributeData("rssi", WiFi.RSSI());
        }
        xSemaphoreGive(tbMutex);
      }
    }
    // Delay for the specified interval (e.g., 10 seconds)
    vTaskDelay(pdMS_TO_TICKS(telemetrySendInterval));
  }
}

void setup_rtos() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  pinMode(LED_PIN, OUTPUT);
  
  Wire.begin(SDA_PIN, SCL_PIN);
  dht20.begin();
  
  // Create Mutex
  tbMutex = xSemaphoreCreateMutex();
  
  // Launch 2 parallel tasks. TaskThingsBoard has higher priority (2)
  xTaskCreatePinnedToCore(TaskThingsBoard, "TB_Task", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(TaskSensor, "Sensor_Task", 4096, NULL, 1, NULL, 1);
}

void loop_rtos() {
  // Intentionally left empty as tasks handle everything in the background
  vTaskDelete(NULL);
}
