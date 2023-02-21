#include <WiFiClientSecure.h>#include <time.h>#include <SPI.h>#include <Wire.h>#include <Adafruit_GFX.h>#include <Adafruit_SH110X.h>#define i2c_Address 0x3c#define SCREEN_WIDTH 128 // OLED display width, in pixels#define SCREEN_HEIGHT 64 // OLED display height, in pixels#define OLED_RESET -1   //   QT-PY / XIAOAdafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
#define JST 3600*9const char* ssid = "atsutoのiphone"; //ルーターのSSIDconst char* password = "2dw71daaebhcv"; //ルーターのパスワードconst int port = 443;
int i=0;
String code_data[2];
uint16_t today_weather_code;
uint16_t tomorrow_weather_code;
const char* jp_weather_host = "www.jma.go.jp";
String office_code= "170000"; //isikawaString jp_weather_area_url = "/bosai/forecast/data/forecast/" + office_code + ".json";
String area_code_str = "170010"; //ishikawa areaString search_tag =  "code\":\"" + area_code_str + "\"},\"weatherCodes\":[\"";
WiFiClientSecure client;
void setup() {
  Serial.begin(9600);
  delay(250);
  Serial.println("Serial begin");
  display.begin(i2c_Address, true);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.printf("Connecting To Internet...");
  Serial.println("Connecting To Internet...");
  display.display();
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.printf("Connected, IP address: ");
  Serial.println(WiFi.localIP());
  //client.setCACert(jp_weather_root_ca);  client.setInsecure();
  configTime(JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
}
void loop() {
  display.clearDisplay();
  display.setTextSize(2.0);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  if( i%60==0 || i==0){
    Serial.println("getting weather data");
    display.printf("getting weather data");
    String ret_str = get_weather();
    String weather_code_from_key = "weatherCodes\":[\"";
    uint8_t from1 = ret_str.indexOf(weather_code_from_key, 0) + weather_code_from_key.length();
    uint8_t to1 = from1 + 3;
    uint8_t from2 = to1 + 3;
    uint8_t to2 = from2 + 3;
    String today_w_code_str = ret_str.substring(from1, to1);
    String tomorrow_w_code_str = ret_str.substring(from2, to2);
    today_weather_code = atoi(today_w_code_str.c_str());
    tomorrow_weather_code = atoi(tomorrow_w_code_str.c_str());
    display.clearDisplay();
  }
  String today_weather = classifyWeatherCode(today_weather_code);
  String tomorrow_weather = classifyWeatherCode(tomorrow_weather_code);
  time_t t;
  struct tm *tm;
  static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
  t = time(NULL);
  tm = localtime(&t);
  Serial.printf("i:%d",i);
  Serial.println();
  Serial.printf("Td:%s\nTm:%s",today_weather.c_str(),tomorrow_weather.c_str());
  Serial.println();
  Serial.printf(" %04d/%02d/%02d(%s) %02d:%02d:%02d\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);
  Serial.println("-----------");
  display.printf("%s\n",today_weather.c_str());
  display.printf("%04d/%02d/%02d\n(%s)\n%02d:%02d:%02d\n",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);
  display.setTextColor(SH110X_BLACK, SH110X_WHITE); // 'inverted' text  display.display();
  delay(1000);
  i++;
}
String get_weather(){
  uint32_t time_out = millis();
  while(true){
    if (client.connect(jp_weather_host, port)){
      Serial.print(jp_weather_host); Serial.print("-------------");
      Serial.println("connected");
      Serial.println("-------Send HTTPS GET Request");
      String req_header_str = String("GET ");
            req_header_str += String(jp_weather_area_url) + " HTTP/1.1\r\n";
            req_header_str += "Host: ";
            req_header_str += String(jp_weather_host) + "\r\n";
            req_header_str += "User-Agent: BuildFailureDetectorESP32\r\n";
            req_header_str += "Accept: text/html,application/xhtml+xml,application/xml\r\n";
            req_header_str += "Connection: close\r\n\r\n";
      client.print(req_header_str);
      break;
    }
    if((millis() - time_out ) > 20000){
      Serial.println("time out!");
      Serial.println("Host connection failed.");
    }
    delay(1);
  }
  String ret_str;
  time_out = millis();
  if(client){
    String tmp_str;
    Serial.println("-------Receive HTTPS Response");
    if(client.connected()){
      while(true) {
        if((millis() - time_out ) > 60000){
          Serial.println("time out!");
          Serial.println("Host HTTPS response failed.");
          break;
        }
        tmp_str = client.readStringUntil(']');
        //Serial.println(tmp_str);        if(tmp_str.indexOf(search_tag) >= 0 ){
          ret_str += tmp_str;
          ret_str += "] ";
          break;
        }
        delay(1);
      }
      while(client.available()){
        if((millis() - time_out ) > 60000) break; //60seconds Time Out        client.read();
        delay(1);
      }
      delay(10);
      client.stop();
      delay(10);
      Serial.println("-------Client Stop");
    }
  }
  if(ret_str.length() < 20) ret_str = "could not get json";
  if(client){
    delay(10);
    client.stop();
    delay(10);
    Serial.println("-------Client Stop");
  }
  Serial.print("char:");
  Serial.println(ret_str);
  return ret_str;
}
String classifyWeatherCode(uint16_t weather_code){
  switch(weather_code){
    //--------Clear（晴れ）-----------------    case 100:
    case 123:
    case 124:
    case 130:
    case 131:
      // Serial.println("sunny");      return "Sunny";
      break;
    //--------晴れ時々（一時）曇り----------------    case 101:
    case 132:
      // Serial.println("sunny partly cloudy");      return "Sun (C)";
      break;
    //--------晴れ時々（一時）雨----------------    case 102:
    case 103:
    case 106:
    case 107:
    case 108:
    case 120:
    case 121:
    case 140:
      // Serial.println("sunny partly rain");      return "Sun (R)";
      break;
    //--------晴れ時々（一時）雪----------------    case 104:
    case 105:
    case 160:
    case 170:
      // Serial.println("sunny partly snow");      return "Sun (Sno)";
      break;
    //--------晴れ後曇り----------------    case 110:
    case 111:
      // Serial.println("sunny then cloudy");      return "Sun -> C";
      break;
    //--------晴れ後雨----------------    case 112:
    case 113:
    case 114:
    case 118:
    case 119:
    case 122:
    case 125:
    case 126:
    case 127:
    case 128:
      // Serial.println("sunny then rain");      return "Sun -> R";
      break;
    //--------晴れ後雪----------------    case 115:
    case 116:
    case 117:
    case 181:
      // Serial.println("sunny then snow");      return "Sun -> Sno";
      break;
    //--------曇り-----------------    case 200:
    case 209:
    case 231:
      // Serial.println("cloudy");      return "Cloudy";
      break;
    //--------曇り時々晴れ-----------------    case 201:
    case 223:
      // Serial.println("cloudy partly sunny");      return "C (Sun)";
      break;
    //--------曇り時々雨-----------------    case 202:
    case 203:
    case 206:
    case 207:
    case 208:
    case 220:
    case 221:
    case 240:
      // Serial.println("cloudy partly rain");      return "C (R)";
      break;
    //--------曇り一時雪-----------------    case 204:
    case 205:
    case 250:
    case 260:
    case 270:
      // Serial.println("partly cloudy snow");      return "C (Sno)";
      break;
    //--------曇り後晴れ-----------------    case 210:
    case 211:
      // Serial.println("cloudy then sunny");      return "C -> Sun";
      break;
    //--------曇り後雨-----------------    case 212:
    case 213:
    case 214:
    case 218:
    case 219:
    case 222:
    case 224:
    case 225:
    case 226:
      // Serial.println("cloudy then rain");      return "C -> R";
      break;
    //--------曇り後雪-----------------    case 215:
    case 216:
    case 217:
    case 228:
    case 229:
    case 230:
    case 281:
      // Serial.println("cloudy then snow");      return "C -> Sno";
      break;
    //--------雨-----------------    case 300:
    case 304:
    case 306:
    case 328:
    case 329:
    case 350:
      // Serial.println("rain");      return "Rain";
      break;
    //--------雨時々晴れ-----------------    case 301:
      // Serial.println("rain partly sunny");      return "R (Sun)";
      break;
    //--------雨時々曇り-----------------    case 302:
      // Serial.println("rain partly cloudy");      return "R (C)";
      break;
    //--------雨時々雪-----------------    case 303:
    case 309:
    case 322:
      // Serial.println("rain partly snow");      return "R (Sno)";
      break;
    //--------暴風雨-----------------    case 308:
      // Serial.println("storm");      return "STORM";
      break;
    //--------雨後晴れ-----------------    case 311:
    case 316:
    case 320:
    case 323:
    case 324:
    case 325:
      // Serial.println("rain then sunny");      return "R -> Sun";
      break;
    //--------雨後曇り-----------------    case 313:
    case 317:
    case 321:
      // Serial.println("rain then cloudy");      return "R -> C";
      break;
    //--------雨後雪-----------------    case 314:
    case 315:
    case 326:
    case 327:
      // Serial.println("rain then snow");      return "R -> Sno";
      break;
    //--------雪-----------------    case 340:
    case 400:
    case 405:
    case 425:
    case 426:
    case 427:
    case 450:
      // Serial.println("snow");      return "Snow";
      break;
    //--------雪時々晴れ-----------------    case 401:
      // Serial.println("snow partly sunny");      return "Sno (Sun)";
      break;
    //--------雪時々曇り-----------------    case 402:
      // Serial.println("snow partly cloudy");      return "Sno (C)";
      break;
    //--------雪時々雨-----------------    case 403:
    case 409:
      // Serial.println("snow partly rain");      return "Sno (R)";
      break;
    //--------暴風雪-----------------    case 406:
    case 407:
      // Serial.println("blizzard");      return "BLIZZARD";
      break;
    //--------雪後晴れ-----------------    case 361:
    case 411:
    case 420:
      // Serial.println("snow then sunny");      return "Sno -> Sun";
      break;
    //--------雪後曇り-----------------    case 371:
    case 413:
    case 421:
      // Serial.println("snow then cloudy");      return "Sno -> C";
      break;
    //--------雪後雨-----------------    case 414:
    case 422:
    case 423:
      // Serial.println("snow then rain");      return "Sno -> R";
      break;
    default:
      return "no data";
      break;
  }
}