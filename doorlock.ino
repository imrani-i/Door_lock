#include <Keypad.h> // keypad 라이브러리
#include <LiquidCrystal_I2C.h> // lcd 라이브러리
#include <Adafruit_Fingerprint.h>


#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2, 3);

#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1

#endif

////////////////////////// 키패드 전처리 //////////////////////////
const byte ROWS = 4; // 행의 갯수
const byte COLS = 3; // 열의 갯수
byte rowPins[ROWS] = {13, 12, 11, 10}; // R1,R2,R3,R4 행(Raw)가 연결된 아두이노 핀 번호
byte colPins[COLS] = {6, 7, 8}; // C1,C2,C3,C4 열(column)가 연결된 아두이노 핀 번호
char myKeys[ROWS][COLS] = { // 키배치
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
/////////////////////////////////////////////////////////////////


LiquidCrystal_I2C lcd(0x27, 16, 2); // lcd 주소, 크기를 매개로 lcd 인스턴스 생성
Keypad customKeypad = Keypad(makeKeymap(myKeys), rowPins, colPins, ROWS, COLS); // 키패드 인스턴스 생성
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

uint8_t id;
bool adminFlag = false; // 어드민 모드 플래그
bool compareFlag = false; // 루프 한틱마다 숫자비교를 위한 숫자 비교 플래그
int mainIndex = 0; // 메인화면의 lcd 인덱스
int menuIndex = 1; // 관리자모드 메인인덱스
bool menuFlag = false;

char pw[4] = "0000"; // 비밀번호

char inPw[4] = "0000"; // 입력받는 패스워드를 저장할 배열
int inPwIndex = 0; // 위 배열의 인덱스

char inNum[3] = {'\0', '\0', '\0'}; // 입력받는 숫자를 저장할 배열
int inNumIndex = 0; // 위 배열의 인덱스

void setup() {
  lcd.init(); // lcd 인스턴스의 생성자 호출
  lcd.backlight(); // lcd 백라이트 켜기
  lcd.clear(); // lcd 초기화면 비우고 0.0으로 커서 이동
  lcd.print("hello");
  customKeypad.addEventListener(keypadEvent); // Add an event listener for this keypad
  customKeypad.setHoldTime(3000);  // 키 홀드 시간 설정 - 2000 = 2초
  finger.begin(57600);
}


void loop(){
  char key = customKeypad.getKey(); // 루프 한 틱마다 입력된 키 저장

  if (adminFlag)
    if (key)
      if (menuFlag) {
        setLcd(0);
        activeMenu(key);
      }
  else {
    if (compareFlag) setLcd(2); // 비교모드일경우
    else setLcd(3); // 유저 모드일 경우
    
    if (key)
      if (key != '*' && key != '#')
        setInputKey(key); // 숫자일경우 키입력 받기

    //getFingerprintID();
  }
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  p = finger.image2Tz();
  p = finger.fingerSearch();
  if (p != FINGERPRINT_OK) {
    //Serial.println("Unknown error");
    return p;
  }
  openDoor();
  return finger.fingerID;
}

void isAdminTrue() {
  setLcd(4); // 관리자 모드 허용
  clearIndex();
  menuIndex = 1;
  compareFlag = false;
  adminFlag = true;
  setLcd(1); // 어드민 모드일 경우
  setLcd_Menu(menuIndex);
}

void openDoor() {
  setLcd(5);
  clearIndex();
}

void setInputKey(char key) {
  if (mainIndex == 0) setLcd(12);
  
  lcd.setCursor(mainIndex + 3, 1); // 메인인덱스번째에 입력받은 * 표시
  lcd.print("*");
  mainIndex++; // 인덱스 증가
  
  inPw[inPwIndex] = key; // 입력받은 키값을 배열에 저장하고
  inPwIndex++; // 인덱스 증가
  
  if (inPwIndex == 4) { // 4자리 모두 입력받은 경우
    if (strncmp(pw, inPw, 4) == 0) { // 입력받은 4자리가 비밀번호와 일치하는 경우
      if (compareFlag) isAdminTrue();
      else openDoor();
    }
    
    else {
      setLcd(6); // 비밀번호 틀릴시
      clearIndex();
      if (compareFlag) compareFlag = false;
    }
  }
}

void setLcd(int displayNum) {
  switch (displayNum) {
    case 0: // 2행 비우기
      lcd.setCursor(0, 1);
      lcd.print("                ");
      break;
      
    case 1: // 어드민 모드 초기 화면
      lcd.setCursor(0, 0);
      lcd.print("Select Menu  [A]");
      break;
      
    case 2: // 비교 모드 진입 화면
      lcd.setCursor(0, 0);
      lcd.print("Admin Access [U]");
      lcd.setCursor(0, 1);
      lcd.print("PW:             ");
      break;

    case 3: // 사용자 모드 초기 화면
      lcd.setCursor(0, 0);
      lcd.print("PW or FP     [U]");
      break;

    case 4: // 비교모드 - 비교성공시
      lcd.setCursor(0, 1);
      lcd.print("Welcome!        ");
      delay(1000);
      break;

    case 5: // 문열기시
      lcd.setCursor(0, 1);
      lcd.print("Door Open       ");
      break;
      
    case 6: // 비밀번호 비교 실패시
      lcd.setCursor(0, 1);
      lcd.print("Access Denied   ");
      break;

    case 7: // 메뉴 1
      lcd.setCursor(0, 0);
      lcd.print("Enroll FP    [A]");
      lcd.setCursor(0, 1);
      lcd.print("Put FPnum 1~127 ");
      break;

    case 8: // 메뉴 2
      lcd.setCursor(0, 0);
      lcd.print("Delete FP    [A]");
      lcd.setCursor(0, 1);
      lcd.print("Enter your FPnum");
      break;

    case 9: // 메뉴 3
      lcd.setCursor(0, 0);
      lcd.print("Update PW    [A]");
      lcd.setCursor(0, 1);
      lcd.print("Enter new PW    ");
      break;

    case 10: // 메뉴 4
      lcd.setCursor(0, 0);
      lcd.print("Are you sure?[A]");
      lcd.setCursor(0, 1);
      lcd.print("Enter PW        ");
      break;

    case 11: // 메뉴 5
      lcd.setCursor(0, 0);
      lcd.print("Save&Exit    [A]");
      lcd.setCursor(0, 1);
      lcd.print("Enter 5         ");
      break;

    case 12:
      lcd.setCursor(0, 1);
      lcd.print("PW:             ");
      break;
      
    case 13:
      lcd.setCursor(0, 0);
      lcd.print("Save&Exit    [A]");
      lcd.setCursor(0, 1);
      lcd.print("Enter 5         ");
      break;
    break;
  }
}

void setLcd_Menu(int menuNum) {
  setLcd(0);
  lcd.setCursor(0, 1);
  switch(menuNum) {
    case 1: // 지문 등록
      lcd.print("<-  Enroll FP ->");
      break;
      
    case 2: // 지문 삭제
      lcd.print("<-  Delete FP ->");
      break;
         
    case 3: // 비밀번호 변경
      lcd.print("<-  Update PW ->");
      break;
      
    case 4: // 전체 초기화
      lcd.print("<-  Reset ALL ->");
      break;
      
    case 5: // 저장 & 관리자모드 종료
      lcd.print("<-    Exit    ->");
      break;
    break;
  }
}

void selectMenu(int menuNum) {
  lcd.clear();
  setLcd(menuNum + 6);
}

void clearIndex() {
  inPwIndex = 0;
  mainIndex = 0;
}

void activeMenu(char key) {
  switch (menuIndex) {
    case 1: // 지문 등록, 지문인식하여 번호 지정
      if (key != '#') {
        lcd.setCursor(mainIndex, 1); // 메인인덱스번째에 입력받은 키 표시
        lcd.print(key);
        mainIndex++; // 인덱스 증가
  
        inNum[inNumIndex] = key; // 입력받은 키값을 배열에 저장하고
        inNumIndex++; // 인덱스 증가

        if (inNumIndex == 3) { // 3자리 모두 입력받은 경우
          //enroll(ctou());
      }
      else {
        //enroll(ctou());
    }
  }

      break;
      
    case 2: // 지문 삭제, 지문번호 입력받기
      break;
      
    case 3: // 비밀번호 변경, 비번 입력받기
      check_cnt=0
      lcd.print("Input New Password");
      delay(1000);
      lcd.print("pw:----");
      for (int k = 0; k < 4; k++) {
        inPw[k] = customKeypad.getKey();
        lcd.setCursor(k + 3, 0);
        lcd.print("*");  
      }
      do
      {
        if(check_cnt>0){
          lcd.print("Incorrect Password");
          
        }
        lcd.print("Check New Password");
        delay(1000);
        lcd.print("pw:----");
        for (int k = 0; k < 4; k++) {
          cmPw[k] = customKeypad.getKey();
          lcd.setCursor(k + 3, 0);
          lcd.print("*");  
        }
        check_cnt++;
      } while(strncmp(inPw, cmPw,4) == 0 && check_cnt<3)
      if(strncmp(inPw, cmPw,4) == 1){
        lcd.print("Success!");
        strncpy(pw,inPw,4);
        //초기화면
      }
      else{
        lcd.print("Try again!");
        
        //초기화면
      }
      }
      

      break;
    case 4: // 전체초기화, 비번 입력
      break;
      
    case 5: // 종료, 5번 입력
      break;
    break;
  }
}

uint8_t ctou(char c) {
  int i = 0;
  for (inNumIndex = 0; inNum[inNumIndex] != '\0'; inNumIndex++) {
    switch (inNumIndex) {
      case 0:
        i += (c - 48) * 100;
        break;
      case 1:
        i += (c - 48) * 10;
        break;
      case 2:
        i += c - 48;
        break;
      break;
    }
  }
  uint8_t u = i;
  return u;
}

void keypadEvent(KeypadEvent key){ // 키패드 이벤트 함수
  switch (customKeypad.getState()){
    case HOLD:
      switch (key){
        case '*':
          key = '\0';
          compareFlag = true;
          break;
        break;
      }
      
    case PRESSED:
      if (adminFlag)
        switch(key) {
          case '4':
            if (menuIndex == 1) menuIndex = 5;
            else menuIndex--;
            setLcd_Menu(menuIndex);
            break;
            
          case '5':
            selectMenu(menuIndex);
            menuFlag = true;
            break;
            
          case '6':
            if (menuIndex == 5) menuIndex = 1;
            else menuIndex++;
            setLcd_Menu(menuIndex);
            break;

          case '*':
            isAdminTrue();
            break;
          break;
        }
    break;
  }
}
