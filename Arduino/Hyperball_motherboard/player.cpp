// PLAYER.cpp
#include "PLAYER.h"
#include "player_names.h"
#include "motherboard-config.h"
#include <Time.h>

// Special chars for the gamepad display
//uint8_t bell[8]  = {0x04,0x0e,0x0e,0x0e,0x1f,0x00,0x04,0x00};
//uint8_t note[8]  = {0x02,0x03,0x02,0x0e,0x1e,0x0c,0x00,0x00};
//uint8_t clockface[8] = {0x00,0x0e,0x15,0x17,0x11,0x0e,0x00,0x00};
//uint8_t heart[8] = {0x00,0x0a,0x1f,0x1f,0x0e,0x04,0x00,0x00};
//uint8_t duck[8]  = {0x00,0x0c,0x1d,0x0f,0x0f,0x06,0x00,0x00};
//const uint8_t check[8] = {0x00,0x01,0x03,0x16,0x1c,0x08,0x00,0x00};
//const uint8_t cross[8] = {0x00,0x1b,0x0e,0x04,0x0e,0x1b,0x00,0x00};
//uint8_t smile[8] = {0x00,0x0a,0x0a,0x00,0x00,0x11,0x0e,0x00};
uint8_t arrowLeft[8] = {0x03,0x06,0x0C,0x18,0x18,0x0C,0x06,0x03};
uint8_t arrowRight[8] = {0x18,0x0C,0x06,0x03,0x03,0x06,0x0C,0x18};
uint8_t heart[8] = {0x00,0x0A,0x1F,0x1F,0x1F,0x0E,0x04,0x00};
uint8_t cup[8] = {0x00,0x1F,0x0E,0x0E,0x04,0x04,0x0E,0x00};
uint8_t sadface[8] = {0x00,0x00,0x0A,0x00,0x00,0x0E,0x11,0x00};
uint16_t timeElapsed;

PLAYER::PLAYER(
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
  uint8_t solenoid) 
{
  // set our basic player vars
  this->score = score;
  this->name  = name;
  this->baseName = name;
  this->playerIndex = playerIndex;

  // create the LCD object
  lcd = new hd44780_I2Cexp(lcdAddress, LCD_COLS, LCD_ROWS);

  // store the output pins for later use
  gamepadButtonPin = gamepadButton;
  gamepadJoystickPin = gamepadJoystick;
  motorStepPin = motorStep;
  motorDirPin = motorDir;
  motorEnablePin = motorEnable;
  limitSwitchLeftPin = limitSwitchLeft;
  limitSwitchRightPin = limitSwitchRight;
  goalSwitchPin = goalSwitch;
  solenoidPin = solenoid;

  // class init
  //this->gamepadButton = new ezButton(gamepadButtonPin, INPUT_PULLUP);
  //this->gamepadButton->setDebounceTime(100);
}

void PLAYER::initializeHardware() {
  // setup our pinouts for the player
  pinMode(gamepadButtonPin, INPUT_PULLUP);
  pinMode(gamepadJoystickPin, OUTPUT);
  pinMode(motorStepPin, OUTPUT);
  pinMode(motorDirPin, OUTPUT);
  pinMode(motorEnablePin, OUTPUT);
  pinMode(limitSwitchLeftPin, INPUT_PULLUP);
  pinMode(limitSwitchRightPin, INPUT_PULLUP);
  pinMode(goalSwitchPin, INPUT_PULLUP);
  pinMode(solenoidPin, OUTPUT);

  // disable the motors
  digitalWrite(motorEnablePin, HIGH);

  // initialize LCD with number of columns and rows
	int status = lcd->begin(LCD_COLS, LCD_ROWS);
	if(status) // non zero status means it was unsuccesful
	{
		// hd44780 has a fatalError() routine that blinks an led if possible
		// begin() failed so blink error code using the onboard LED if possible
		hd44780::fatalError(status); // does not return
	}
	// initalization was successful, the backlight should be on now

  // Intialize the motor
  this->stepper = new AccelStepper(AccelStepper::DRIVER, motorStepPin, motorDirPin);
  this->stepper->setMaxSpeed(MOTOR_MAX_SPEED);
  this->stepper->setAcceleration(10000);
  //this->stepper = new Stepper(200, motorStepPin, motorDirPin);

	// create custom characters
	lcd->createChar(0, arrowLeft);
	lcd->createChar(1, arrowRight);
	lcd->createChar(2, cup);
  lcd->createChar(3, sadface);
}

void PLAYER::setupBaseVariables() {
  digitalWrite(motorEnablePin, HIGH); // Motor disabled - Can so only move in other direction

  this->name = baseName;
  this->score = 10;
}

void PLAYER::Scored() {
  score--;
}

bool PLAYER::isDead() {
  return score <= 0;
}

bool PLAYER::getIsJoined() { return isJoined; }
void PLAYER::setIsJoined(bool val) { 
  isJoined = val; 

  // when the player is joined, we can confirm the name for the player
  if (val) {
    this->name = availableNames[playernamePickerIndex];
  }
}

bool PLAYER::getIsEnabled() { return isEnabled; }
void PLAYER::setIsEnabled(bool val) { isEnabled = val; }

uint8_t PLAYER::getPlayerNameIndex() { return playernamePickerIndex; }
void PLAYER::setPlayerNameIndex(uint8_t val) { playernamePickerIndex = val; }

uint8_t PLAYER::getGamepadButtonState() { return gamepadButtonState; }
void PLAYER::setGamepadButtonState(uint8_t val) { gamepadButtonState = val; }

bool PLAYER::getGamepadPressedState() { return isGamePadPressedState; }
void PLAYER::setGamepadPressedState(bool val) { isGamePadPressedState = val; }

bool PLAYER::getHasPickedAName() { return hasPlayerPickedAname; };
void PLAYER::setHasPickedAName(bool val) { hasPlayerPickedAname = val; };
bool PLAYER::getIsPickingName() { return isPlayerPickingName; };
void PLAYER::setIsPickingName(bool val) { isPlayerPickingName = val; }
int PLAYER::getPickedNameIndex() { return playernamePickerIndex; };

bool PLAYER::isGamepadButtonPressed() { 
  bool isPressed = false;
  gamepadButtonValue = digitalRead(gamepadButtonPin); 
  if (gamepadButtonSteadyState == HIGH && gamepadButtonValue == LOW)
    isPressed = true;

  gamepadButtonSteadyState = gamepadButtonValue;
  return isPressed;
}

/**
  Check if there has been scored at the players location
  */
bool PLAYER::hasBeenScored() {
  bool hasScored = false;
  goalCurrentState = digitalRead(goalSwitchPin);
  if (goalLastSteadyState == HIGH && goalCurrentState == LOW)
    hasScored = true;

  // save the the last state
  goalLastSteadyState = goalCurrentState;

  if (hasScored) { 
    this->score--; // The player loses a point
    this->drawPlayerStatsOnLCD(0, 1);
  }
  return hasScored;
}

/**
  Gets the current Score from the player
  */
uint8_t PLAYER::getScore() { return this->score; }

/**
  This check if the button of the gamepad is pressed, if so the Solenoid is fired.
  Loop should be called in order to actually fire of the Solenoid
  */
void PLAYER::handlePlayerKick() {
  // check if we pressed the gamepad button
  gamepadButtonValue = digitalRead(gamepadButtonPin); // 1 == not pressed, 0 == pressed  
  if(gamepadButtonValue == LOW && gamepadButtonState == 1){ // If the button to trigger the solenoid is pressed on the gamepad
  
    if(gamepadButtonState == 1 && this->solenoidCurrentState == LOW){ // This prevents from keeping the button pressed, it would heat up the solenoid
      this->solenoidCurrentState = HIGH; // trigger the solenoid
    }
    gamepadButtonState = 0; // The solenoid has been activated, its state is set to 0, the state will return to 1 when the button is no longer pressed             
  } else if (gamepadButtonValue == HIGH && this->solenoidCurrentState == LOW) {
    gamepadButtonState = 1; // The button to trigger the solenoid is not pressed, the state is 1 wich means "Ready"
  }
}

/**
  This checks the joystick if the ship from the player should move. Boundries are taken into account here with the switches.
  */
void PLAYER::handlePlayerMovement() {
  // read the joystick value
  joystickXValue = this->getXAxisValue();
  
  //Serial.println(joystickXValue);

  // Only update at intervals, this lets the motor correctly accelerate
  if (current_time - last_input_time > INPUT_READ_INTERVAL) 
  { 
    int speed_ = map(abs(joystickXValue), 0, MAX_VALUE, MOTOR_MIN_SPEED, MOTOR_MAX_SPEED); 

    //Center point
    if (joystickXValue == 0) { 
      stepper->setCurrentPosition(0);  // Accellstepper
      stepper->setMaxSpeed(0);  // Accellstepper
      stepper->moveTo(0);
    }
    else if (joystickXValue >= JOYSTICK_DEAD_ZONE) {
      stepper->setMaxSpeed(speed_);
      if (digitalRead(limitSwitchRightPin) == HIGH){ // If limit switch left is not triggered
        digitalWrite(motorEnablePin, LOW); // Motor enabled
        stepper->moveTo(-1000000000); // Accellstepper
        //stepper->move(-1);
      }
    }

    else if (joystickXValue <= -JOYSTICK_DEAD_ZONE) {
      stepper->setMaxSpeed(speed_); // Accellstepper
      if (digitalRead(limitSwitchLeftPin) == HIGH){ // If limit switch left is not triggered
        digitalWrite(motorEnablePin, LOW); // Motor enabled
        stepper->moveTo(1000000000); // Accellstepper
        //stepper->move(1);
      }
    }
    else {
      stepper->setMaxSpeed(0);  // Accellstepper
      //digitalWrite(motorEnablePin, HIGH); // Motor disabled
    }

    last_input_time = current_time; 
  }

  // Check if the limits has been reached
  if (
    ((joystickXValue >= JOYSTICK_DEAD_ZONE || joystickXValue == 0) && digitalRead(limitSwitchRightPin) != HIGH) ||
    ((joystickXValue <= -JOYSTICK_DEAD_ZONE || joystickXValue == 0) && digitalRead(limitSwitchLeftPin) != HIGH)) {
    digitalWrite(motorEnablePin, HIGH); // Motor disabled - Can so only move in other direction
    stepper->setCurrentPosition(0);  // Accellstepper
    stepper->stop(); // Accellstepper      
  }
  
  //stepper->runSpeed();  // Run at a constant speed
  stepper->run();  // Run with acceleration

  // Check if we can disable the motor
  if(!stepper->isRunning())
    digitalWrite(motorEnablePin, HIGH); // Motor disabled
}

void PLAYER::loop() {
  current_time = millis(); 

  // Check if the solenoid need to trigger
  if (this->solenoidCurrentState == HIGH && this->solenoidLastState == LOW && (millis() > (this->solenoidTriggerBetweenTime + this->solenoidDelayTimeBetween))) {
    digitalWrite(solenoidPin, HIGH); // The solenoid is on
    this->solenoidTriggerTime = millis(); // the current timestamp
    this->solenoidLastState = HIGH;
  }
  // Check if the previous state was high and the delay time has passed
  else if (this->solenoidLastState == HIGH && (millis() > (this->solenoidTriggerTime + this->solenoidDelayTime))) {
    digitalWrite(solenoidPin, LOW); // The solenoid is off
    this->solenoidLastState = LOW;
    this->solenoidCurrentState = LOW;
    this->solenoidTriggerBetweenTime = millis(); // the current timestamp
  }

  // check for movement
}

// REGION LCD Functions -----------------------------------------------------------------------------------

/**
  Clear the Players LCD screen
  */
void PLAYER::clearLCD() { lcd->clear(); }

void PLAYER::drawWelcomeMessageOnLCD() {
  // Print a message to the LCD
  lcd->clear();
  lcd->setCursor(0, 0);
  lcd->print(F("   Welcome to   "));
  lcd->setCursor(0, 1);
  lcd->print(F("   HYPERBALL   "));
}

/**
  Start an animated message scroll on the display. This moves from right to left.
  @row: the row on the display
  @message: The message to display
  @totalColumns: The amount of columns it uses to display the message
  */
void PLAYER::startMessageScrollOnLCD(int row, String message, int totalColumns) {
  if (this->getMessageScrollStateOnLCD()) {
    return;
  } 
  this->setMessageScrollStateOnLCD(true);
  timeScrollStarted = millis();
  messageToDisplay = message;
  for (int i=0; i < totalColumns; i++) {
    messageToDisplay = " " + messageToDisplay; // move it to the right (out of the screen) 
  }
  messageToDisplay = messageToDisplay + " "; 
  messageLength = messageToDisplay.length();
  
  // set start x pos
  messageRowPos = row;
  messageScreenWidth = totalColumns;
  messageColumnPos = totalColumns;
}

/**
  Sets the state of the message scroll. False = stopped, True = scrolling
  */
void PLAYER::setMessageScrollStateOnLCD(bool val) {
  isMessageScrolling = val;
}

/**
  Sets the state of the message scroll. False = stopped, True = scrolling
  */
bool PLAYER::getMessageScrollStateOnLCD() { return isMessageScrolling; }

/**
  Animate the message scroll. Should be called after startMessageScroll
  */
void PLAYER::scrollMessageOnLCD() {
  if (!this->getMessageScrollStateOnLCD()) {
    return;
  } 
  // Check if the time has passing to scroll a letter
  if ((millis() - timeScrollStarted) < delayTime) return;

  timeScrollStarted = millis(); // update the timer

  // for (int position = 0; position < message.length(); position++) {
  lcd->setCursor(0, messageRowPos);
  lcd->print(messageToDisplay.substring(messageColumnPos, messageColumnPos + messageScreenWidth));
  messageColumnPos++;
  
  // check for the end so we start over
  if (messageColumnPos >= messageLength)
    messageColumnPos = 0; 
}

/**
  Draws the current player on the given row and column  (Player #...)
  @column: the column on the display
  @row: the row on the diplay
  */
void PLAYER::drawPlayernameOnLCD(int column, int row) {
  //lcd->setCursor(column, row);
  //lcd->print(F("Player: "));
  lcd->setCursor(column, row);
  lcd->print(name);
}

/**
  Draws the current player stats on the given row and column
  @column: the column on the display
  @row: the row on the diplay
  */
void PLAYER::drawPlayerStatsOnLCD(int column, int row) {
  lcd->setCursor(column, row);
  lcd->print(F("Lives left: "));
  lcd->setCursor(column + 12, row);
  
  sprintf(scoreAsString, "%2d", score);
  lcd->print(scoreAsString);
}

/**
  Clears a whole row on the LCD screen
  **/
void PLAYER::clearRowOnLCD(int row) {
  lcd->setCursor(0, row);
  lcd->print("                ");
}

/**
  Draws a message on the given row and column
  @column: the column on the display
  @row: the row on the diplay
  @message: the string that should be displayed
  */
void PLAYER::drawMessageOnLCD(int column, int row, String message) {
  lcd->setCursor(column, row);
  lcd->print(message);
}

/**
  Draws the text WINNER on the LCD with special charcters
  This will be drawn on the second line, leaving their name on the first
  */
void PLAYER::drawWinnerOnScreen() {
    lcd->setCursor(0, 1);
    lcd->write(2); // cup character
    lcd->setCursor(15, 1);
    lcd->write(2); // cup character

    lcd->setCursor(1, 1);
    lcd->print(F(" THE WINNER!! "));
}

/**
  Draws the text LOSE on the LCD with special charcters
  This will be drawn on the second line, leaving their name on the first
  */
void PLAYER::drawLoseOnScreen() {
    lcd->setCursor(0, 1);
    lcd->write(3); // sad character
    lcd->setCursor(15, 1);
    lcd->write(3); // sad character

    lcd->setCursor(1, 1);
    lcd->print(F("  YOU LOSE!!  "));
}

/**
  Draw the currently selected name for picking onto the LCD screen
  */
void PLAYER::drawNamePickingOnLCD(bool includeHeader) {
  char* n = availableNames[playernamePickerIndex];
  size_t length = strlen(n); // length of the name
  size_t lcdStartIndex = (16 / 2) - (length / 2);

  if (includeHeader) {
    this->clearLCD();
    lcd->setCursor(0, 0);
    lcd->print(F("CHOOSE YOUR NAME"));
    lcd->setCursor(0, 1);
    lcd->write(0); // arrow left
    lcd->setCursor(15, 1);
    lcd->write(1); // arrow right
  }

  // clear the name
  lcd->setCursor(1, 1);
  lcd->print("              ");

  // now write the name
  lcd->setCursor(lcdStartIndex, 1);
  lcd->print(n);
}
// END REGION

void PLAYER::handleNamePicking() {
  // read the joystick value
  joystickXValue = this->getXAxisValue();
  int currentPickerIndex = playernamePickerIndex;

  // check for left
  if (joystickXValue <= -150 && !joystickDebouncer) {
    joystickDebouncer = true;
    currentPickerIndex++;
    if (currentPickerIndex >= totalNamesAvailable) 
      currentPickerIndex = 0;
  }
  // check right value
  else if (joystickXValue >= 150 && !joystickDebouncer) {
    joystickDebouncer = true;
    currentPickerIndex--;
    if (currentPickerIndex < 0) 
      currentPickerIndex = totalNamesAvailable - 1;
  }
  // back to center
  else if (joystickXValue > -50 && joystickXValue < 50) {
    joystickDebouncer = false;
  }

  // check if our index has changed
  if (currentPickerIndex != playernamePickerIndex) {
    playernamePickerIndex = currentPickerIndex;

    // draw the new name on the LCD
    this->drawNamePickingOnLCD(false);
  }
}

int16_t PLAYER::getXAxisValue()
{
    //read current joystick axis position.
    int16_t raw_value = analogRead(gamepadJoystickPin);
    
    // Used to print the raw joystick value onto the LCD screen, used for testing!
    // ****************
    //if (timeElapsed < (millis() + 1000)) {
    //  timeElapsed = millis();
    //  sprintf(scoreAsString, "%4d", raw_value);
    //  this->drawMessageOnLCD(0, 1, scoreAsString);
    //}
    // ****************

    //apply offset.
    // raw_value -= offset;

    //shift the range from 0 - 1023 to -512 to 512.
    int16_t axis_value = map(raw_value, JOYSTICK_X_MIN, JOYSTICK_X_MAX, -MAX_VALUE, MAX_VALUE);

    //constrain values to -512 to 512 range.
    axis_value = constrain(axis_value, -MAX_VALUE, MAX_VALUE);

    //deadzone
    if (abs(axis_value) < JOYSTICK_DEAD_ZONE)
      return 0;

    return axis_value;
}
