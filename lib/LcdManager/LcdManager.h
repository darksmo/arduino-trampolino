/*
 * LcdManager.h - Library for managing various screens and button presses.
 * Created by Savio Dimatteo, September 18, 2014.
 * Released into the public domain.
 */

#ifndef LcdManager_h 
#define LcdManager_h

#define DISPLAY_TIMEOUT_MILLIS 120000

#include "Arduino.h"
#include <LiquidCrystal.h>

enum message_t {
    MSG_INIT_MOTOR,
    MSG_CALIBRATION_BEGIN,
    MSG_CALIBRATION_STORE_POINT,
    MSG_CALIBRATION_END,
    MSG_MOTOR_DOWN,
    MSG_MOTOR_UP,
    MSG_POUR_ONE_UNIT,
    MSG_IS_WATER_POURING,
    MSG_GET_PARAM_A,
    MSG_GET_PARAM_B,
    MSG_GET_PARAM_C,
    MSG_CALIBRATION_IS_VALID,
    MSG_CALIBRATION_SAVE,
    MSG_CALIBRATION_LOAD
};

enum lcd_mode_t {
    LCD_MODE_CALIBRATION,
    LCD_MODE_MESSAGE,
    LCD_MODE_FILL_WATER,
    LCD_MODE_CALIBRATED,
    LCD_MODE_SET_UNITS,
    LCD_MODE_SET_STARTAT,
    LCD_MODE_SET_EVERY,
    LCD_MODE_AUTOMATIC,
    LCD_MODE_SHOW_PARAM_A,
    LCD_MODE_SHOW_PARAM_B,
    LCD_MODE_SHOW_PARAM_C
};

class LcdManager 
{
    public:
        LcdManager(LiquidCrystal *lcd, void (*notifyFunc) (message_t, void *param));
        void begin();
        void onButtonPressed(int button);
        void onButtonReleased(int button);
        void loop();
    private:
        // keeps the current screen displayed
        lcd_mode_t _currentMode;

        void resetBitsStatus();
        void drawModeMessage(char *msg);
        void drawModeCalibration(int progress, bool showEnd);
        void drawModeCalibrated();
        void drawModeSetUnits(int displayUnits);
        void drawEveryMinutes(int minutes);
        void drawStartAtMinutes(int minutes);
        void drawModeAutomatic(int units, int remainingMinutes);
        void drawModeShowParam(char paramName, double paramValue);
        void setMode(lcd_mode_t mode);
        void setDefaultMode();
        void refreshMode();

        int decreaseMinutes(int currentMinutes);
        int increaseMinutes(int currentMinutes);

        LiquidCrystal *_lcd;

        // the messaging bus
        void (*_sendMessage)(message_t, void *);

        // byte fields used to detect which buttons are pressed
        int _bitButtonStatusAfter;
        int _bitButtonStatusBefore;

        int _previousButtonStatus[3] = {LOW, LOW, LOW};

        // used to detect water flowing between button press
        bool _didWaterFlow;

        // time the straw went down
        unsigned long _strawFirstDownMillis;

        // time the water took to reach the straw
        unsigned long _timeToStrawMillis;

        // time at which the preferences were first saved
        unsigned long _timePreferencesSaved;

        // decomposes minutes into days, hours, minutes
        void decomposeMinutes(int inMinutes, int *outDays, int *outHours, int *outMinutes);

        /*
         * Mode state
         *
         * These variables represent the state for each mode.
         * Internally the program should:
         *
         * 1) change the state (i.e., modify these variables);
         *
         * 2) call setMode(mode) or refreshMode() to update the display.
         */
        void setDefaultState();

        struct {
            int calibration_currentStep;
            bool calibration_showEnd;
            int setUnits_units;
            int setStartAt_minutes;
            int setEvery_minutes;
            int automatic_units;
            int automatic_remainingMinutes;
            char * message_text = "                ";
        } _modeState;
};

#endif
