#include <UnoWiFiDevEd.h>

#define EMER_BUTTON 2
#define SAFE_BUTTON 3
#define DHT 4
#define VIBRATOR 5
#define BUZZER 6
#define TRIG 7
#define ECHO 8
#define CONNEC 13

#define Y_PIN A0
//#include <dht.h>

#define CONNECTOR "rest"
#define ADDRESS "158.108.165.223"
#define SET "/data/groupZeedUp"
#define SET3 "/data/groupZeedUpLight/Status/set/"

CiaoData data;

String watch_status = "12";
String event_code = "00";
String readfromweb = "1200";
int cons_y = 400;
int count = 0;
//sonic part
unsigned long prev_time = 0;
//dht Input;
byte temperature;
byte humidity;
bool isChange = false;
float CMP;//Distance in the past
float SCM;//Distance for Safe_Sta
float Safe_Sta = 2;
float Danger_Sta = 15;
float CM;//Distance in the present
int Many = 0; //Detec about exchance valuable
int Error = 0;
/* 10  full offline
   11  normal
   12  online
   13  warning
   14  emerge
*/

void setup() {
  Ciao.begin();
  Serial.begin(9600);
  pinMode(Y_PIN, INPUT);
  pinMode(VIBRATOR, OUTPUT);
  pinMode(SAFE_BUTTON, INPUT);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  attachInterrupt(digitalPinToInterrupt(SAFE_BUTTON), safebutton, FALLING);
  attachInterrupt(digitalPinToInterrupt(EMER_BUTTON), emergency, FALLING);
  //cons_y = analogRead(Y_PIN);
  pinMode(CONNEC, OUTPUT);
  }

void loop() {
  isChange = false;
  Serial.print("Status: ");
  Serial.println(watch_status);
  //Read status from web
  readfromweb = Read("IsOnOff");
  Serial.print(F("ReadfromStatus: "));
  Serial.println(readfromweb);
  if(watch_status == "10" || watch_status == "11" || watch_status == "12") {
    if(readfromweb == "1000") {
      watch_status = "10";
    }
    else if(readfromweb == "1200" && watch_status != "11") {
      watch_status = "12";
    }
  }
  if (watch_status != "10") {
    if (watch_status == "12") {
      //do sonic
      sonicSession();
    }
    //do accel
    accelSession();
    //do heat
    //DHTSession(); goto another board
  }
  if(isChange)
  {
    char temp[5];
    temp[0] = watch_status[0];
    temp[1] = watch_status[1];
    temp[2] = event_code[0];
    temp[3] = event_code[1];
    temp[4] = '\0';
    Write(temp);
  }
}


void safebutton() {
  unsigned long m = millis();
  if (m - prev_time < 250){}
  else { 
    if (watch_status == "13" || watch_status == "14") {
      //turn off vibrate
      //alone but safe
      if(watch_status == "13") {Safe_Sta = SCM;
          Serial.println(F("Chace Safe_Sta"));
      }
      count = 0;
      Serial.println("Change to 12");
      watch_status = "12";
    }
    else if (watch_status == "12") {
      //from alone to meet friend
      //change back to offline
      Serial.println("Change to 11");
      watch_status = "11";
      event_code = "00";
    }
    else if(watch_status == "11") {
      //leave friend
      //change back to online
      Serial.print("Change to 12");
      watch_status = "12";
      Safe_Sta=2;
      event_code = "00";
    }
    prev_time = m;
    digitalWrite(VIBRATOR, LOW);
    analogWrite(BUZZER, 0);
    isChange = true;
  }
}

void emergency() {
  watch_status = "14";
  if(event_code != "22") {
    analogWrite(BUZZER, 20);
  }
  isChange = true;
  Serial.println("Emer");
}

void accelSession() {
  int y = analogRead(Y_PIN);
  if (y > 410) {
    count = 0;
  }
  if (cons_y - y > 20  || y - cons_y > 20 ) {
    count += 1;
    if (count >= 2 && watch_status != "14")
    {
      Serial.println(F("Send 1321"));
      watch_status = "13";
      event_code = "21";
      digitalWrite(VIBRATOR, HIGH);
      analogWrite(BUZZER, 20);
      Write("1321");
    }
//  cons_y = y;
  }
  Serial.print("Y: ");
  Serial.println(y);
  delay(100);
}
/*
void ActionCriminal(){
  Write("Light/Status", "1322");
  event_code="22";
  digitalWrite(VIBRATOR, HIGH);
  analogWrite(BUZZER, 20);
  watch_status = "13";
}
*/
void sonicSession() {
  CM = CalDistance();
// Code 02


// Code 01
  if(CM > 100 || CM <= 0) {
    CM = -1;
    if(Error==0&&Many==1)Error++;
    else if(Error<2) Error++;
    else{
      Serial.println(F("Error Time"));
      Safe_Sta=2;
      Serial.println(F("Chance Safe_Sta because Error"));
      Many=0;
      Error=0; 
    }
  }
  if (CM!=-1) {
    Serial.println("OK Distance");
    if (CM < Safe_Sta + 3) {
      if(Safe_Sta==2){}
      else if(CM > Safe_Sta - 3){
          Serial.print(F("CM before Check Save State is : "));
          Serial.println(CM);
          CM = CalDistance();
          if(Safe_Sta-3<CM){
            if(Safe_Sta+3>CM){}
            else{
              Safe_Sta=2;// back to default
              Serial.println(F("Chace Safe_Sta to Default"));  
            }  
          }
          else{
            Safe_Sta=2;// back to default
            Serial.println(F("Chace Safe_Sta to Default"));
          }
      }  
    }
    else if(CM < Danger_Sta +1){
      Serial.println(F("Emergency Call"));
      CM = CalDistance();
      Serial.print(F("CM for Check Emergency is "));
      Serial.println(CM);
      if(CM <Danger_Sta+1 && CM>0){
        Many = 0;
        Serial.println(F("Real Danger"));
        SCM = CM;
        digitalWrite(VIBRATOR, HIGH);
        event_code="22";
        watch_status = "13";        
        Serial.println(F("Send 1322"));
        Write("1322"); 
      }
    }
    else{
      if (Many == 0) {
        CMP = CM;
        Many++;
        Serial.println(F("Plus Many for First Time"));
        Error=0;
      }
      else if (Many < 1) {
        Serial.println(F("Plus Many again"));
        Many++;
      }
      else {
        Many = 0;
        SCM=CM;
        Serial.println(F("Chace SCM"));
        digitalWrite(VIBRATOR, HIGH);
        watch_status = "13";
        event_code="22";
        Serial.println(F("Normall Call"));
        Serial.println(F("Send 1322"));
        Write("1322");
      }
    }
    CMP = CM;
  }
  Serial.print("CM is ");
  Serial.println(CM);
}
//Code 02
/*float CalDistance() {
  do{
  digitalWrite(TRIG, LOW);
  delay(2);
  digitalWrite(TRIG, HIGH);
  delay(2);
  digitalWrite(TRIG, LOW);
  int Time = pulseIn(ECHO, HIGH);
  CM = Time * 0.01274;
  }while(CM<0||CM>100);
}*/

// Code 01
float CalDistance() {
  digitalWrite(TRIG, LOW);
  delay(2);
  digitalWrite(TRIG, HIGH);
  delay(2);
  digitalWrite(TRIG, LOW);
  int Time = pulseIn(ECHO, HIGH);
  return Time * 0.01274;
}
/*void DHTSession() {
  Input.read11(DHT);
  temperature = Input.temperature;
  Serial.print("Temperature is ");
  Serial.println(temperature);
  Write("/Temperature",(String)temperature);
  if (temperature > 50) {
    watch_status = "13";
    event_code = "23";
    Write("/Status", (String)"1323");
  }
  delay(100);
}*/

void Write(String input) {
//  for(int x = 0 ; x < 2 ; x++){ 
    digitalWrite(CONNEC,LOW);
    String str = SET3+ input;
    Serial.print(F("Write: "));
    Serial.print(str);
    Serial.print("\t");
    data = Ciao.write(CONNECTOR, ADDRESS, str);
    Serial.print(F("Writing: "));
    if (!data.isEmpty()) {
      Serial.println(F("Success"));
      digitalWrite(CONNEC,HIGH);
//      break;
    } else {
      Serial.println(F("Failed"));
    }
    delay(100);
//  }
}
String Read(String var) {
  digitalWrite(CONNEC,LOW);
  while(true){
    String str = SET + var;
    Serial.print(F("Read: "));
    Serial.print(str);
    Serial.print("\t");
    data = Ciao.read(CONNECTOR, ADDRESS,str);
    Serial.print(F("Reading: "));
    if (!data.isEmpty()) {
      Serial.println(F("Success"));
      digitalWrite(CONNEC,HIGH);
      return data.get(2);
    } else {
      Serial.println(F("Failed: Reconnnect"));
    }
  }

  delay(100);
}
