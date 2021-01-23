#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include "AsyncJson.h"
#include "ArduinoJson.h"
extern "C" {
#include "dl_lib_matrix3d.h"
}
#include <datamodel.h>
#include <waterreporter.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "credentials.h"

#define PREF_CAPTURE_X "capture_x"
#define PREF_CAPTURE_Y "capture_y"
#define PREF_CAPTURE_SIZE "capture_size"
#define PREF_FILTER_START "filter_start"
#define PREF_FILTER_LENGTH "filter_length"


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
#include "camera_pins.h"


Preferences prefs;

data_model_t datamodel;

// To be able to send dynamic image data in webserver responses, this image pointer is used.
// The data is freed before each response.
dl_matrix3du_t * image; 
AsyncWebServer server(80);

WaterReporter reporter(IOT_ID);
TaskHandle_t Task1;

void handleRoot();
void handleStop();
void handleStart();
void handleStatus();
void handleCalibrate();
void handleReset();
void run(void * params);
dl_matrix3du_t * get_image(int x, int y, int w, byte type = 0);
int find_angle(dl_matrix3du_t *s_matrix, int start, int length, bool paint = false);

void reload_config(){
  prefs.begin("settings", true);
  datamodel.settings.capture_img_x = prefs.getInt(PREF_CAPTURE_X, 104);
  datamodel.settings.capture_img_y = prefs.getInt(PREF_CAPTURE_Y, 55);
  datamodel.settings.capture_img_size = prefs.getInt(PREF_CAPTURE_SIZE, 150);
  datamodel.settings.filter_start = prefs.getInt(PREF_FILTER_START, 30);
  datamodel.settings.filter_length = prefs.getInt(PREF_FILTER_LENGTH, 40);
  prefs.end();
}

int set_config_parameter(const char * parameter, int value) {
  
  if(strcmp(parameter, PREF_CAPTURE_X) ||
     strcmp(parameter, PREF_CAPTURE_Y)||
     strcmp(parameter, PREF_CAPTURE_SIZE)||
     strcmp(parameter, PREF_FILTER_START)||
     strcmp(parameter, PREF_FILTER_LENGTH)) {
    bool success = false;
    prefs.begin("settings", false);
    success = prefs.putInt(parameter, value) == 4;
    prefs.end();
    reload_config();
    return success ? 0 : 1;
  }
  return 2;
}

String config_paramaters_string() {
  String s = String();
  s += "\ncapture_img_size: " + String(datamodel.settings.capture_img_size);
  s += "\ncapture_img_x:" + String(datamodel.settings.capture_img_x);
  s += "\ncapture_img_y: " + String(datamodel.settings.capture_img_y);
  s += "\nfilter_length: " + String(datamodel.settings.filter_length);
  s += "\nfilter_start: " + String(datamodel.settings.filter_start);
  return s;
}

void configureWebServer() {
  server.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) { 
    vTaskSuspend( Task1 );
    //datamodel.state.enabled = false;
    request->send(200, "text/plain", "OK");
  });
  server.on("/resume", HTTP_GET, [](AsyncWebServerRequest *request) { 
    vTaskResume( Task1 );
    //datamodel.state.enabled = false;
    request->send(200, "text/plain", "OK");
  });
  
  server.on("^\\/image\\/([0-9])$", HTTP_GET, [] (AsyncWebServerRequest *request) {
    vTaskSuspend( Task1 );
    //datamodel.state.enabled = false;
    delay(500);
    int image_type = request->pathArg(0).toInt();
    dl_matrix3du_free(image);    
    image = get_image(datamodel.settings.capture_img_x,datamodel.settings.capture_img_y,datamodel.settings.capture_img_size, image_type);

    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/octet-stream", image->item, image->h * image->w * image->c);
    response->addHeader("w", String(image->w));
    response->addHeader("h", String(image->h));
    response->addHeader("c", String(image->c));
    request->send(response);
  });
  
  server.on("/calibrate", HTTP_GET, [](AsyncWebServerRequest *request) {
    vTaskSuspend( Task1 );
    delay(500);
    
    Serial.println("HandleCalibrate");
    dl_matrix3du_free(image);    
    image = get_image(datamodel.settings.capture_img_x,datamodel.settings.capture_img_y,datamodel.settings.capture_img_size, 0);
    int angle = find_angle(image, datamodel.settings.filter_start, datamodel.settings.filter_length, true);
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/octet-stream", image->item, image->h * image->w);
    response->addHeader("Angle", String(angle));
    request->send(response);
  });
  
  server.on("/configuration", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<100> data;
    data[PREF_FILTER_START] = datamodel.settings.filter_start;
    data[PREF_FILTER_LENGTH] = datamodel.settings.filter_length;
    data[PREF_CAPTURE_X] = datamodel.settings.capture_img_x;
    data[PREF_CAPTURE_Y] = datamodel.settings.capture_img_y;
    data[PREF_CAPTURE_SIZE] = datamodel.settings.capture_img_size;
    String response;
    serializeJson(data, response);
    request->send(200, "application/json", response);
  });
  
  server.on("^\\/set\\/([a-z_]+)/([0-9]+)$", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String parameter = request->pathArg(0);
        int value = request->pathArg(1).toInt();
        String content = "Set parameter\n------------\n";
        int result = set_config_parameter(parameter.c_str(), value);
        content += (result != 0) ? String("Error: ") + String(result) : "Success";
        content += "\n\nParameter: " + parameter + "\n";
        content += "Value: " + String(value) + "\n";
        
        
        content += "\nCurrent settings\n--------------";
        content += config_paramaters_string();
        request->send(200, "text/plain", content);
    });
  
  server.begin();
}

void setup() {
  reload_config();
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  Serial.println();

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
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.fb_count = 1;

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

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  ArduinoOTA
    .onStart([]() {
      vTaskSuspend( Task1 );
      delay(500);
      //datamodel.state.enabled = false;
      
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
    
      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  if(!MDNS.begin(IOT_ID)) {
      Serial.println("Error starting mDNS");
      return;
  }
  Serial.print("Camera Ready! IP:");
  Serial.print(WiFi.localIP());

  reporter
    .SetBrokerUrl(MQTT_broker)
    .SetUSer(MQTT_user)
    .SetPass(MQTT_password)
    .SetTopic(WATER_TOPIC)
    .SetId("waterthingy");

  reporter.connect();

  configureWebServer();

  xTaskCreatePinnedToCore(
      run, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      &datamodel,  /* Task input parameter */
      10,  /* Priority of the task */
      &Task1,  /* Task handle. */
      1); /* Core where the task should run */
  datamodel.state.handled = true;
  datamodel.state.enabled = true;
}

long last_send_tick = millis();

void loop() {
  
  if(WiFi.status() != WL_CONNECTED)
  {
    ESP.restart();
  }
  reporter.reconnectingLoop();
  ArduinoOTA.handle();
  
  if(datamodel.state.enabled) {
    if (!datamodel.state.handled || millis() - last_send_tick > 5000) {
      last_send_tick = millis();
      reporter.Report(&datamodel);
      datamodel.state.handled = true;
    }
  }
}

dl_matrix3du_t * get_image(int x, int y, int w, byte type) {
  camera_fb_t *fb = esp_camera_fb_get();
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);
  esp_camera_fb_return(fb);

  if (type == 1){
    // paint sub-image square
    for (int i = x; i < x+w; i++) {
      image_matrix->item[(y * image_matrix->w + i) * 3 + 1] = 250;
      image_matrix->item[((y + w) * image_matrix->w + i) * 3 + 1] = 250;
    }
    for (int i = y; i < y+w; i++) {
      image_matrix->item[(i * image_matrix->w + x) * 3 + 1] = 250;
      image_matrix->item[(i * image_matrix->w + x + w) * 3 + 1] = 250;
    }
    return image_matrix;
  }
  
  if (type == 2) {
    // crop image and return RGB
    dl_matrix3du_t *sub_m = dl_matrix3du_alloc(1, w, w, 3);
    dl_matrix3du_slice_copy(sub_m, image_matrix, x,y, w, w);
    dl_matrix3du_free(image_matrix);  
    return sub_m;
  }
  
  dl_matrix3du_t *s_m = dl_matrix3du_alloc(1, w, w, 1);
  // Calculate saturation for each pixel in the croped image  
  for (int i = 0; i < w; i++) {
    for(int j = 0; j < w; j++) {
      int index = ((y +i) * image_matrix->w + x + j + image_matrix->w) * 3;
      unsigned char r = image_matrix->item[index];
      unsigned char g = image_matrix->item[index + 1];
      unsigned char b = image_matrix->item[index + 2];
      
      double max = r;
      double min = r;
      if (g > max)
        max = g;
      if (g < min)
        min = g;
      if (b > max)
        max = b;
      if (b < min)
        min = b;
      
      max /= 255;
      min /= 255;

      double l = (min + max) / 2.0;
      double s = 0;
      if (min == max)
        s = 0;
      else if(l < .5)
        s = (max - min) / (max + min);
      else
        s = (max - min)/(2 - max - min);
      s_m->item[i * w + j] = s * 255;
    }
  }
  dl_matrix3du_free(image_matrix);
  return s_m;
}

double to_radians(double angle) {
  return angle*PI/180.0;
}

int find_angle(dl_matrix3du_t *s_matrix, int start, int length, bool paint) {
  if (start + length > s_matrix->w / 2)
    return -1;
  int deg_at_max = -1;
  int filter_max = 0;
  int center = s_matrix->w / 2;
  
  for (int deg = 0; deg < 360; deg++) {
    double radians = to_radians(deg);
    int sum = 0;
    double cos_rad = cos(radians);
    double sin_rad = sin(radians);
    for (int r = start; r < start + length; r++) {
      int x = r * cos_rad + center;
      int y = r * sin_rad + center;
      sum += s_matrix->item[y*s_matrix->w + x];
    }
    if (sum > filter_max) {
      filter_max = sum;
      deg_at_max = deg;
    }
  }

  if (paint) {
    Serial.println("painting");
    double a = to_radians(deg_at_max);
    double dx = cos(a);
    double dy = sin(a);
    double x = center + start * dx;
    double y = center + start * dy;
    
    for (int i = 0; i < length; i++)
    {
      s_matrix->item[(int)y* s_matrix->w + (int)x] = 20;
      x += dx;
      y += dy;
    }
  }
  return deg_at_max;
}

void run( void * params ) {
  
  data_model_t* a = (data_model_t *) params;
  int angle;
  int angle_diff;
  double liter_per_deg = 0.002659752;
  int x = a->settings.capture_img_x;
  int y = a->settings.capture_img_y;
  int w = a->settings.capture_img_size;
  int fs = a->settings.filter_start;
  int fl = a->settings.filter_length;

  for(;;) {
    if (!a->state.enabled) {
      delay(100);
      continue;
    }
    long tt = millis();
    dl_matrix3du_t *s_matrix = get_image(x, y, w, 0);
    a->state.times[0] = millis() - tt;
    angle = find_angle(s_matrix, fs, fl);
    dl_matrix3du_free(s_matrix);
    a->state.times[1] = millis() - tt;

    if (angle < 0){
      // something went wrong... 
      continue;
    }

    long t = millis();
    if (t - a->state.measurement_time_ms > 3000) {
      // re-starting by setting last angle to current angle.
      a->state.angle = angle;
    }
    
    angle_diff = angle - a->state.angle;
    if (angle_diff < -20) {
      // passed 0 deg.
      angle_diff += 360;
    }
    
    bool newData = false;
    if (angle_diff > 0) {
      double c = angle_diff * liter_per_deg;
      a->state.angle = angle;
      a->state.angle_diff = angle_diff;
      a->state.acc_angle += angle_diff;
      a->state.consumption = c;
      a->state.acc_consumption += c;
      newData = true;
    }
    else if(a->state.angle_diff != 0) {
      a->state.angle_diff = 0;
      a->state.consumption = 0;
      newData = true;
    }
    
    a->state.timediff_ms = t - a->state.measurement_time_ms;
    a->state.measurement_time_ms = t;
    if (newData)
      a->state.handled = false;
  }
}
