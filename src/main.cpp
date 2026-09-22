#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define NEO_PIN 45
#define LED_COUNT 1 


void neo_blinky(void *pvParameters){

    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    // Set initially to green
    strip.setPixelColor(0, strip.Color(0, 255, 0));
    strip.show();
    vTaskDelay(500);

    while(1) {                          
        // Red
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        strip.show();
        vTaskDelay(500);

        // Green
        strip.setPixelColor(0, strip.Color(0, 255, 0));
        strip.show();
        vTaskDelay(500);

        // Blue
        strip.setPixelColor(0, strip.Color(0, 0, 255));
        strip.show();
        vTaskDelay(500);
    }
}

void neo_rainbow(void *pvParameters) {
    Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.show();

    uint32_t colors[7] = {
        strip.Color(255, 0, 0),     // Đỏ
        strip.Color(255, 127, 0),   // Cam
        strip.Color(255, 255, 0),   // Vàng
        strip.Color(0, 255, 0),     // Lục
        strip.Color(0, 0, 255),     // Lam
        strip.Color(75, 0, 130),    // Chàm
        strip.Color(148, 0, 211)    // Tím
    };

    int colorIndex = 0;

    while(1) {
        strip.setPixelColor(0, colors[colorIndex]);
        strip.show();
        
        colorIndex++;
        if (colorIndex >= 7) colorIndex = 0;
        
        vTaskDelay(500); // Đổi màu mỗi 500ms
    }
}

void led_blinky(void *pvParameters) {
  pinMode(GPIO_NUM_48, OUTPUT); // Initialize LED pin

  while(1) {
    digitalWrite(GPIO_NUM_48, HIGH); // Turn ON LED
    vTaskDelay(500);
    digitalWrite(GPIO_NUM_48, LOW); // Turn OFF LED
    vTaskDelay(500);
  }
}

void serial_counter_task(void *pvParameters) {
  int counter = 0;
  while(1) {
    Serial.print("Counter: ");
    Serial.println(counter++);
    vTaskDelay(1000); // 1 second
  }
}

void setup() {
  // put your setup code here, to run once for task creation
  Serial.begin(115200);
  xTaskCreate(led_blinky, "LED Control", 2048, NULL, 2, NULL);
  // xTaskCreate(neo_blinky, "NEO Control", 2048, NULL, 2, NULL); 
  xTaskCreate(neo_rainbow, "NEO Rainbow", 2048, NULL, 2, NULL);
  xTaskCreate(serial_counter_task, "Counter Task", 2048, NULL, 1, NULL);
}

void loop() {
  // Serial.println("Hello Custom Board");
  // delay(1000);
}