#include <ArduinoJson.h>
#include <base64.h>
#include <json.h>

// #include <ArduinoJson.h>
// #include <ArduinoJson.hpp>

#include "esp_camera.h"
#include <WiFi.h>
//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

#include "camera_pins.h"

const char* ssid = "barong";
const char* password = "bolaliar";
const int FLASH = 4;
const size_t JSON_BUFFER_SIZE = 1024;
void startCameraServer();
camera_fb_t *fb;
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  pinMode(FLASH, OUTPUT);
  digitalWrite(FLASH, LOW);  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  digitalWrite(FLASH, HIGH);
  delay(500);
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Error mengambil gambar");
    return;
  }
  size_t jpg_buf_len = fb->len;
  uint8_t *jpg_buf = fb->buf;
  // membuat JSON document dengan ukuran yang cukup besar
  // disesuaikan dengan data yang ingin dimasukkan
  const size_t capacity = JSON_OBJECT_SIZE(1) + jpg_buf_len;
  DynamicJsonDocument doc(capacity);
  // Serial.println((const char*)jpg_buf_len);
  // mengonversi data byte array ke base64 string
  String imageStr = base64::(jpg_buf, jpg_buf_len);
  Serial.println((const char*)jpg_buf);
  Serial.println("-------");
  // menambahkan data gambar ke dalam JSON document
  doc["image"] = imageStr;
  // mengonversi JSON document ke string
  String json_str;
  serializeJson(doc, json_str);
  Serial.println(json_str);
  esp_camera_fb_return(fb);
  digitalWrite(FLASH, LOW);
  delay(3000);
}

// void loop() {
//   // Capture an image from the camera
//   fb = esp_camera_fb_get();
//   if (!fb) {
//     Serial.println("Camera capture failed");
//     return;
//   }

//   // Encode the image to base64
//   String *base64_image = base64::encode(fb->buf, fb->len);
//   Serial.printf("Image data (base64): %s\n", base64_image);

//   // Free the memory used by the image buffer and base64-encoded string
//   esp_camera_fb_return(fb);
//   free(base64_image);

//   delay(5000); // Wait 5 seconds before taking another image
// }
