#include <Servo.h>
#include <LiquidCrystal.h>
#include <LcdManager.h>
#include <CurveFitting.h>
#include <avr/eeprom.h>

#define BUTTON1 1
#define BUTTON2 2
#define BUTTON3 3

LiquidCrystal lcd(5, 6, 10, 11, 12, 8);

// here we should first check if we actually need calibration
LcdManager LcdManagerInstance(&lcd, onMessage);

const int pinWaterPassingSensor = 9;
const int ledWaterPassing = 13;

const int pinMotor = 2;
const int pinButton1 = 3;
const int pinButton2 = 4;
const int pinButton3 = 7;

double calibrationPoints[50][2];
int calibrationPointsSize = 0;

Servo Motor;
CurveFitting CurveFittingInstance;

void onMessage(message_t action, void *param) {
    /* for loading/saving in the eeprom */
    double aParam, bParam, cParam;

    /* for MSG_POUR_ONE_UNIT */
    unsigned long timePouring;
    unsigned long timeBeforeStraw;
    unsigned long timeToStraw;
    unsigned long howLong;
    unsigned long diff;

    switch (action) {

        case MSG_INIT_MOTOR:
            Motor.write(0);
            break;

        case MSG_MOTOR_DOWN:
            Motor.write(90);
            break;

        case MSG_IS_WATER_POURING:
            *((bool*) param) = isWaterFlowing();
            break;

        case MSG_GET_PARAM_A:
            *((double*) param) = CurveFittingInstance.getEstimatedParameter(0);
            break;
        case MSG_GET_PARAM_B:
            *((double*) param) = CurveFittingInstance.getEstimatedParameter(1);
            break;
        case MSG_GET_PARAM_C:
            *((double*) param) = CurveFittingInstance.getEstimatedParameter(2);
            break;

        case MSG_POUR_ONE_UNIT:

            if (false == CurveFittingInstance.isCurveFitted()) {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("ERROR!");
                lcd.setCursor(0, 1);
                
                double paramA, paramB, paramC;

                onMessage(MSG_GET_PARAM_A, &paramA);
                onMessage(MSG_GET_PARAM_B, &paramB);
                onMessage(MSG_GET_PARAM_C, &paramC);

                lcd.print(paramA);

                return;
            }

            // begin pouring
            timePouring = millis();

            // motor goes down for exactly one unit time
            onMessage(MSG_MOTOR_DOWN, (void *) NULL);

            // maybe the water goes down so fast and we want to turn the led on
            realtimeLoop();

            // how long does the time to straw sensor to light up?
            timeBeforeStraw = millis();
            while (!isWaterFlowing()) {
                delay(20);
                realtimeLoop();
            }
            timeToStraw = millis() - timeBeforeStraw;

            // Estimate how long to hold the straw down using the interpolation function
            // f(timeToStraw) = a + b * e^(c * timeToStraw)
            howLong = (unsigned long) CurveFittingInstance.estimate((double)timeToStraw);

            // wait until howLongHas passed
            do {
                realtimeLoop();
                diff = millis() - timePouring;
                delay(20);
            } while (diff < howLong);

            realtimeLoop();

            onMessage(MSG_MOTOR_UP, (void *) NULL);
            break;

        case MSG_CALIBRATION_BEGIN:
            calibrationPointsSize = 0;
            break;

        case MSG_CALIBRATION_IS_VALID:
            // the first time is always valid
            if (calibrationPointsSize == 0) {
                *((bool *)((void **)param)[1]) = true;
            }
            else {
                double timeToStraw = ((double *)((void **)param)[0])[0];
                double strawDownTime = ((double *)((void **)param)[0])[1];
                // WRT to previous pouring we must meet:
                // - higher timeToStraw
                // - longer duration

                if (calibrationPoints[calibrationPointsSize-1][0] < timeToStraw &&
                    calibrationPoints[calibrationPointsSize-1][1] < strawDownTime) {

                    *((bool *)((void **)param)[1]) = true;
                }
                else {
                    *((bool *)((void **)param)[1]) = false;
                }
            }

            break;
            

        case MSG_CALIBRATION_STORE_POINT:
            calibrationPoints[calibrationPointsSize][0] = (((double *) param)[0]);
            calibrationPoints[calibrationPointsSize][1] = (((double *) param)[1]);
            calibrationPointsSize++;

            break;

        case MSG_CALIBRATION_END:
            // perform exponential curve fitting
            CurveFittingInstance.fitPoints(
                calibrationPoints,
                calibrationPointsSize
            );
            break;

        case MSG_CALIBRATION_SAVE:
            
            // retrieve current parameters
            onMessage(MSG_GET_PARAM_A, &aParam);
            onMessage(MSG_GET_PARAM_B, &bParam);
            onMessage(MSG_GET_PARAM_C, &cParam);
            
            eeprom_write_block((const void*)&aParam, (void*)0, sizeof(aParam));
            eeprom_write_block((const void*)&bParam, (void*)4, sizeof(bParam));
            eeprom_write_block((const void*)&cParam, (void*)8, sizeof(cParam));
            break;
            
        case MSG_CALIBRATION_LOAD:
            eeprom_read_block((void*)&aParam, (void*)0, sizeof(aParam));

            eeprom_read_block((void*)&bParam, (void*)4, sizeof(bParam));

            eeprom_read_block((void*)&cParam, (void*)8, sizeof(cParam));

            CurveFittingInstance.setParams(aParam, bParam, cParam);
            break;

        case MSG_MOTOR_UP:
            int currentPosition = 90;
            for (int i=currentPosition; i>=0; i--) {
              Motor.write(i);
              realtimeLoop();
              delay(25);
            }  
            break;
    }
}

// Circuit from http://www.electroschematics.com/9964/arduino-water-level-indicator-controller/
bool isWaterFlowing() {
    return digitalRead(pinWaterPassingSensor) == LOW;
}


void setup()
{
    LcdManagerInstance.begin();
    Motor.attach(pinMotor);

    pinMode(ledWaterPassing, OUTPUT);
    pinMode(pinWaterPassingSensor, INPUT);

    pinMode(pinButton1, INPUT);
    pinMode(pinButton2, INPUT);
    pinMode(pinButton3, INPUT);

    digitalWrite(ledWaterPassing, LOW);

    onMessage(MSG_INIT_MOTOR, NULL);
}

void realtimeLoop() {
    // update water led
    digitalWrite(ledWaterPassing, isWaterFlowing() ? HIGH : LOW);
}

void loop()
{
    LcdManagerInstance.loop();

    realtimeLoop();

    if (digitalRead(pinButton1) == HIGH) {
        LcdManagerInstance.onButtonPressed(BUTTON1);
    }
    else {
        LcdManagerInstance.onButtonReleased(BUTTON1);
    }
    delay(25);

    if (digitalRead(pinButton2) == HIGH) {
        LcdManagerInstance.onButtonPressed(BUTTON2);
    }
    else {
        LcdManagerInstance.onButtonReleased(BUTTON2);
    }
    delay(25);
    
    if (digitalRead(pinButton3) == HIGH) {
        LcdManagerInstance.onButtonPressed(BUTTON3);
    }
    else {
        LcdManagerInstance.onButtonReleased(BUTTON3);
    }
    delay(25);
}
