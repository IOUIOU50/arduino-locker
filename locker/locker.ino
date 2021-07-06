/*********************************
 * 라이브러리를 불러옵니다.
 * ******************************/
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <string.h>

/*********************************
 * 센서 - 아두이노 연결될 핀번호를 정의합니다.
 * ******************************/
#define PIN_SERVO 2     // sig = dig
#define PIN_LED_RED 3   // sig = dig
#define PIN_LED_BLUE 4  // sig = digj
#define PIN_POT 5       // sig = dig, POT : potentiometer
#define PIN_LED_BLUE 6  // sig = dig
#define PIN_CDS A1      // CDS = analog
#define RFID_RST 9      // sig = dig, RST  RESET
#define RFID_SS 10      // sig = dig, SS : SDA

/*********************************
 * 코드에 사용될 변수를 선언합니다.
 * ******************************/
bool isLocked;
bool isTagged;
int countSec;

//이전 실습에서 알아온 rfid 카드의 일련번호를 아래에 적도록 합니다.
byte registeredUid[4] = {0x83, 0xD2, 0xE9, 0x18};

/*
  센서 사용을 위한 클래스 변수를 선언합니다.
*/
Servo servo;
MFRC522 mfrc(RFID_SS, RFID_RST);  // Instance of the class

/*********************************
 * dev variables
 * ******************************/
String readString;

/*********************************
 * setup함수. 1회 실행.
 * ******************************/
void setup() {  // put your setup code here, to run once:
  /**
   * serial transportation settings
   */
  Serial.begin(9600);
  servo.attach(PIN_SERVO);
  pinMode(PIN_CDS, INPUT);
  SPI.begin();      // Init SPI bus
  mfrc.PCD_Init();  // Init MFRC522

  /**
   *  init variables
   */
  if (analogRead(PIN_CDS) > 700) {  // 잠김
    isLocked = true;
  } else {
    isLocked = false;
  }
  countSec = 0;
  toggleLockState(isLocked);

  /**
   * logs
   */
  Serial.println("arduino starts");
  Serial.print("registered card uid : ");
  for (byte i = 0; i < 4; i++) {  // 태그의 ID출력을 ID사이즈 4 까지 반복
    Serial.print(registeredUid[i],
                 HEX);  // mfrc.uid.uidByte[0] ~ mfrc.uid.uidByte[3]까지 출력
    Serial.print(" ");  // id 사이의 간격 출력
  }
  Serial.println();

  /**
   * dev settings
   */
}

/*********************************
 * loop
 * ******************************/
void loop()  // put your main code here, to run repeatedly:
{
  /**
   * 열려있을 때
   */
  while (!isLocked)  //! isLocked : case is open
  {
    int valCds = analogRead(PIN_CDS);
    delay(1000);
    if (valCds > 700) {
      countSec++;
      if (countSec > 3) {
        toggleLockState(true);
        return;
      }
      continue;
    }
    countSec = 0;

    return;
  }

  /**
   * 잠겨있을 때
   */
  if (!mfrc.PICC_IsNewCardPresent() || !mfrc.PICC_ReadCardSerial()) {
    delay(500);  // 0.5초의 딜레이
    return;
  }

  // card 유효성 검증
  if (isVerified(mfrc.uid.uidByte)) {
    Serial.println("true");
    Serial.println("servo toggle");
    toggleLockState(false);  // false : open / true : closed
  }
}

/***********************************************
 * functions
 ***********************************************/

// toggle degree of servo motor
void toggleLockState(bool state) {
  isLocked = state;
  if (state) {
    Serial.print("isLocked : ");
    Serial.println(isLocked);
    servo.write(180);
    delay(1000);
    return;
  }
  Serial.print("isLocked : ");
  Serial.println(isLocked);
  servo.write(0);
  delay(1000);
}

// 리팩토링 필요
bool isVerified(byte uid[]) {
  // if (sizeof(uid) != sizeof(registeredUid))
  //   return false;

  bool flag = true;
  for (byte i = 0; i < 4; i++) {
    Serial.print(registeredUid[i]);
    Serial.print("\t");
    Serial.print(uid[i]);
    Serial.println("");

    if (registeredUid[i] != uid[i]) flag = false;
    // return false;
  }
  Serial.println(flag);
  return flag;
  // return true;
}

/***********************************************
 * only dev functions
 ***********************************************/
String readText() {
  noInterrupts();  // critical section
  while (Serial.available()) {
    delay(2);  // delay to allow byte to arrive in input buffer
    char c = Serial.read();
    readString += c;
  }
  String returnStr = readString;
  while (Serial.available()) {
    readString = Serial.readString();
  }

  readString = "";
  interrupts();  // release
  return returnStr;
}

