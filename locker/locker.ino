/*********************************
 * 실습에 필요한 라이브러리를 불러옵니다.
 * ******************************/
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <String.h>
#include <Wire.h>
#include <math.h>

/*********************************
 * 센서가 아두이노에 연결될 핀번호를 정의합니다.
 * ******************************/
#define PIN_SERVO 2     // sig = dig
#define PIN_LED_RED 3   // sig = dig
#define PIN_LED_BLUE 4  // sig = dig
#define PIN_POT 3       // sig = dig, POT : potentiometer
#define PIN_LED_GREEN 6 // sig = dig
#define RFID_RST 9      // sig = dig, RST  RESET
#define RFID_SS 10      // sig = dig, SS : SDA
/**
    키패드	핀
    2	    첫번째 핀
    1	    두번째 핀
    3	    네번째 핀
    4	    세번째 핀
 */
#define BUTTON_1 4 // 확인 버튼
#define BUTTON_2 5 // 취소버튼
#define BUTTON_3 6 // ??
#define BUTTON_4 7 // 문 닫힘 버튼

/*********************************
 * 코드에 사용될 변수들을 선언하는곳 입니다.
 * ******************************/
const int buttonDelay = 150; //

bool isLocked;
bool isTagged;
int countSec;

/**
 * 이전 실습을 통해 진행하였던 내용을 아래에 추가합니다.
 * registeredUid : 등록할 카드의 일련번호
 * registeredPw : 기본으로 사용할 비밀번호
 * oneTimePw : 무인택배함에 사용될 일회용 비밀번호
 */
byte registeredUid[4] = {0x83, 0xD2, 0xE9, 0x18}; // rfid 카드 일련번호
String registeredPw = "1111"; // 사용자가 지정한 비밀번호
String oneTimePw = "2222"; // '무인 택배함' 기능에 사용될 일회용 비밀번호

/*
  센서/모듈 사용을 위한 클래스 변수를 선언합니다.
*/
Servo servo;
MFRC522 mfrc(RFID_SS, RFID_RST); // Instance of the class
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*********************************
 * for dev
 * ******************************/

/*********************************
 * setup함수. 1회 실행.
 * ******************************/
void setup() {
  /**
   * 시리얼 통신을 위한 준비
   */

  Serial.begin(9600);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUTTON_3, INPUT_PULLUP);
  pinMode(BUTTON_4, INPUT_PULLUP);
  servo.attach(PIN_SERVO);
  SPI.begin();     // Init SPI bus
  mfrc.PCD_Init(); // Init MFRC522
  lcd.init();      // LCD 초기화
  lcd.backlight();

  /**
   * setup()이전에 선언했던 변수들에 대해
   * 필요한 초기값을 대입해 줍니다.
   */
  isLocked = true; // 처음상태 : 잠겨있는 상태

  /**
   * 시리얼 모니터에 초기화 상태를 출력합니다.
   */
  // 등록된 카드 uid 출력
  Serial.println("arduino starts");
  Serial.print("registered card uid : ");
  for (byte i = 0; i < 4; i++) { // 태그의 ID출력을 ID사이즈 4 까지 반복
    Serial.print(registeredUid[i], HEX);
    Serial.print(" "); // id 사이의 간격 출력
  }

  // 등록된 비밀번호 출력
  Serial.print("registered password : ");
  for (int i = 0; i < registeredPw.length(); i++) {
    Serial.print(registeredPw.charAt(i));
  }
  Serial.println();

  /**
   * dev settings
   */
}

/*********************************
 * 아두이노가 반복할 상태입니다.
 * 스마트 금고는 아래의 상태를 반복합니다.
 * loop
 * -> setState: locked/opened 어떤 상태에 들어갈지 결정
 * -> locked/opened : 잠금 해제 여부에 따라 변경
 * -> loop으로 돌아와 어떤 상태에 들어갈지 다시 결정
 * -> ...(반복)
 * ******************************/
void loop() {
  lcd.clear();
  lcd.setCursor(0, 0);
  setState(isLocked);
}

/***********************************************
 * loop문 및 사용되는 함수를 정리한 부분입니다.
 ***********************************************/

/**
 * loop문에서 어떤 상태에 들어갈지 결정짓는 함수입니다.
 * setState에서 locked일지 opened일지 결정되고,
 * 상태가 전환 될 때마다 locked/opened에서 빠져나와 다시 loop으로 돌아갑니다.
 */
void setState(bool flag) {
  toggleLockState(flag);
  if (!flag) {
    setStateOpened();
    return;
  };
  setStateLocked();
}

/**
 * 잠겨있을 때 아두이노가 수행할 동작을 정의한 함수입니다.
 * 잠겨있을 때에는 두 가지 행동을 취할 수 있습니다.
 * 1. 비밀번호로 잠금 해제
 * 2. RFID카드로 잠금해제
 * 3. 블루투스로 연결된 핸드폰에 비밀번호를 입력하여 잠금해제(예정)
 */
void setStateLocked() {
  String input = ""; // 입력될 비밀번호가 담길 변수
  int poten;         // 입력될 비밀번호를 측정할 potentiometer값

  while (isLocked) { // 잠겨있는 동안은 setStateLocked()함수 유지
    // 1. RFID로 잠금 해제
    if (mfrc.PICC_IsNewCardPresent() && mfrc.PICC_ReadCardSerial()) {
      bool flag = isVerified(mfrc.uid.uidByte);
      if (flag) {
        isLocked = false;
        return; // 카드 잠금 해제 후, locekd상태에서 빠져나와 loop로 복귀
      }
    }

    // 2. 비밀번호로 잠금 해제
    poten = map(analogRead(PIN_POT), 0, 1023, 0, 10);
    lcd.setCursor(0, 0);
    lcd.print("input : ");
    lcd.setCursor(10, 0);
    lcd.print("or tag");
    lcd.setCursor(8, 0);
    lcd.print(poten);
    lcd.setCursor(0, 1); // 1번 칸, 2번 줄
    lcd.print("password : ");
    for (int i = 0; i < input.length(); i++) {
      lcd.print("*");
    }
    // true = 1, false = 0. 스위치는 누르면 low(= 0)를 전송(low active)
    if (!digitalRead(BUTTON_2)) {
      delay(buttonDelay);
      if (input.length() < 4) {
        input.concat(String(poten));
        Serial.println(input);
      } else {
        if (isPwCorrect(input)) {
          isLocked = false;
          return;
        }
        printWrong();
        input = "";
        clearLine(1);
      }
    }
    if (!digitalRead(BUTTON_1) && input.length() != 0) {
      delay(buttonDelay);
      input.remove(input.length() - 1);
      clearLine(1);
      lcd.print("password : ");

      for (int i = 0; i < input.length(); i++) {
        lcd.print("*");
      }
      Serial.println(input);
    }
  }
}

/**
 * 열려있을 때 아두이노가 동작할 내용에 대한 함수
 * 금고가 열려있을 때 아두이노는
 * 1.실내 LED 점등
 * 2.메뉴기능 활성화
 *  - 스터디 기능
 *  - 일회용 비밀번호 설정
 *  -
 * 3.
 */
void setStateOpened() {
  lcd.print("opened!");
  while (!isLocked) {
    if (!digitalRead(BUTTON_3)) {
      isLocked = true;
      return;
    }
  }
}

/**
 * pw를 검사하는 함수
 * pw에는
 * (1)저장된 비밀번호와
 * (2)일회용 비밀번호가 있다.
 * 일회용 비밀번호는 사용 후 소멸
 */
bool isPwCorrect(String input) {
  if (input == registeredPw)
    return true;
  if (input == oneTimePw) {
    oneTimePw = ""; // 일회용 비밀번호 사용 후 소멸
    return true;
  }
  return false;
}

/**
 * 잠금<->열림 전환 시 모터 동작을 담당하는 함수
 */
void toggleLockState(bool state) {
  isLocked = state;
  lcd.clear();
  lcd.print("door is moving!");
  lcd.setCursor(0, 1);
  lcd.print("be careful!");

  if (state) {
    Serial.print("isLocked : ");
    Serial.println(isLocked);
    // servo.write(180);
    myServoWrite(180);
    lcd.clear();
    return;
  }

  Serial.print("isLocked : ");
  Serial.println(isLocked);
  // servo.write(0);
  myServoWrite(0);
  lcd.clear();
}

/**
 * RFID 카드 검증을 수행하는 함수
 */
bool isVerified(byte uid[]) {
  bool flag = true;
  Serial.print("registered");
  Serial.print("\t");
  Serial.println("input");

  for (byte i = 0; i < 4; i++) {
    Serial.print(registeredUid[i]);
    Serial.print("\t\t\t");
    Serial.println(uid[i]);

    if (registeredUid[i] != uid[i]) {
      printWrong();
      return false;
    }
  }
  return true;
}

/**
 * lcd의 한 줄삭제 기능을 담당하는 함수를 자체 구현
 *
 * @params
 * int line : 몇 번째 줄을 삭제시킬 것인지
 *
 * 줄 삭제 후, lcd커서를 해당 줄 맨앞으로 지정
 */
void clearLine(int line) {
  if (line > 1 || line < 0)
    return;
  lcd.setCursor(0, line);
  lcd.print("                "); // 공백 16개
  lcd.setCursor(0, line);
}

/**
 * 잘못된 RFID카드 태그, 비밀번호 입력 시
 * lcd에 "wrong" 문구를 출력시키는 함수
 */
void printWrong() {
  lcd.setCursor(10, 1);
  lcd.print("wrong");
  delay(750);
  clearLine(1);
}

/**
 * 문/닫힘, 열림 속도를 조정하는 함수
 *
 * params
 * angle : 서보모터의 각 조절
 */
void myServoWrite(int angle) {
  int pos = servo.read();
  if (angle - pos > 0) {
    for (int i = pos; i <= angle; i++) {
      servo.write(i);
      delay(10);
    }
    return;
  }
  for (int i = pos; i >= angle; i--) {
    servo.write(i);
    delay(10);
  }
}

/***********************************************
 * only dev functions
 ***********************************************/
