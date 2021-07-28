// locker 최종 소스코드

/*********************************
 * 실습에 필요한 라이브러리를 불러옵니다.
 * ******************************/
#include <LiquidCrystal_I2C.h>
#include <MFRC522.h>
#include <SPI.h>
#include <Servo.h>
#include <SoftwareSerial.h>
#include <String.h>
#include <Wire.h>

/*********************************
 * 센서가 아두이노에 연결될 핀번호를 정의합니다.
 * 0--  RX(시리얼 통신에 사용)
 * 1--  TX(시리얼 통신에 사용)
 * 2--  서보모터
 * 3--  블루투스 TX -> D1 전환 가능?(시리얼 모니터 사용X 가정)
 * 4--  멤브레인 #1
 * 5--  멤브레인 #2
 * 6--  멤브레인 #3
 * 7--  멤브레인 #4
 *
 * 8--  블루투스 RX -> D0 전환 가능?(시리얼 모니터 사용X 가정)
 * 9--  RFID-RST
 * 10-  RFID-SS
 * 11-  RFID-SPI(고정)
 * 12-  RFID-SPI(고정)
 * 13-  RFID-SPI(고정)
 *
 * A0-  조도센서
 * A1-  X
 * A2-  X
 * A3-  
 * A4-  LCD
 * A5-  LCD
 * ******************************/
#define PIN_SERVO 2      // sig = dig
#define PIN_POT 3        // sig = analog, POT : potentiometer
#define PIN_LED_GREEN 6  // sig = dig
#define RFID_RST 9       // sig = dig, RST  RESET
#define RFID_SS 10       // sig = dig, SS : SDA
#define BT_TXD 3
#define BT_RXD 8
/**
    키패드	핀
    [2]	    첫번째 핀
    [1]	    두번째 핀
    [3]	    네번째 핀
    [4]	    세번째 핀
 */
#define BUTTON_1 4  
#define BUTTON_2 5  
#define BUTTON_3 6  
#define BUTTON_4 7  
#define CDS A0

/*********************************
 * 코드에 사용될 변수들을 선언하는곳 입니다.
 *******************************/
#define BUTTON_DELAY 300  // 버튼에 딜레이를 주어 오입력 방지

enum LcdCommand { OPEN, CLOSE, UPDATE, WRONG };
enum LockerState{LOCKER_CLOSED, LOCKER_OPENED};
LockerState lockerState;  // 시스템의 현재 상태를 나타내는 변수. 

/**
 * 이전 실습을 통해 진행하였던 내용을 아래에 추가합니다.
 * registeredCard : 등록할 카드의 일련번호
 * registeredPw : 기본으로 사용할 비밀번호
 */
byte registeredCard[4] = {0x83, 0xD2, 0xE9, 0x18};  // rfid 카드 일련번호
String registeredPw = "1111";  // 사용자가 지정한 비밀번호
String keyPressed = "";        // 키패드에 입력된 값을 담는 변수

/*
 *센서/모듈 사용을 위한 클래스 변수를 선언합니다.
 */
Servo servo;
MFRC522 mfrc(RFID_SS, RFID_RST);  // Instance of the class
SoftwareSerial bluetooth(BT_TXD, BT_RXD);
LiquidCrystal_I2C lcd(0x27, 16, 2);

/*********************************
 * setup함수. 1회 실행.
 * ******************************/
void setup() {
    /**
     * 시리얼 통신을 위한 준비
     */
    Serial.begin(9600);
    bluetooth.begin(9600);
    pinMode(BUTTON_1, INPUT_PULLUP);  // 멤브레인키보드 #1
    pinMode(BUTTON_2, INPUT_PULLUP);  // 멤브레인키보드 #2
    pinMode(BUTTON_3, INPUT_PULLUP);  // 멤브레인키보드 #3
    pinMode(BUTTON_4, INPUT_PULLUP);  // 멤브레인키보드 #4
    pinMode(A2, INPUT);               // 조도센서
    SPI.begin();                      // Init SPI bus(for RFID)
    mfrc.PCD_Init();                  // Init MFRC522
    servo.attach(PIN_SERVO);          // 서보모터 연결
    lcd.init();                       // LCD 초기화
    lcd.backlight();                  // 백라이트 켜기
    lcd.setCursor(0, 0);              // 1번째, 1라인

    close();
    Serial.println("arduino starts");
}

void loop() {
    if (lockerState == LOCKER_CLOSED) {  // 금고가 잠겨있을 때
    /* 카드 리더에 카드가 인식 되었다면? */
        if (mfrc.PICC_IsNewCardPresent() && mfrc.PICC_ReadCardSerial()) {
            if (isVerified(mfrc.uid.uidByte)) { // 일련번호가 일치한다면?
                open();  // 금고 열림
                return;  // loop()함수 탈출
            } else {
                lcdHandler(LcdCommand::WRONG);  // 일련번호 불일치. lcd에 메시지 WRONG 메시지 출력
                lcdHandler(LcdCommand::UPDATE); // WRONG메시지 출력 후 다시 LCD 초기화
            }
        }
        if (bluetooth.available()) {
            String btRead = bluetooth.readStringUntil('\n');
            btHandler(btRead);
        }
        /**
         * 키패드에서 값 읽어오기
         *
         * 1. button이 눌렸는가?
         * 2. "yes" => 어떤 버튼이 눌렸나? => 해당 눌린 버튼에 대한 동작을
         * switch-case문으로 정의
         */
        for (int i = BUTTON_1; i <= BUTTON_4; i++) {
            if (!digitalRead(i)) {
                delay(BUTTON_DELAY);
                switch (i) {
                    case BUTTON_1:
                        keyPressed += "2";
                        break;
                    case BUTTON_2:
                        keyPressed += "1";
                        break;
                    case BUTTON_3:
                        keyPressed += "4";
                        break;
                    case BUTTON_4:
                        keyPressed += "3";
                        break;
                    default:
                        break;
                }
                lcdHandler(LcdCommand::UPDATE);
                break;
            }
        }

        // 입력된 숫자가 4자리에 다다랐을 때 비밀번호 대조
        if (keyPressed.length() >= 4) {
            if (keyPressed == registeredPw) {
                open();
            } else {
                lcdHandler(LcdCommand::WRONG);
            }
            keyPressed = "";
            lcdHandler(LcdCommand::UPDATE);
            return;
        }
    }

    /* 열려있는 상태 */
    if (lockerState == LOCKER_OPENED) {
        if (analogRead(CDS) < 400) {  // 조도 센서의 값이 일정 이하라면
            for(int i=0; i<3; i++){
                delay(1000);
                if(analogRead(CDS) > 400)   return;
            }
            close();
            return;
        }
    }
}

/************************************************
 * functions
 ************************************************/

/**
 * 카드키의 일련번호를 대조하는 함수
 */
bool isVerified(byte uid[]) {
    for (byte i = 0; i < 4; i++) {
        if (registeredCard[i] != uid[i]) {
            return false;
        }
    }
    return true;
}

/**
 * 블루투스 통신을 처리하는 함수
 */
void btHandler(String btRead) {
    if (btRead.length() == 4 && btRead == registeredPw &&
        lockerState == LOCKER_CLOSED) {
        open();
        return;
    }
    if (btRead == "close" && lockerState == LOCKER_OPENED) {
        close();
        return;
    }
}

/**
 * lcd를 조작하는 함수
 */
void lcdHandler(LcdCommand command) {
    switch (command) {
        case OPEN:
            lcd.clear();            lcd.print("Welcome!");
            break;

        case CLOSE:
            lcd.clear();            lcd.print("ENTER PW : ");
            lcd.setCursor(0, 1);    lcd.print("OR TAG CARD");   
            break;

        case UPDATE:
            lcd.setCursor(11, 0);
            for (int i = 0; i < keyPressed.length(); i++) {
                lcd.print("*");
            }
            break;

        case WRONG:
            lcd.setCursor(11, 0);   lcd.print("     ");
            lcd.setCursor(11, 0);   lcd.print("WRONG"); delay(750);
            lcd.setCursor(11, 0);   lcd.print("     ");
            break;

        default:
            break;
    }
}

/**
 * 금고의 잠금을 해제하는 함수
 */
void open() {
    keyPressed = "";
    moveMotor(lockerState = LOCKER_OPENED);
    lcdHandler(LcdCommand::OPEN);
}

/**
 * 금고를 잠구는 함수
 */
void close() {
    moveMotor(lockerState = LOCKER_CLOSED);
    lcdHandler(LcdCommand::CLOSE);
}

/**
 * 모터의 동작을 담당하는 함수
 */
void moveMotor(int nextState) {
    if (nextState == LOCKER_OPENED) {
        servo.write(0);
    } else {
        servo.write(180);
    }
}
