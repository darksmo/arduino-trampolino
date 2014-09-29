/*
 * CurveFitting.h - Library for performing curve fitting.
 * Created by Savio Dimatteo, September 18, 2014.
 * Released into the public domain.
 */

#include "LcdManager.h"
#include "Arduino.h"

LcdManager::LcdManager(LiquidCrystal *lcd, void (*notifyFunc) (message_t, void *param))
{
    _sendMessage = notifyFunc;
    _lcd = lcd;
}

void LcdManager::begin() {
    resetBitsStatus();
    _lcd->begin(16, 2);

    // sets the default parameters of the LcdState
    setDefaultState();
    setDefaultMode();
}

void LcdManager::drawModeCalibration(int progress, bool showEnd)
{
    char *str = "                ";

    sprintf(str, "Fill unit: #%d", progress);

    _lcd->setCursor(0, 0);
    _lcd->print(str);
    _lcd->setCursor(0, 1);
    _lcd->print( showEnd ? "Cancel End  Pour" : "Cancel      Pour");
}

void LcdManager::drawModeCalibrated()
{
    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print("Calibrated");
    _lcd->setCursor(0, 1);
    _lcd->print("Record Unit Pour");
}

void LcdManager::drawModeSetUnits(int displayUnits) 
{
    char *str = "                ";
    sprintf(str, "Pour %d units", displayUnits);

    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print(str);
    _lcd->setCursor(0, 1);
    _lcd->print("-       +   Next");
}

void LcdManager::drawStartAtMinutes(int minutes)
{
    // hours
    int hours = (int) ((float) minutes / (float) 60);
    int remainingMinutes = minutes - (60 * hours);

    char *str = "                ";
    sprintf(str, "Pour in %dH %dM", hours, remainingMinutes);
    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print(str);
    _lcd->setCursor(0, 1);
    _lcd->print("-       +   Next");
}

void LcdManager::decomposeMinutes(int inMinutes, int *outDays, int *outHours, int *outMinutes)
{
    int minutes = inMinutes;
    int days = (int) ((float) inMinutes / ((float) 60 * 24));
    
    minutes -= days * 24 * 60;

    int hours = (int) ((float) minutes / (float) 60);

    minutes -= hours * 60;

    *outDays = days;
    *outHours = hours;
    *outMinutes = minutes;
}

void LcdManager::drawEveryMinutes(int inMinutes)
{
    int days, hours, minutes;
    decomposeMinutes(inMinutes, &days, &hours, &minutes);

    char *str = "                ";
    if (hours == 0 && days == 0) {
        sprintf(str, "Every %d min", minutes);
    }
    else if (days > 0) {
        sprintf(str, "Ev %dD %dh %dm", days, hours, minutes);
    }
    else if (days == 0) {
        sprintf(str, "Every %dh %dm", hours, minutes);

    }

    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print(str);
    _lcd->setCursor(0, 1);
    _lcd->print("-       +   Done");
}

void LcdManager::drawModeMessage(char *msg) {
    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print(msg);
}

void LcdManager::drawModeShowParam(char paramName, double paramValue) {
    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print(paramName);
    _lcd->setCursor(2, 0);
    _lcd->print(paramValue);
    _lcd->setCursor(0, 1);
    _lcd->print("Cancel      Next");
}

void LcdManager::drawModeAutomatic(int units, int remainingMinutes)
{
    int days, hours, minutes;
    decomposeMinutes(remainingMinutes, &days, &hours, &minutes);

    char *str = "                ";

    if (days > 0) {
        sprintf(str, "%du in %dd%dh", units, days, hours);
    }
    else if (hours > 0) {
        sprintf(str, "%du in %dh%dm", units, hours, minutes);
    }
    else {
        sprintf(str, "%du in %dmin", units, minutes);
    }

    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print(str);
    _lcd->setCursor(0, 1);
    _lcd->print("Cancel          ");
}

void LcdManager::setDefaultState()
{
    this->_modeState.calibration_currentStep = 1;
    this->_modeState.setUnits_units = 1;
    this->_modeState.setStartAt_minutes = 60;
    this->_modeState.setEvery_minutes = 60;
    this->_modeState.automatic_units = 2;
    this->_modeState.automatic_remainingMinutes = 9999;
    this->_modeState.calibration_showEnd = false;
}

void LcdManager::setDefaultMode()
{
    double paramA, paramB, paramC;

    _sendMessage(MSG_GET_PARAM_A, &paramA);
    _sendMessage(MSG_GET_PARAM_B, &paramB);
    _sendMessage(MSG_GET_PARAM_C, &paramC);

    if (paramA == 0 && paramB == 0 && paramC == 0) {
        // load the parameters from the eprom 
        _sendMessage(MSG_CALIBRATION_LOAD, (void *)NULL);

        _sendMessage(MSG_GET_PARAM_A, &paramA);
        _sendMessage(MSG_GET_PARAM_B, &paramB);
        _sendMessage(MSG_GET_PARAM_C, &paramC);

        if (paramA == 0 && paramB == 0 && paramC == 0) {
            setMode(LCD_MODE_CALIBRATION);
        }
        else {
            setMode(LCD_MODE_CALIBRATED);
        }
    }
    else {
        setMode(LCD_MODE_CALIBRATED);
    }
}
void LcdManager::refreshMode()
{
    setMode(_currentMode);
}
void LcdManager::setMode(lcd_mode_t mode)
{
    double paramA, paramB, paramC;

    _lcd->clear();
    switch (mode) {
        case LCD_MODE_SHOW_PARAM_A:
            _sendMessage(MSG_GET_PARAM_A, &paramA);
            drawModeShowParam('A', paramA);
            break;
        case LCD_MODE_SHOW_PARAM_B:
            _sendMessage(MSG_GET_PARAM_B, &paramB);
            drawModeShowParam('B', paramB);
            break;
        case LCD_MODE_SHOW_PARAM_C:
            _sendMessage(MSG_GET_PARAM_C, &paramC);
            drawModeShowParam('C', paramC);
            break;
            
        case LCD_MODE_CALIBRATION:
            if (this->_modeState.calibration_currentStep == 1) {
                _sendMessage(MSG_CALIBRATION_BEGIN, (void *) NULL);
            }

            drawModeCalibration(
                this->_modeState.calibration_currentStep,
                this->_modeState.calibration_showEnd
            ); // defaults
            break;

        case LCD_MODE_MESSAGE:
            drawModeMessage(this->_modeState.message_text);
            break;

        case LCD_MODE_CALIBRATED:
            drawModeCalibrated();
            break;

        case LCD_MODE_SET_UNITS:
            drawModeSetUnits(
                this->_modeState.setUnits_units
            );
            break;

        case LCD_MODE_SET_STARTAT:
            drawStartAtMinutes(
                this->_modeState.setStartAt_minutes
            );
            break;

        case LCD_MODE_SET_EVERY:
            drawEveryMinutes(
                this->_modeState.setEvery_minutes
            );
            break;

        case LCD_MODE_AUTOMATIC:
            drawModeAutomatic(
                this->_modeState.automatic_units, 
                this->_modeState.automatic_remainingMinutes
            );
            break;

        default:
            _lcd->print("UNKNOWN MODE!");
    }

    /* save the mode */
    _currentMode = mode;
}

void LcdManager::resetBitsStatus()
{
    _bitButtonStatusBefore = 0x0;
    _bitButtonStatusAfter = 0x0;
}

/*
 * This function is called continually as long as the button is pressed
 */
void LcdManager::onButtonPressed(int button) 
{
    if (_previousButtonStatus[button-1] == HIGH) {

        // ---- this block is called repeatedly as long as the button is pushed
        if (_didWaterFlow == false) {
            _sendMessage(MSG_IS_WATER_POURING, &_didWaterFlow);
            if (_didWaterFlow) {
                _timeToStrawMillis = millis() - _strawFirstDownMillis;
            }
        }

        return;
    }

    // ---- this is called only once
    _didWaterFlow = false;
    _strawFirstDownMillis = millis();

    if (button == 3 && (
        _currentMode == LCD_MODE_CALIBRATED ||
        _currentMode == LCD_MODE_CALIBRATION
    )) {

        _sendMessage(MSG_MOTOR_DOWN, (void *) NULL);
    }

    _bitButtonStatusBefore |= (1 << (button - 1));

    _previousButtonStatus[button-1] = HIGH;
}

void LcdManager::onButtonReleased(int button) 
{
    if (_previousButtonStatus[button-1] == LOW) {
         return;
    }

    _previousButtonStatus[button-1] = LOW;

    _bitButtonStatusAfter |= (1 << (button - 1));

    if (_bitButtonStatusBefore == _bitButtonStatusAfter) {
        /*
         * all buttons up!
         */

        // special case: calibration : first 2 buttons from left pressed
        if (_bitButtonStatusBefore == 3) {
            if (_currentMode == LCD_MODE_SHOW_PARAM_A ||
                _currentMode == LCD_MODE_SHOW_PARAM_B ||
                _currentMode == LCD_MODE_SHOW_PARAM_C) {

                this->_modeState.calibration_currentStep = 1;
                setMode(LCD_MODE_CALIBRATION);    
                    
            }
            else {
                setMode(LCD_MODE_SHOW_PARAM_A);    
            }
        }

        switch (_currentMode) {
            case LCD_MODE_SHOW_PARAM_A:
                switch (_bitButtonStatusBefore) {
                    case 1: setDefaultMode(); break;
                    case 4: setMode(LCD_MODE_SHOW_PARAM_B);
                }
                break; 

            case LCD_MODE_SHOW_PARAM_B:
                switch (_bitButtonStatusBefore) {
                    case 1: setDefaultMode(); break;
                    case 4: setMode(LCD_MODE_SHOW_PARAM_C);
                }
                break;

            case LCD_MODE_SHOW_PARAM_C:
                switch (_bitButtonStatusBefore) {
                    case 1: setDefaultMode(); break;
                    case 4: setMode(LCD_MODE_SHOW_PARAM_A);
                }
                break;

            case LCD_MODE_CALIBRATED:
                switch (_bitButtonStatusBefore) {
                    case 1: setMode(LCD_MODE_SET_UNITS);                   break;
                    case 2: _sendMessage(MSG_POUR_ONE_UNIT, (void *) NULL); break;
                    case 4: _sendMessage(MSG_MOTOR_UP, (void *) NULL);      break;
                }
                break;

            case LCD_MODE_CALIBRATION:
                switch (_bitButtonStatusBefore) {

                    case 1: 
                        setMode(LCD_MODE_CALIBRATED);
                        break; 

                    case 2: // end calibration
                        _sendMessage(MSG_CALIBRATION_END, (void *) NULL);
                        _sendMessage(MSG_CALIBRATION_SAVE, (void *) NULL);
                        setMode(LCD_MODE_CALIBRATED);
                        break;

                    case 4: 

                        if (false == _didWaterFlow) {
                            sprintf(this->_modeState.message_text, "No Water!");
                            setMode(LCD_MODE_MESSAGE);

                            // note all blocks while motor goes up
                            _sendMessage(MSG_MOTOR_UP, (void *) NULL); 

                            setMode(LCD_MODE_CALIBRATION);
                        }
                        else {
                            double point[2];
                            point[0] = (double) _timeToStrawMillis;
                            point[1] = (double) (millis() - _strawFirstDownMillis);

                            _sendMessage(MSG_MOTOR_UP, (void *) NULL); 

                            // first param is the point to be checked
                            // second param is an output param
                            void *params[2];
                            bool isValidCalibrationPoint;
                            params[0] = (void *) point;
                            params[1] = (void *) &isValidCalibrationPoint;

                            _sendMessage(MSG_CALIBRATION_IS_VALID, params);

                            if (isValidCalibrationPoint) {
                                _sendMessage(MSG_CALIBRATION_STORE_POINT, point);

                                this->_modeState.calibration_currentStep++;

                                if (this->_modeState.calibration_currentStep > 5) {
                                    this->_modeState.calibration_showEnd = true;
                                }
                            }
                            else {
                                // error
                                sprintf(this->_modeState.message_text, "Invalid: retry!");
                                setMode(LCD_MODE_MESSAGE);
                                delay(1000);
                                setMode(LCD_MODE_CALIBRATION);
                            }

                            refreshMode();
                        }

                        break;
                }
                break;

            case LCD_MODE_SET_UNITS:
                switch (_bitButtonStatusBefore) {
                    case 1:
                        // decrease units
                        if (this->_modeState.setUnits_units > 1) {
                            this->_modeState.setUnits_units--;
                            refreshMode();
                        }
                        break;
                    case 2:
                        // increase units
                        if (this->_modeState.setUnits_units < 9) {
                            this->_modeState.setUnits_units++;
                            refreshMode();
                        }
                        break;
                    case 4:
                        // go next

                        /*
                         * NOTE: we will read the desired setUnits_units
                         * preference later, along with other preferences if
                         * all is done.
                         */
                        setMode(LCD_MODE_SET_STARTAT);
                        break;
                }
                break;

            case LCD_MODE_SET_STARTAT:
                switch (_bitButtonStatusBefore) {
                    case 1:
                        this->_modeState.setStartAt_minutes = decreaseMinutes(
                            this->_modeState.setStartAt_minutes
                        );
                        refreshMode();
                        break;
                    
                    case 2:
                        this->_modeState.setStartAt_minutes = increaseMinutes(
                            this->_modeState.setStartAt_minutes
                        );
                        refreshMode();
                        break;
                    case 4:
                        // go next
                        setMode(LCD_MODE_SET_EVERY);
                        break;
                }
                break;

            case LCD_MODE_SET_EVERY:
                switch (_bitButtonStatusBefore) {
                    case 1:
                        this->_modeState.setEvery_minutes = decreaseMinutes(
                            this->_modeState.setEvery_minutes
                        );
                        refreshMode();
                        break;

                    case 2:
                        this->_modeState.setEvery_minutes = increaseMinutes(
                            this->_modeState.setEvery_minutes
                        );
                        refreshMode();
                        break;

                    case 4:
                        this->_modeState.automatic_units = this->_modeState.setUnits_units;
                        this->_modeState.automatic_remainingMinutes = this->_modeState.setStartAt_minutes;

                        _timePreferencesSaved = millis();
                        setMode(LCD_MODE_AUTOMATIC);

                        break;
                }
                break;

            case LCD_MODE_AUTOMATIC:
                if (_bitButtonStatusBefore == 1) {
                    setMode(LCD_MODE_CALIBRATED);
                }
                break;
        };

        resetBitsStatus();
    }
}


int LcdManager::decreaseMinutes(int currentMinutes) {
    // decrease
    if (currentMinutes > 60 * 24) {
        currentMinutes -= 60 * 24;
    }
    else if (currentMinutes > 60) {
        currentMinutes -= 30;
    }
    else if (currentMinutes > 30) {
        currentMinutes -= 10;
    }
    else if (currentMinutes > 15) {
        currentMinutes -= 5;
    }
    else {
        currentMinutes -= 1;
    }

    if (currentMinutes <= 1) {
        currentMinutes = 1;
    }

    return currentMinutes;
}

int LcdManager::increaseMinutes(int currentMinutes) {
    if (currentMinutes >= 60 * 24) {
        currentMinutes += 60 * 24;
    }
    else if (currentMinutes >= 60) {
        currentMinutes += 30;
    }
    else if (currentMinutes >= 30) {
        currentMinutes += 10;
    }
    else if (currentMinutes >= 15) {
        currentMinutes += 5;
    }
    else {
        currentMinutes += 1;
    }

    return currentMinutes;
}

void LcdManager::loop()
{
    /*
     * In automatic mode we must decrease the next counter until it reaches 0.
     */

    if (_currentMode == LCD_MODE_AUTOMATIC) {

        unsigned long  millisSincePreferencesSet = (millis() - _timePreferencesSaved);

        int minutesPassed = (int) ((double)millisSincePreferencesSet / (double) 60000);
        int minutesToGo = this->_modeState.automatic_remainingMinutes - minutesPassed;
        if (minutesToGo != this->_modeState.automatic_remainingMinutes) {
            this->_modeState.automatic_remainingMinutes = minutesToGo;
            refreshMode();
            _timePreferencesSaved = millis();
        }

        if (minutesToGo <= 0) {               // won't go through next time...
            _timePreferencesSaved = millis(); // ... because we reset this one!

            // pour the units
            int howManyUnits = _modeState.automatic_units;
            while (this->_modeState.automatic_units-- > 0) {
                _sendMessage(MSG_POUR_ONE_UNIT, (void *)NULL);
                refreshMode();
                delay(2000);
            }

            this->_modeState.automatic_remainingMinutes = this->_modeState.setEvery_minutes;
            this->_modeState.automatic_units = howManyUnits;

            refreshMode();
        }
    }
}
