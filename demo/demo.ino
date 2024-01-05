
#define BLYNK_TEMPLATE_ID "TMPL6pLyo-9q-"
#define BLYNK_TEMPLATE_NAME "ESP32 Plant Monitor"
#define BLYNK_AUTH_TOKEN "o64jcl55n49DP2CXb-V36ewp2x2IChDH"

char ssid[] = "MacOS"; 
char pass[] = "lamgicopassok"; 

//Đặt giá trị khô max và ẩm max  đo bằng cảm biến
int wetSoilVal = 940 ;  // Giá trị min khi đất ướt
int drySoilVal = 2559 ;  //giá trị max khi đất khô

//Đặt phần trăm phạm vi độ ẩm lý tưởng (%) trong đất
int moistPerLow =   20 ;  //min moisture %
int moistPerHigh =   80 ;  //max moisture %

#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>  
#include <AceButton.h>

using namespace ace_button; 

// Khai bao cac chan ket noi
#define SensorPin       34  //D34
#define DHTPin          14  //D14
#define RelayPin        25  //D25
#define wifiLed         2   //D2
#define RelayButtonPin  32  //D32
#define ModeSwitchPin   33  //D33
#define BuzzerPin       26  //D26
#define ModeLed         15  //D15

// Sử dụng cảm biến DHT11
#define DHTTYPE DHT11     // DHT 11

//Dinh nghia chan
#define VPIN_MoistPer    V1 
#define VPIN_TEMPERATURE V2
#define VPIN_HUMIDITY    V3
#define VPIN_MODE_SWITCH V4
#define VPIN_RELAY       V5

#define SCREEN_WIDTH 128 // chieu rong oled
#define SCREEN_HEIGHT 32 // chieu dai oled

// Khai báo cho màn hình SSD1306 được kết nối với I2C (chân SDA, SCL)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


int     sensorVal;
int     moisturePercentage;
bool    toggleRelay = LOW; // Ghi nhớ trạng thái chuyển đổi
bool    prevMode = true; // Khai báo biến prevMode kiểu đúng/sai
int     temperature1 = 0;
int     humidity1   = 0;
String  currMode  = "A";

char auth[] = BLYNK_AUTH_TOKEN;

ButtonConfig config1;
AceButton button1(&config1);// Cấu hình Button
ButtonConfig config2;
AceButton button2(&config2); // Cấu hình button

void handleEvent1(AceButton*, uint8_t, uint8_t);
void handleEvent2(AceButton*, uint8_t, uint8_t);

BlynkTimer timer;
DHT dht(DHTPin, DHTTYPE);

void checkBlynkStatus() { // Check mỗi lần sau 3 giây 

  bool isconnected = Blynk.connected();
  if (isconnected == false) {
    Serial.print("Blynk Not Connected ");
    digitalWrite(wifiLed, LOW);
  }
  if (isconnected == true) {
    digitalWrite(wifiLed, HIGH);
    //Serial.println("Blynk Connected");
  }
}

BLYNK_CONNECTED() 
{
  Blynk.syncVirtual(VPIN_MoistPer); // Kênh độ ẩm đất
  Blynk.syncVirtual(VPIN_RELAY); // Kênh relay
  Blynk.syncVirtual(VPIN_TEMPERATURE); // Kênh nhiệt độ phòng
  Blynk.syncVirtual(VPIN_HUMIDITY); // KKênh độ ẩm không khí
  Blynk.virtualWrite(VPIN_MODE_SWITCH, prevMode); //Kênh chế độ tự động 
}

BLYNK_WRITE(VPIN_RELAY) 
{
  if(!prevMode)
  {
    toggleRelay = param.asInt();// Chuyển đổi giá trị về dạng số nguyên
    digitalWrite(RelayPin, toggleRelay); //nếu toggleRelay =low ngắt kết nối vs ngược lại
  }
  else
  {
    Blynk.virtualWrite(VPIN_RELAY, toggleRelay); // Gửi giá trị của toggleRelay lên blynk
  }
}

BLYNK_WRITE(VPIN_MODE_SWITCH) //Xử lý mode switch
{
  if(prevMode !=  param.asInt())//Kiểm tra điều kiện
  {
    prevMode =  param.asInt();
    currMode = prevMode ? "A" : "M";
    digitalWrite(ModeLed, prevMode);// Bật led
    controlBuzzer(500);
    if(!prevMode && toggleRelay == HIGH){
      digitalWrite(RelayPin, LOW);
      toggleRelay = LOW;
      Blynk.virtualWrite(VPIN_RELAY, toggleRelay);
    }
   }   
}

void controlBuzzer(int duration)
{
  digitalWrite(BuzzerPin, HIGH);
  delay(duration);
  digitalWrite(BuzzerPin, LOW);
}

void displayData(String line1 , String line2){
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30,2);
  display.print(line1);
  display.setTextSize(1);
  display.setCursor(1,25);
  display.print(line2);
  display.display();
}

void getMoisture()
{
  sensorVal = analogRead(SensorPin);// Độc giá trị cảm biến
  Serial.println(sensorVal);
  if (sensorVal > (wetSoilVal - 100) && sensorVal < (drySoilVal + 100) )// Khoảng setup giá trị Moisture
  {
    moisturePercentage = map(sensorVal ,drySoilVal, wetSoilVal, 0, 100);//Ánh xạ giá trị cảm biến
    // Nuyên mẫu : map(value, fromLow, fromHigh, toLow, toHigh)

    // Print result to serial monitor
    Serial.print("Moisture Percentage: ");
    Serial.print(moisturePercentage);
    Serial.println(" %");
  }
  else
  {
    Serial.println(sensorVal);
  }
  delay(100);
}

void getWeather()
{
  
  float h = dht.readHumidity();
  float t = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  else 
  {
    humidity1 = int(h);
    temperature1 = int(t);
   // Serial.println(temperature1);
   // Serial.println(humidity1);
  }  
}

void sendSensor()
{ 
  getMoisture(); // get Moisture reading
  getWeather(); // get reading from DHT11
  
  displayData(String(moisturePercentage) + " %", "T:" + String(temperature1) + " C,  H:" + String(humidity1) + " %  " + currMode);// Viết lên màn hình OLed
  
  Blynk.virtualWrite(VPIN_MoistPer, moisturePercentage); // Gửi dữ liệu moisture đến Pin ảo 
  Blynk.virtualWrite(VPIN_TEMPERATURE, temperature1); //
  Blynk.virtualWrite(VPIN_HUMIDITY, humidity1); //
}

void controlMoist()
{
  if(prevMode)
  {
    if (moisturePercentage < (moistPerLow))
    {
      if (toggleRelay == LOW)// Tắt relay
      {
        controlBuzzer(500);
        digitalWrite(RelayPin, HIGH);
        toggleRelay = HIGH;
        Blynk.virtualWrite(VPIN_RELAY, toggleRelay);
        delay(1000);
      }      
    }
    if (moisturePercentage > (moistPerHigh))
    {
      if (toggleRelay == HIGH)
      {
        controlBuzzer(500);
        digitalWrite(RelayPin, LOW);
        toggleRelay = LOW;
        Blynk.virtualWrite(VPIN_RELAY, toggleRelay);
        delay(1000);
      } 
    } 
  }
  else{
    button1.check();
  }
}
 
void setup() 
{
  // Set up serial monitor
  Serial.begin(115200);
 
  // Set pinmodes for GPIOs
  pinMode(RelayPin, OUTPUT);
  pinMode(wifiLed, OUTPUT);
  pinMode(ModeLed, OUTPUT);
  pinMode(BuzzerPin, OUTPUT);

  pinMode(RelayButtonPin, INPUT_PULLUP);
  pinMode(ModeSwitchPin, INPUT_PULLUP);

  digitalWrite(wifiLed, LOW);
  digitalWrite(ModeLed, LOW);
  digitalWrite(BuzzerPin, LOW);

  dht.begin();    // Enabling DHT sensor

  config1.setEventHandler(button1Handler);
  config2.setEventHandler(button2Handler);
  
  button1.init(RelayButtonPin);// Khởi tạo Button
  button2.init(ModeSwitchPin);//Khởi tạo button
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(1000);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();

  WiFi.begin(ssid, pass);
  timer.setInterval(2000L, checkBlynkStatus); // kiểm tra xem máy chủ Blynk có được kết nối k sau 2 giây 
  timer.setInterval(3000L, sendSensor); // hiển thị và gửi kết quả cảm biến cứ sau 3 giây
  Blynk.config(auth);
  //delay(1000);
  controlBuzzer(1000); 
  digitalWrite(ModeLed, prevMode);
}
 void loop() 
 {
  Blynk.run(); // Kết vs vs Blynk
  timer.run(); // Khởi tạo SimpleTimer
  button2.check(); // Check nút nhấn 2
  controlMoist(); // Hàm controlMoist
}

void button1Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.println("EVENT1");
  switch (eventType) {
    case AceButton::kEventReleased:
      //Serial.println("kEventReleased");
      digitalWrite(RelayPin, !digitalRead(RelayPin));
      toggleRelay = digitalRead(RelayPin);
      Blynk.virtualWrite(VPIN_RELAY, toggleRelay);
      break;
  }
}

void button2Handler(AceButton* button, uint8_t eventType, uint8_t buttonState) 
//AceButton* button Con trỏ đến đối tượng AceButton gọi hàm xử lý sự kiện.
//eventType: Loại sự kiện được xảy ra (ví dụ: nhấn, nhả, giữ).
//buttonState: Trạng thái hiện tại của nút button2 (HIGH hoặc LOW).
{
  Serial.println("EVENT2");
  switch (eventType) {
    case AceButton::kEventReleased: // Button 2 nhả
      //Serial.println("kEventReleased");
      if(prevMode && toggleRelay == HIGH)
      {
        digitalWrite(RelayPin, LOW);
        toggleRelay = LOW;
        Blynk.virtualWrite(VPIN_RELAY, toggleRelay);
      }
      prevMode = !prevMode;// Flase
      currMode = prevMode ? "A" : "M";
      digitalWrite(ModeLed, prevMode);
      Blynk.virtualWrite(VPIN_MODE_SWITCH, prevMode);
      controlBuzzer(500);
      break;
  }
}
