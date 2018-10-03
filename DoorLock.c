#include <SoftwareSerial.h> //wifi 통신을 위해 필요한 헤더파일 선언
#include <LiquidCrystal.h>// LCD Display를 편하게 사용하기 위한 헤더파일 선언
#include <EEPROM.h> // 변경된 패스워드 저장하기 위한 변수
#include <Servo.h>
#include "pitches.h"

#define numRows 4
#define numCols 4

#define MOTOR_PIN 10
#define SPEAKER 11
#define INPUT_KEY hexaKeys[i][j]
#define PW_WRONG 1
//PW_WRONG번까진 참는다 PW_WRONG+1번째 되는 순간 사이렌

String SSID = "nam";
String PASSWORD = "hkit0008";
String SERVER = "192.168.0.2";//192.168.0.35
String PORT = "7777";     // 8000

String WARNING_MSG = "PW";         // 서버로 보낼 문자열

const int pinRows[numRows] = {6,7, 8, 9};  // input
const int pinCols[numCols] = {2,3, 4, 5};  // output

const int melody[]={NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5,   NOTE_G5, NOTE_A5};

char secretCode[] = "1234"; // 초기 비밀번호
char Input_password[] = "1234";//입력한 패스워드
char tmp_SecretCode[]="0000";// 비밀번호 변경시 입력받은 비밀번호 저장용 변수
// 비밀번호를 바꾸면 잘못 누른 패스워드도 바꿔야 된다..

int pw_position = 0;//패스워드 자리수
int wrong_input = 0;//잘못된 패스워드 자리수 입력
int incorrect_pw= 0;//패스워드 입력 실패 횟수

boolean pw_Mode=false;
boolean pw_Change=false;
boolean pressed =false;
boolean row_temp=false;
boolean check_pw=false;

SoftwareSerial esp(12, 13);// RX, TX
LiquidCrystal lcd(A4, A5, A0, A1, A2, A3);
Servo myservo;
int servo_pos=0;  // 서보모터의 위치값 저장

const char hexaKeys[numRows][numCols]={
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'},
};

void beep(int order, int note_length){
  tone(SPEAKER,melody[order],note_length);
  delay(325);
  noTone(500);
}

void right_number(int i,int j){
    pw_position++;
    //Serial.print("right number!");//////////////디버깅용
    Serial.println(INPUT_KEY);//INPUT_KEY == hexaKeys[i][j]
}
void wrong_number(int i, int j){
    pw_position++;
    wrong_input++;
    //Serial.print("(Does not match password position)");///디버깅용
    Serial.println(INPUT_KEY);//INPUT_KEY == hexaKeys[i][j]
}
void correct_pw(){
  Serial.println("correct password");
  beep(0,250);//도
  beep(2,250);//미
  beep(4,250);//솔
  beep(7,250);//도
  pw_position = 0;
  incorrect_pw= 0;  
}

void incorrect_warning(){
  Serial.println("incorrect password");
  beep(7,1000);// 땡~          
  
  incorrect_pw++;
  wrong_input = 0;
  pw_position=0;
  
  pw_Mode=false; // 비밀번호 변경모드에서 패스워드를 틀리면 변경모드를 취소시킴
}

void incorrect_password(){
  Serial.println("Warning! incorrect password!");
  wrong_input=0;
  incorrect_pw=0;
  pw_position=0;
  for(int k=0;k<3;k++){
    beep(8,500);
    beep(9,500);      
  }
  pw_Mode=false;// 비밀번호를 여러번 틀린 상태에서 비밀번호 변경모드를 눌러도 틀리면 변경모드를 취소
}

void setup() {
  esp.begin(9600);
  Serial.begin(9600);    // init Serial communication
  lcd.begin(16, 2);       // 16X2 설정
  myservo.attach(MOTOR_PIN);
  myservo.write(0);
  pinMode(SPEAKER,OUTPUT);

  // initialize row pins as INPUT_PULLUP
  for(int i=0; i<numRows; i++) {
      pinMode(pinRows[i], INPUT_PULLUP);
  }

  // initialize column pins as OUTPUT
  for(int j=0; j<numCols; j++) {
      pinMode(pinCols[j], OUTPUT);
      digitalWrite(pinCols[j], HIGH);// set initial output as HIGH
  }

  for(int i=0;i<4;i++){
    if(EEPROM.read(i) < 48 || EEPROM.read(i)>57 ){//아스키코드 0~9값
      break;
    }else{
      secretCode[i]= EEPROM.read(i);//저장된 패스워드 불러오기
      Input_password[i]=EEPROM.read(i);//저장된 패스워드 불러오기2
    }
  }
  reset();
  connectWifi();
  lcd.print("Input Password");
}

void loop() {
  for(int j=0; j<numCols; j++) {
    digitalWrite(pinCols[j], LOW);// set as LOW to check button press
    
    for(int i=0; i<numRows; i++) {
      if(digitalRead(pinRows[i]) == LOW){

        beep(0,250);// '도'를 250의 길이(단위 모름)동안 출력
        pressed = true;
        
        if(INPUT_KEY=='*' || INPUT_KEY=='#'){ // 비밀번호 입력 초기화 버튼
          pw_position = 0;
          wrong_input = 0;
          pw_Mode=false;/// 비밀번호 변경모드 강제 종료
          Serial.println("Input Reset");
          lcd.clear();
          lcd.print("Input Reset");
          delay(1000);
          lcd.clear();
          lcd.print("Input Password");
        }
        else if(INPUT_KEY=='D'){ // 비밀번호 입력 하나 취소
          /////////잘못 입력한 패스워드 카운트를 적절히 초기화 시켜야함
          if(Input_password[pw_position] != secretCode[pw_position]){// 취소 시킨 자리수의 비밀번호가 틀린 입력이었다면
            if(wrong_input>0){
              wrong_input--;
            }
            Input_password[pw_position]=secretCode[pw_position];
          }
          if(pw_position>=1){
            pw_position--;
          }
          lcd.clear();
          lcd.print("Input Cancel"); 
          Serial.println(pw_position);
          for(int i=0;i<pw_position;i++){
            lcd.setCursor(i,1);
            lcd.print("*");
          }
        }else if(INPUT_KEY=='A' || INPUT_KEY=='B' || INPUT_KEY=='C'){ // 비밀번호 변경
          if(pw_Change==false){
            Serial.println("Change Password");
            lcd.clear();
            lcd.print("Change Password");
            pw_Mode=true;
          }else{
            Serial.println("password is included only number");//비밀번호 변경모드에서 A,B,C 키를 누르면 취소된다.
            pw_Change=false;
            pw_Mode=false;
            pw_position=0;
            lcd.clear();
            lcd.print("password is");
            lcd.setCursor(0,1);
            lcd.print("only number!");
            delay(2000);
            lcd.clear();
          }
        }        
        else if(pw_Change==true && pw_position<=3){// 변경할 비밀번호 입력
          tmp_SecretCode[pw_position]=INPUT_KEY;///// 임시 비밀번호 저장 변수
          pw_position++;
          Serial.print("New password : ");
          Serial.println(INPUT_KEY);
          lcd.clear();
          lcd.print("New password : ");
          for(int i=0;i<pw_position;i++){
            lcd.setCursor(i,1);
            lcd.print("*");
          }
        }
        else if(pw_Change==true && pw_position>=4 && !check_pw){// 새로운 패스워드 입력이 완료되면 boolean을 참으로
          pw_position=0;
          check_pw=true;  
        }             
        else if(INPUT_KEY==secretCode[pw_position] && pw_Change==false){// 해당 자리에 맞는 비밀번호인 경우
          //비밀번호 변경시 이 함수는 실행되지 않음
          right_number(i,j);
          Input_password[pw_position-1]=INPUT_KEY;
          lcd.clear();
          lcd.print("Input Password");
          for(int i=0;i<pw_position;i++){
              lcd.setCursor(i,1);
              lcd.print("*");
          }
        }
        else if(INPUT_KEY != secretCode[pw_position] && pw_Change==false){// 해당 자리에 맞는 비밀번호가 아니라면
          //비밀번호 변경시 이 함수는 실행되지 않음
          wrong_number(i,j);
          Input_password[pw_position]=INPUT_KEY;//틀린 비밀번호는 뭘 눌렀는지 기억
          lcd.clear();
          lcd.print("Input Password");
          for(int i=0;i<pw_position;i++){
              lcd.setCursor(i,1);
              lcd.print("*");
            }        
        }
       
        if(pw_position == 4 && wrong_input == 0 && pw_Change==false){
          //비밀번호 올바른 입력시
          if(pw_Mode==true){// 비밀번호 변경모드라면
            lcd.clear();
            Serial.println("Input New password(number only)");
            pw_Change=true;
            pw_position = 0;
            incorrect_pw= 0;
            lcd.clear();
            lcd.print("Input New P/W");
            for(int i=0;i<pw_position;i++){
              lcd.setCursor(i,1);
              lcd.print("*");
            }
          }else{
            correct_pw();            
            myservo.write(179);
            lcd.clear();
            lcd.print("Door opened!");
            delay(5000);
            lcd.clear();
            myservo.write(0);
            lcd.print("Input Password");
          }
        }
        else if(pw_position <= 3 && pw_Change==true){/// 비밀번호 변경 시
          tmp_SecretCode[pw_position]=INPUT_KEY;
        }else if(pw_position>=4 && pw_Change==true){// 비밀번호 변경이 끝났다면
          
          pw_position=0;
          pw_Change=false;
          pw_Mode=false;
          for(int i=0;i<4;i++){
            secretCode[i]=tmp_SecretCode[i];
            Input_password[i]=secretCode[i];
            EEPROM.write(i,tmp_SecretCode[i]);
          }
          Serial.println("change password complete");
          lcd.clear();
          lcd.print("Change Success");
          delay(1500);
          lcd.clear();
          lcd.print("Input Password");
        }
        
        else if(incorrect_pw>=PW_WRONG && pw_position == 4 && pw_Change==false){//4자리 비밀번호의 입력이 n번 이상 틀린 경우
          lcd.clear();
          lcd.print("Please");
          lcd.setCursor(0,1);
          lcd.print("Try Again later");
          incorrect_password();
          ////////비밀번호 다회 입력 실패시 서버로 전송///////////////////////////////////
          tcpip();
          ////////비밀번호 다회 입력 실패시 서버로 전송///////////////////////////////////
          pw_Mode=false;// 비밀번호 변경모드인 경우 모드 취소
          lcd.clear();
          lcd.print("Input Password");
        }
        else if(wrong_input!=0 && pw_position==4 && incorrect_pw<PW_WRONG && pw_Change==false){
          // 입력한 4자리 비밀번호가 하나라도 틀린 경우면서 경고음이 울릴 만큼 틀리지 않은 경우
          incorrect_warning();
          myservo.write(0);
          pw_Mode=false;
          lcd.clear();
          lcd.print("Wrong Password");
          delay(1500);
          lcd.clear();
          lcd.print("Input Password");
        }
      }
    } 
    digitalWrite(pinCols[j],HIGH);    // set as default (HIGH)   
  }
}

void reset() { 
  esp.println("AT+RST"); 
  delay(1000); 
  if (esp.find("OK") ) Serial.println("Module Reset"); 
} 
//connect to your wifi network
 
void connectWifi() { 
  String cmd = "AT+CWJAP=\"" + SSID + "\",\"" + PASSWORD + "\"";
  esp.println(cmd); 
  delay(4000); 
  if (esp.find("OK")) { 
    Serial.println("Connected!"); 
  } 
  else {
    Serial.println("Connect failed, retry");
    connectWifi(); 
  } 
}

void tcpip() {
 
  esp.println("AT+CIPSTART=\"TCP\",\"" + SERVER + "\",7777");//start a TCP connection.
 
  if ( esp.find("OK")) { 
    Serial.println("TCP connection ready"); 
  } delay(1000);
 
  String sendCmd = "AT+CIPSEND=";//determine the number of caracters to be sent.
 
  esp.print(sendCmd); 
  esp.println(WARNING_MSG.length()); 
  delay(500);
 
  if (esp.find(">")) {
    Serial.println("Sending.."); esp.print(WARNING_MSG); 
    if ( esp.find("SEND OK")) {      
      // close the connection 
      esp.println("AT+CIPCLOSE"); 
    } 
  }
}