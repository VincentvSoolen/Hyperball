#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header
#include <Stepper.h> // Libray to control unipolar or bipolar stepper motors
#include "SerialTransfer.h";
#include "motherboard-config.h";
#include "player.h";

// Define the player objects
PLAYER *Players[] = {
  new PLAYER(0, 10, "BLUE", LCD_BLUE, BTN_BLUE, JOY_X_BLUE, MOTOR_STEP_BLUE, MOTOR_DIR_BLUE, MOTOR_ENA_BLUE, LIMIT_LEFT_BLUE, LIMIT_RIGHT_BLUE, GOAL_BLUE, SOLENOID_BLUE),
  new PLAYER(1, 10, "GREEN", LCD_GREEN, BTN_GREEN, JOY_X_GREEN, MOTOR_STEP_GREEN, MOTOR_DIR_GREEN, MOTOR_ENA_GREEN, LIMIT_LEFT_GREEN, LIMIT_RIGHT_GREEN, GOAL_GREEN, SOLENOID_GREEN),
  new PLAYER(2, 10, "PURPLE", LCD_PURPLE, BTN_PURPLE, JOY_X_PURPLE, MOTOR_STEP_PURPLE, MOTOR_DIR_PURPLE, MOTOR_ENA_PURPLE, LIMIT_LEFT_PURPLE, LIMIT_RIGHT_PURPLE, GOAL_PURPLE, SOLENOID_PURPLE),
  new PLAYER(3, 10, "ORANGE", LCD_ORANGE, BTN_ORANGE, JOY_X_ORANGE, MOTOR_STEP_ORANGE, MOTOR_DIR_ORANGE, MOTOR_ENA_ORANGE, LIMIT_LEFT_ORANGE, LIMIT_RIGHT_ORANGE, GOAL_ORANGE, SOLENOID_ORANGE)
};
uint8_t playerCount = 4;

/* Some game ideas for the future
  Solo:
  - Time based
  - Overview of how many times kicked
  - AI is not possible as we have no camera's etc to monitor the ball

  Multi:
  - Now we only have 1v1v1v1
  - Team based > 1v2; 2v2; 3v1 etc.
    - Let the players choose their team at the start
*/
uint8_t gameType = 0; // 0 == Normal game play, 1v1 / 1v1v1 etc.. ||  1 == single play, the player will play solo

// Used for the communication between boards
SerialTransfer myTransfer;

// Struct used in the communication between devices
struct __attribute__((__packed__)) STRUCT {
  int16_t p;  // defines the player index
  int16_t y;  // defines the type
  int16_t v;  // defines the value
};
STRUCT Board_communication;

/** 
  0 = INIT, waiting for reply from main
  1 = JOIN, waiting for key press to join the game
  2 = JOINED, waiting for others
  3 = GAME STARTING, countdown for the game to start
  5 = GAME IN PROGRESS, able to move etc (playing the game)
  7 = ENDED, game is finished
  10 = IDLE, not joined the game (waiting on message from main)
  */
int16_t current_game_state = 0;
int16_t countDown = 3;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Communication between the devices
  Serial1.begin(115200);
  myTransfer.begin(Serial1);

  // Initialize the hardware for the players
  for (int i = 0; i < playerCount; i++) {
    Players[i]->initializeHardware();
  }
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  // loop through the players loop
  for (int i = 0; i < playerCount; i++) {
    Players[i]->loop();
  }

  // 0 = INIT, waiting for reply from main
  if (current_game_state == 0) {
    handleInitState();
  }
  // 1 = JOIN, waiting for key press to join the game
  else if (current_game_state == 1) {
    handleWaitingToJoinState();
  }
  // 3 = GAME STARTING, countdown for the game to start
  else if (current_game_state == 3)
  {
    handleCountDown();
  }
  // 5 = GAME IN PROGRESS, able to move etc (playing the game)
  else if (current_game_state == 5)
  {
    handleGamePlayingState();
  }
  // 7 = FINISHED, the current game is finised
  else if (current_game_state == 7)
  {
    handleFinishedState();
  }
} // End of loop

// REGION State Handling --------------------------------------------------------------------------------------

/** 
  Handles the initialization phase of te board.
  */
void handleInitState() {
  for (int i = 0; i < playerCount; i++) {
    Players[i]->setupBaseVariables();
    Players[i]->drawWelcomeMessageOnLCD();
  }

  // Check if we have received a state change
  while (current_game_state == 0) {
    if (!receiveMessageFromMaster()) {
      delay(50);
    }  
  }

  //current_game_state = 5;
  //gameType = 1; // 0 = vs, 1 = solo play (for testing individual)
  //Players[0]->setPlayerNameIndex(0);
  //Players[0]->setIsJoined(true);
  //Players[1]->setPlayerNameIndex(3);
  //Players[1]->setIsJoined(true);
  //Players[2]->setPlayerNameIndex(7);
  //Players[2]->setIsJoined(true);
  //Players[3]->setPlayerNameIndex(11);
  //Players[3]->setIsJoined(true);
  //delay(1000);

  // clear the players display, when this state is done
  for (int i = 0; i < playerCount; i++) {
    PLAYER *p = Players[i];
    p->clearLCD();
    //p->drawPlayernameOnLCD(0, 0);
    p->drawMessageOnLCD(0, 0, "    WELCOME     ");
    p->startMessageScrollOnLCD(1, "Press the button to join", 16);
  }
}

bool isInPressedState = true;

/** 
  In this state the board is waiting for the user to press the Gamepad button.
  If the button is pressed, a message is send to the master and this state will wait for response
  */
void handleWaitingToJoinState() {

  // keep the loop contained in this state for now
  while (current_game_state == 1) 
  {
    for (int i = 0; i < playerCount; i++)
    {
      PLAYER *p = Players[i];
      p->loop();

      // Scrolls the message on the LCD if needed
      p->scrollMessageOnLCD();

      // if the current player is already joined continue to the next
      if (p->getIsJoined()) continue;

      if (p->getGamepadPressedState() && !p->isGamepadButtonPressed())
        p->setGamepadPressedState(false);

      // check if that player has pressed the button
      if (p->isGamepadButtonPressed() && !p->getGamepadPressedState() && !p->getIsPickingName() && !p->getHasPickedAName()) {
        // disable the scroll and init the pick name feature
        p->setMessageScrollStateOnLCD(false);
        p->setIsPickingName(true);
        p->drawNamePickingOnLCD(true); // draw the first name

        p->setGamepadPressedState(true);

        //sendMessageToMaster(i, 1, 1);
        //p->setGamepadButtonState(0);
      }
      // else if the user is picking a name
      else if (p->getIsPickingName() && !p->getHasPickedAName()) {
        // then handle the name picking
        p->handleNamePicking();

        // check if the button is pressed, if so we try to set that name at the master
        if (p->isGamepadButtonPressed() && !p->getGamepadPressedState()) {
          p->setGamepadPressedState(true);
          p->setHasPickedAName(true);
          sendMessageToMaster(i, 1, p->getPickedNameIndex());

          // then we wait for a message back from the master
          p->drawMessageOnLCD(0, 1, "CONFIRMING NAME ");
        }
      }
    }

    // check if we got a message from the master
    receiveMessageFromMaster();
  }

  // Determine if we got a solo gameplay
  uint8_t playersJoined = 0;

  // clear and prepare for the next state
  for (int i = 0; i < playerCount; i++) {
    PLAYER *p = Players[i];
    if (!p->getIsJoined()) {
      p->setMessageScrollStateOnLCD(false); // reset just to be sure
      p->clearLCD();

      p->drawMessageOnLCD(0, 0, "HYPERBALL");
      p->drawMessageOnLCD(0, 1, "Game in progress");
      continue;
    }

    playersJoined++; // for the gameplay type

    p->setMessageScrollStateOnLCD(false); // stop the current message scroll
    p->setGamepadButtonState(1); // reset the button state
    p->setGamepadPressedState(false);
    p->clearLCD(); // clear the lcd for the next state
  }

  // set our gametype based on the players joined
  gameType = playersJoined > 1 ? 0 : 1;
  
  delay(200);
}

/**
  Handles the countdown on the screen
  The countdown will come from the master
  */
void handleCountDown() {
  // clear the gamepad display and show the players name
  for (int i = 0; i < playerCount; i++) {
    PLAYER *p = Players[i];
    if (!p->getIsJoined()) continue;

    p->drawMessageOnLCD(0, 0, " GAME START IN  ");
  }

  // Keep the game in the state for the countdown
  while (current_game_state == 3) {
    for (int i = 0; i < playerCount; i++) {
      PLAYER *p = Players[i];
      if (!p->getIsJoined()) continue;
      p->drawMessageOnLCD(7, 1, String(countDown));
    }

    receiveMessageFromMaster();
    delay(50);
  }
}

/**
  Handle the playing state. While the player still has lives left, he can move and use the bumper
  When the player died, it moves to the died gamestate
  */
void handleGamePlayingState() {
  
  for (int i = 0; i < playerCount; i++) {
    PLAYER *p = Players[i];
    if (!p->getIsJoined()) continue;

    p->clearLCD(); // clear the lcd 
    p->drawPlayernameOnLCD(0, 0);
    p->drawPlayerStatsOnLCD(0, 1);
  }

  // Playing state
  while (current_game_state == 5) {
    for (int i = 0; i < playerCount; i++) {
      PLAYER *p = Players[i];
      if (!p->getIsJoined()) {
        continue;
      } 
      if (p->isDead()) {
        // controlls are disabled, but do the message scroll
        // p->scrollMessageOnLCD();
        continue;
      }

      // player update
      p->loop();

      // chech if there has been scored at the players location
      if (p->hasBeenScored()) {
        // Send it to the server to update the display
        sendMessageToMaster(i, 3, p->getScore());

        // check if the player is dead
        if (p->isDead()) {
          p->clearRowOnLCD(1);
          p->drawMessageOnLCD(0, 1, "You are deadmeat");
        }
      }

      if (!p->isDead()) {
        // handle the solenoid kick
        p->handlePlayerKick();

        // check if we need to move the ship
        p->handlePlayerMovement();
      }
    }

    uint8_t activePlayerCount = 0;
    uint8_t deadPlayerCount = 0;
    for (int i = 0; i < playerCount; i++) {
      if (Players[i] != NULL && Players[i]->getIsJoined()) {
        activePlayerCount++;
        if (Players[i]->isDead())
          deadPlayerCount++;
      }
    }
    // If its a solo play and all players are dead
    if (gameType == 1 && activePlayerCount == deadPlayerCount) {
      uint8_t winningPlayerIndex = 0;
      // Game ends
      for (int i = 0; i < playerCount; i++) {
        if (Players[i] != NULL && Players[i]->getIsJoined() && Players[i]->isDead()) {
          Players[i]->setMessageScrollStateOnLCD(false); // disable the dead scroll
          Players[i]->drawLoseOnScreen();

          winningPlayerIndex = i;
        }
      }
      delay(1000); // Let the display process the score update first

      // Send to the display that the game is finished
      sendMessageToMaster(0, 5, -1);

      current_game_state = 7; // go to the finished state

    } else if (gameType == 0 && activePlayerCount - deadPlayerCount == 1) {
      uint8_t winningPlayerIndex = 0;
      // Game ends if only one active player remains
      // show on the controller that that player is the winner
      for (int i = 0; i < playerCount; i++) {
        if (Players[i] != NULL && Players[i]->getIsJoined() && !Players[i]->isDead()) {
          Players[i]->drawWinnerOnScreen();
          winningPlayerIndex = i;
        } else if (Players[i] != NULL && Players[i]->getIsJoined() && Players[i]->isDead()) {
          Players[i]->setMessageScrollStateOnLCD(false); // disable the dead scroll
          Players[i]->drawLoseOnScreen();
        }
      }
      delay(1000); // Let the display process the score update first

      // Send to the display that the game is finished
      sendMessageToMaster(0, 5, winningPlayerIndex);

      current_game_state = 7; // go to the finished state
    }

    // check for any messages (could be we ended prematurly)
    if (receiveMessageFromMaster()) {
      if (Board_communication.y == 2 && Board_communication.v == 7) {
        // Game ends
        for (int i = 0; i < playerCount; i++) {
          if (Players[i] != NULL && Players[i]->getIsJoined() && Players[i]->isDead()) {
            Players[i]->setMessageScrollStateOnLCD(false); // disable the dead scroll
            Players[i]->drawLoseOnScreen();
          }
        }
      }
    }
  }
}

void handleFinishedState() {
  // Check if we have received a state change
  while (current_game_state == 7) {
    if (!receiveMessageFromMaster()) {
      delay(50);
    }  
  }
}

// END REGION ---------------------------------------------------------------------------------------------------

// REGION Communication -----------------------------------------------------------------------------------------

/**
 Sends a message to the master
 @playerIndex: the indedx of the player
 @type: defines the type of message > 0 = handshake, 1 = join game, 2 = statechange, 3 = scoreupdate, 5 = finish game
 @val: the value for the message if needed
 */
void sendMessageToMaster(int16_t playerIndex, int16_t type, int16_t val) {
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer
  uint16_t sendSize = 0;

  Board_communication.p = playerIndex;
  Board_communication.v = val;
  Board_communication.y = type;

  ///////////////////////////////////////// Stuff buffer with struct
  sendSize = myTransfer.txObj(Board_communication, sendSize);

  ///////////////////////////////////////// Send buffer
  myTransfer.sendData(sendSize);
}

/**
  Receive a message from the master. This could be a countdown, statechange or a score update
  */
bool receiveMessageFromMaster() {
  if(myTransfer.available())
  {
    // get the object transferred
    myTransfer.rxObj(Board_communication);

    if (Board_communication.y == 0) { // countdown
      // Connection has been made
      countDown = Board_communication.v;
    }
    else if (Board_communication.y == 1) // confirming that the player joined the game
    {
      // first we have to check if the chosen name is confirmed
      if (Board_communication.v == 1) 
      {
        // set the players state so it joined the game
        Players[Board_communication.p]->setIsJoined(true);

        // clear the gamepad display and show the players name
        Players[Board_communication.p]->clearLCD();
        Players[Board_communication.p]->drawPlayernameOnLCD(0, 0);

        // draw waiting to start
        Players[Board_communication.p]->setMessageScrollStateOnLCD(false);
        Players[Board_communication.p]->startMessageScrollOnLCD(1, "Joined, waiting to start", 16);
      } else {
        // reset the state, he has to pick a new name
        Players[Board_communication.p]->setGamepadButtonState(0);
        Players[Board_communication.p]->setHasPickedAName(false);
        Players[Board_communication.p]->setGamepadPressedState(false);

        Players[Board_communication.p]->drawNamePickingOnLCD(true); // draw the first name
      }
    }
    else if (Board_communication.y == 2) // State change
    {
      // The given value describes what state we have to become
      current_game_state = Board_communication.v;
    }
    else if (Board_communication.y == 3) // Score update...
    {
      
    } 

    return true; // a message has been received
  }
  return false;  // nothing is received
}

// END REGION
