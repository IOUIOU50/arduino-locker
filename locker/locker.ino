/*********************************
 * 라이브러리를 불러옵니다.
 * ******************************/
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <String.h>
#include <Wire.h>
#include <math.h>

/*********************************
 * 센서 - 아두이노 연결될 핀번호를 정의합니다.
 * ******************************/
#define PIN_SERVO 2     // sig = dig
#define PIN_LED_RED 3   // sig = dig
#define PIN_LED_BLUE 4  // sig = dig
#define PIN_POT 1       // sig = dig, POT : potentiometer
#define PIN_LED_BLUE 6  // sig = dig
#define RFID_RST 9      // sig = dig, RST  RESET
#define RFID_SS 10      // sig = dig, SS : SDA
/**
    키패드	핀
    2	    첫번째 핀
    1	    두번째 핀
    3	    네번째 핀
    4	    세번째 핀
 */
#define BUTTON_1 4  // 확인 버튼
#define BUTTON_2 5  // 취소버튼
#define BUTTON_3 6  // ??
#define BUTTON_4 7  // 문 닫힘 버튼

/*********************************
 * 코드에 사용될 변수를 선언합니다.
 * ******************************/
const int buttonDelay = 150;

bool isLocked;
bool isTagged;
int countSec;

//이전 실습에서 알아온 rfid 카드의 일련번호를 아래에 적도록 합니다.
byte registeredUid[4] = {0x83, 0xD2, 0xE9, 0x18};
String registeredPw = "1111";

/*
  센서/모듈 사용을 위한 클래스 변수를 선언합니다.
*/
Servo servo;
MFRC522 mfrc(RFID_SS, RFID_RST);  // Instance of the class
LiquidCrystal_I2C lcd(0x27, 16, 2);

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
    pinMode(BUTTON_1, INPUT_PULLUP);
    pinMode(BUTTON_2, INPUT_PULLUP);
    pinMode(BUTTON_3, INPUT_PULLUP);
    pinMode(BUTTON_4, INPUT_PULLUP);

    servo.attach(PIN_SERVO);
    SPI.begin();      // Init SPI bus
    mfrc.PCD_Init();  // Init MFRC522
    lcd.init();       // LCD 초기화
    lcd.backlight();

    /**
     *  init variables
     */
    isLocked = true;
    /**
     * logs
     */
    Serial.println("arduino starts");
    Serial.print("registered card uid : ");
    for (byte i = 0; i < 4; i++) {  // 태그의 ID출력을 ID사이즈 4 까지 반복
        Serial.print(
            registeredUid[i],
            HEX);  // mfrc.uid.uidByte[0] ~ mfrc.uid.uidByte[3]까지 출력
        Serial.print(" ");  // id 사이의 간격 출력
    }

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
 * loop
 * ******************************/
void loop()  // put your main code here, to run repeatedly:
{
    lcd.clear();
    lcd.setCursor(0, 0);
    setState(isLocked);
}

void setState(bool flag) {
    toggleLockState(flag);
    if (!flag) {
        setStateOpened();
        return;
    };
    setStateLocked();
}

void setStateLocked() {
    String input = "";
    int poten;
    int pwCursor = 0;

    while (isLocked) {
        /**
         * RFID로 잠금 해제
         */
        if (mfrc.PICC_IsNewCardPresent() && mfrc.PICC_ReadCardSerial()) {
            bool flag = isVerified(mfrc.uid.uidByte);
            if (flag) {
                isLocked = false;
                return;
            }
        }

        /**
         * 비밀번호로 잠금해제
         */
        poten = map(analogRead(PIN_POT), 0, 1023, 0, 10);
        lcd.setCursor(0, 0);
        lcd.print("input : ");
        lcd.setCursor(10, 0);
        lcd.print("or tag");
        lcd.setCursor(8, 0);
        lcd.print(poten);
        lcd.setCursor(0, 1);  // 1번 칸, 2번 줄
        lcd.print("password : ");
        for (int i = 0; i < input.length(); i++) {
            lcd.print("*");
        }
        // true = 1, false = 0. 스위치는 누르면 low(= 0)를 전송
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

bool isPwCorrect(String input) { return registeredPw == input; }

void setStateOpened() {
    lcd.print("opened!");
    while (!isLocked) {
        if (!digitalRead(BUTTON_3)) {
            isLocked = true;
            return;
        }
    }
}
bool checkPassword(String pw) {}

bool checkCard() {}
/***********************************************
 * functions
 ***********************************************/

// toggle degree of servo motor
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

// 리팩토링 필요
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
 * lcd functions
 */
void clearLine(int line) {
    if (line > 1 || line < 0) return;
    lcd.setCursor(0, line);
    lcd.print("                ");  // 공백 16개
    lcd.setCursor(0, line);
}

void printWrong() {
    lcd.setCursor(10, 1);
    lcd.print("wrong");
    delay(750);
    clearLine(1);
}

// 문 닫힘/열림 속도 조절
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
