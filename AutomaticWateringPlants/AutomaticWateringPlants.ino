#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <DHT_U.h>
// 溫濕度角位設定
#define dhtPin D1 
#define dhtType DHT11 //類型是 DHT11
static float now_Humidity = 0;
static float now_Temperature = 0; 
// 土壤溼度角位
#define GWetPin A0
static int now_GroundHumidity = 0;
// 蜂嗚器角位
#define BiBiPin D2 
// 澆水器角位
#define WaterPin D3
DHT dht(dhtPin, dhtType);
ESP8266WiFiMulti WiFiMulti;

//增加保護
//每一小時最多澆3次水
//一小時有120次30秒，超過就歸零
static int watering_times=0; // max 3
static int counts = 0; //計算幾次
static String plants_name = "3WA_Plants";
//資料傳輸
static HTTPClient http;
static String URL = "http://3wa.tw/my_plants/api.php?mode=setData";
void setup() {
    Serial.begin(115200);
   // Serial.setDebugOutput(true);
    Serial.println();
    Serial.println();
    Serial.println();
    for(uint8_t t = 3; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(500);
    }
    pinMode(BiBiPin,OUTPUT); //蜂嗚角要設輸出
    pinMode(GWetPin,INPUT); //土壤接角要設讀取
    digitalWrite(BiBiPin,LOW);
    pinMode(WaterPin,OUTPUT); //澆水器要設輸出
    now_Humidity = 0;
    now_Temperature = 0;
    now_GroundHumidity = 0;
    dht.begin(); //啟動 dht
    WiFiMulti.addAP("3wa_hinet", "0919566444");
}
void getDht()
{ 
  //取得環境濕度、溫度
  now_Humidity = dht.readHumidity(); //現在濕度
  now_Temperature = dht.readTemperature(); //現在溫度(攝氏) 
  Serial.print("濕度 Humidity: ");
  Serial.print(now_Humidity);
  Serial.print("\n");
  Serial.print("溫度 Temperature: ");
  Serial.print(now_Temperature);
  Serial.print("\n");
}
void getGroundHumidity()
{
  // 低於700 土很溼，數值越小，越溼，剛淋完水的土，也許會低到250左右
  // 土壤溼度適中 450~900 之間
  // 高於1000 土偏乾
  now_GroundHumidity = analogRead(GWetPin);
  Serial.print("土壤濕度 Ground Humidity: ");
  Serial.print(now_GroundHumidity);
  Serial.print("\n");
}
void playWater()
{
  //嗶三秒
  digitalWrite(BiBiPin,HIGH);
  delay(3000); //等3秒
  digitalWrite(BiBiPin,LOW);
  //然後打水1.5秒
  digitalWrite(WaterPin,HIGH);
  delay(1500); //等1.5秒
  digitalWrite(WaterPin,LOW);
}
void loop() {
    // wait for WiFi connection
    //停止澆水
    digitalWrite(WaterPin,LOW);
    //取得溫濕度計
    getDht();
    //取得土壤濕度
    getGroundHumidity();
    //如果太乾，> 900，蜂嗚器嗶個3秒，提示準備打水
    if( now_GroundHumidity > 900 && watering_times < 3 )
    {
      playWater();
      watering_times = watering_times + 1; //澆了一次水
      if((WiFiMulti.run() == WL_CONNECTED)) 
      {
        //有網路，回報澆水
        String data = "&plants_name="+plants_name+"&humidity="+String(now_Humidity)+"&temperature="+String(now_Temperature)+"&groundhumidity="+String(now_GroundHumidity)+"&is_watering=1";        
        http.begin(URL+data); //HTTP
        int httpCode = http.GET(); // 如果是 200 就對 
      }
    }
    if((WiFiMulti.run() == WL_CONNECTED)) 
    {
        //HTTPClient http;
        Serial.printf("成功連上網路...\n");
        //有網路連線
        String data = "&plants_name="+plants_name+"&humidity="+String(now_Humidity)+"&temperature="+String(now_Temperature)+"&groundhumidity="+String(now_GroundHumidity)+"&is_watering=0";
        http.begin(URL+data); //HTTP
        int httpCode = http.GET(); // 如果是 200 就對        
    }
    delay(30000); //等30秒
    counts = counts + 1;
    counts = counts % 120;
    if(counts==0)
    {
      watering_times = 0;      
    }
}

