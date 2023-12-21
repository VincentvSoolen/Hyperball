// Player.h
#ifndef PLAYER_h
#define PLAYER_h

#include <stdint.h>
#include <Arduino.h>
#include "motherboard-config.h"
#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
//#include <Stepper.h> // Libray to control unipolar or bipolar stepper motors
#include <AccelStepper.h>

class PLAYER {
  private:
    // lcd
    hd44780_I2Cexp *lcd;

    // all the pinouts
    uint8_t gamepadButtonPin;
    uint8_t gamepadJoystickPin;
    uint8_t motorStepPin;
    uint8_t motorDirPin;
    uint8_t motorEnablePin;
    uint8_t limitSwitchLeftPin;
    uint8_t limitSwitchRightPin;
    uint8_t goalSwitchPin;
    uint8_t solenoidPin;

    // player vars
    bool isJoined = false;
    bool isEnabled = false;
    uint8_t score;
    char scoreAsString[6];
    char* name;
    char* baseName;
    uint8_t playerIndex;

    uint8_t gamepadButtonValue = 0;
    int gamepadButtonSteadyState = HIGH;
    uint8_t gamepadButtonState = 1;
    bool isGamePadPressedState = false;

    // Goal settings
    int goalLastSteadyState = HIGH;       // the previous steady state from the input pin
    int goalLastFlickerableState = HIGH;  // the previous flickerable state from the input pin
    int goalCurrentState;                // the current reading from the input pin
    unsigned long goalLastDebounceTime = 0;  // the last time the output pin was toggled

    // Solenoid settings
    int solenoidLastState = LOW;          // The previous state from the input pin
    int solenoidCurrentState = LOW;       // Current reading from the input pin
    unsigned long solenoidDelayTime = 80; // The delay the solenoid should retract
    unsigned long solenoidDelayTimeBetween = 100; // The delay the solenoid should retract
    unsigned long solenoidTriggerTime = 0;    // The timestamp the solenoid has triggered
    unsigned long solenoidTriggerBetweenTime = 0;    // The timestamp the solenoid has triggered

    // Message scroll on the display
    int messageRowPos = 0;
    int messageColumnPos = 0;
    int messageLength = 0;
    int messageScreenWidth = 16; // default entire display width
    String messageToDisplay = "";
    bool isMessageScrolling = false;
    unsigned long timeScrollStarted = 0;
    unsigned long delayTime = 500; // in ms

    int joystickXValue = 0; // To store value of the X axis
    bool joystickDebouncer = false;

    bool isPlayerPickingName = false;
    bool hasPlayerPickedAname= false;
    uint8_t playernamePickerIndex = 0;

    unsigned long last_input_time = 0;
    unsigned long current_time = 0;

    // Motor settings
    // 1 or AccelStepper::DRIVER means a stepper driver (with Step and Direction pins)
    //AccelStepper stepper(AccelStepper::DRIVER, stepPin, directionPin);
    // initialize stepper library
    //Stepper *stepper; //(200, stepPin, directionPin);
    AccelStepper *stepper;

  public:
    PLAYER(
      uint8_t playerIndex,
      uint8_t score, 
      char name[], 
      uint16_t lcdAddress, 
      uint8_t gamepadButton,
      uint8_t gamepadJoystick,
      uint8_t motorStep,
      uint8_t motorDir,
      uint8_t motorEnable,
      uint8_t limitSwitchLeft,
      uint8_t limitSwitchRight,
      uint8_t goalSwitch,
      uint8_t solenoid);

    void initializeHardware();
    
    void Scored();
    bool isDead();
    bool isGamepadButtonPressed();
    void setupBaseVariables();
    
    /**
      Loop should be called as many times as you can from the main loop
      */
    void loop();

    bool getIsJoined();
    void setIsJoined(bool val);

    bool getIsEnabled();
    void setIsEnabled(bool val);

    uint8_t getPlayerNameIndex();
    void setPlayerNameIndex(uint8_t val);

    bool getHasPickedAName();
    void setHasPickedAName(bool val);
    bool getIsPickingName();
    void setIsPickingName(bool val);
    void handleNamePicking();
    int getPickedNameIndex();

    bool getGamepadPressedState();
    uint8_t getGamepadButtonState();
    void setGamepadPressedState(bool val);
    void setGamepadButtonState(uint8_t val);

    bool hasBeenScored();
    uint8_t getScore();

    void handlePlayerKick();
    void handlePlayerMovement();

    void clearLCD();
    void clearRowOnLCD(int row);
    void drawWelcomeMessageOnLCD();
    void startMessageScrollOnLCD(int row, String message, int totalColumns);
    void setMessageScrollStateOnLCD(bool val);
    bool getMessageScrollStateOnLCD();
    void scrollMessageOnLCD();
    void drawPlayernameOnLCD(int column, int row);
    void drawPlayerStatsOnLCD(int column, int row);
    void drawMessageOnLCD(int column, int row, String message);
    void drawNamePickingOnLCD(bool includeHeader);
    void drawWinnerOnScreen();
    void drawLoseOnScreen();

    int16_t getXAxisValue();
};

#endif