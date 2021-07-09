/*********************************
 * 라이브러리를 불러옵니다.
 * ******************************/
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <String.h>
#include <Wire.h>

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
#define BUTTON_1 7

/*********************************
 * 코드에 사용될 변수를 선언합니다.
 * ******************************/
bool isLocked;
bool isTagged;
int countSec;

//이전 실습에서 알아온 rfid 카드의 일련번호를 아래에 적도록 합니다.
byte registeredUid[4] = {0x83, 0xD2, 0xE9, 0x18};
String registeredPw = "1234";

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
    pinMode(BUTTON_1, INPUT);
    servo.attach(PIN_SERVO);
    SPI.begin();      // Init SPI bus
    mfrc.PCD_Init();  // Init MFRC522
    lcd.init();       // LCD 초기화
    lcd.backlight();

    /**
     *  init variables
     */
    isLocked = true;
    toggleLockState(isLocked);

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
    /**
     * 잠겨있을 때
     */
    setStateLocked();
}

void setStateLocked() {
    String input = "";
    int poten;
    int pwCursor = 0;

    while (isLocked) {
        poten = (int)map(analogRead(PIN_POT), 0, 1023, 0, 10);
        lcd.setCursor(0, 0);
        lcd.print("input : ");
        lcd.print(poten);
        lcd.setCursor(0, 1);  // 1번 칸, 2번 줄
        lcd.print("password : ");
        for (int i = 0; i < input.length(); i++) {
            lcd.print("*");
        }

        // for (int i = 0; i < input.length(); i++) {
        //     lcd.print("*");
        //     lcd.setCursor(i, 1);  // 1번 칸, 2번 줄
        // }

        if (digitalRead(BUTTON_1)) {
            delay(300);
            if (input.length() < 4) {
                input.concat(String(poten));
                Serial.println(input);
            } else {
                input = "";
                clearLine(1);
                input.concat(String(poten));
                Serial.println(input);
            }
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

void clearLine(int line) {
    lcd.print("                ");
    if (line > 1) return;
    lcd.setCursor(0, line);
    lcd.print("                ");
    lcd.setCursor(0, line);
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
