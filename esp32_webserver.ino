
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_GFX.h>     // Core graphics library
#include <Adafruit_ST7789.h>  // Hardware-specific library for ST7789
const char* ssid = "YOUR WIFI SSID";// aqui deberas incluir tu ssid 
const char* password = "YOUR WIFI PASSWORD";//aqui va tu contrasena 

int canvasBack = 0x0000;//asignamos un color para el display 
int canvasFont = 0xFFFF;// asignamos color para el texto 
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
GFXcanvas1 canvas(220, 40);
GFXcanvas1 canvas1(220, 40);
Adafruit_AHTX0 aht;

//aqui declaramos variables para el servidor 
String hString;
String tempString;
String inputMessage = "25.0";
String lastTemperature;
String enableArmChecked = "checked";
String inputMessage2 = "true";
String inputMessage3 = "2.0";
String lastHumidity;
//aqui debajo construimos nuestro webserver en HTML
const char index_html[] PROGMEM = R"rawliteral(  
<!DOCTYPE HTML><html><head>
  <title>Thermohigrometro con Adafruit ESP32 feather TFT </title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h2>AM2315C </h2> 
  <h3>%TEMPERATURE% &deg;C</h3>
  <h3>%HUMIDITY% &percnt;Relative Humidity</h3>
  <h2>SETTINGS</h2>
  <form action="/get">
    SET POINT <input type="number" step="0.1" name="threshold_input" value="%THRESHOLD%" required><br>
    HYSTERESIS <input type="number" step="0.1" name="hyst" value="%hyst%" required><br>
    AUTO <input type="checkbox" name="enable_arm_input" value="true" %ENABLE_ARM_INPUT%><br><br>
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest* request) {
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);

String processor(const String& var) {
  //Serial.println(var);
  if (var == "TEMPERATURE") {
    return lastTemperature;
  } else if (var == "THRESHOLD") {
    return inputMessage;
  } else if (var == "hyst") {
    return inputMessage3;
  } else if (var == "HUMIDITY") {
    return lastHumidity;
  } else if (var == "ENABLE_ARM_INPUT") {
    return enableArmChecked;
  }
  return String();
}

bool triggerActive = false;
const char* PARAM_INPUT_1 = "threshold_input";
const char* PARAM_INPUT_2 = "enable_arm_input";
const char* PARAM_INPUT_3 = "hyst";
unsigned long previousMillis = 0;
const long interval = 10000;
const int on = 13;



void setup() {
// Iniciamos todo, paso a paso. 
  Serial.begin(115200);
  aht.begin();
  WiFi.mode(WIFI_STA);
  // estos pines son asignados al display
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);

  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
//por defecto, el relay estara encendido hasta que se inicie el servidor, esto pueden cambiarlo segun la aplicacion 
  pinMode(on, OUTPUT);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest* request) {

    if (request->hasParam(PARAM_INPUT_1)) {//convertir el valor que asignamos en el cuadro de dialogo de la pagina web, en parametros usados por el programa
      inputMessage = request->getParam(PARAM_INPUT_1)->value(); // parametro = temperatura 
      inputMessage3 = request->getParam(PARAM_INPUT_3)->value();//parametro = humedad

      if (request->hasParam(PARAM_INPUT_2)) {
        inputMessage2 = request->getParam(PARAM_INPUT_2)->value(); // parametro = palomita en el recuadro (si  o no == true or false)
        enableArmChecked = "checked";
      }

      else {
        inputMessage2 = "false";
        enableArmChecked = "";
      }
    }



    Serial.println(inputMessage);
    Serial.println(inputMessage2);
    Serial.println(inputMessage3);
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");// el texto que podran leer al 
    //cambiar los parametros de los cuadros de dialogo 
  });
  server.onNotFound(notFound);
  server.begin();//iniciamos el servidor 
  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(0x0000);
  tft.setCursor(0, 2);
  tft.setTextColor(0x07FF);
  tft.setTextSize(1);
  tft.setTextWrap(true);
  tft.print(WiFi.localIP());//obtenemos la direccion ip local http 192.168.0.1 por ejemplo y la ponemos en el display para poder conectarnos 
  tft.setCursor(48, 30);
  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.print("Hello World");//El texto que se muestra en el display, este no cambiara 
  canvas.setTextWrap(false);
  canvas1.setTextWrap(false);
}

void loop() {
  sensors_event_t humidity, temp; //asignamos parametros para la lectura de nuestro sensor
  float temperature = temp.temperature; //convertimos la lecturas del sensor a float 
  float h = humidity.relative_humidity;
  aht.getEvent(&humidity, &temp);//pedimos al sensor que nos de la lectura.

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;



    Serial.print(temperature);
    Serial.println(" degrees C");
    Serial.print(h);
    Serial.println("% rH");


    canvas.fillScreen(0);
    canvas.setCursor(1, 5);
    canvas.setTextSize(2);
    tempString = String("Temperature:") + String(temperature) + String("C");
    canvas.print(tempString);
    tft.drawBitmap(1, 60, canvas.getBuffer(),
                   canvas.width(), canvas.height(), canvasFont, canvasBack);
                   canvas.fillScreen(0);
    canvas1.fillScreen(0);               
    canvas1.setCursor(1, 5);
    canvas1.setTextSize(2);
    hString = String("Humidity:") + String(h) + String("%RH");
    canvas1.print(hString);
    tft.drawBitmap(1, 100, canvas1.getBuffer(),
                   canvas1.width(), canvas1.height(), canvasFont, canvasBack);



    lastTemperature = temperature;
    lastHumidity = h;

    // medir temperatura, encender o apagar el relevador segun sea necesario
    if (temperature > inputMessage.toFloat() + inputMessage3.toFloat() && inputMessage2 == "true" && triggerActive) {
      String message = String("La temperatura es mayor al parametro requerido, temperatura es: ") + String(temperature) + String("C");
      Serial.println(message);
      triggerActive = false;
      digitalWrite(on, LOW);

    }
    
    else if (temperature < inputMessage.toFloat() - inputMessage3.toFloat() && inputMessage2 == "true" && !triggerActive) {
      String message = String("La temperatura es menor al parametro requerido, temperatura es: ") + String(temperature) + String(" C");
      Serial.println(message);
      triggerActive = true;
      digitalWrite(on, HIGH);
    }
  }
}
