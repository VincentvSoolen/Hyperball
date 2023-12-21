/* HYPERBALL
 * Tools used: 
    https://rop.nl/truetype2gfx/
    http://www.rinkydinkelectronics.com/calc_rgb565.php
    https://chrishewett.com/blog/true-rgb565-colour-picker/

    Sound generators:
    https://texttospeechrobot.com/text-to-speech-tiktok/
    https://www.101soundboards.com/boards/tts
    https://speechgen.io/

    Better audio:
    https://opencircuit.nl/product/adafruit-audio-fx-sound-16mb

    Better screen:
    https://store.arduino.cc/products/giga-display-shield  (But you will need: Arduino GIGA R1 WiFi)

  */

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <SerialTransfer.h>
#include "DFRobotDFPlayerMini.h";
#include "player.h";
#include "Font_Minecraftia.h";
#include "Font_FiveFont.h";
#include "Font_PixelOperatorSC.h";
#include "Font_PixelOperatorSC_Bold.h";
#include "Font_upheavtt.h";

#define MINPRESSURE 200
#define MAXPRESSURE 1000

// Default color definitions
#define TFT_BLACK       0x0000      
#define TFT_PURPLE      0xA3F3      
#define TFT_LIGHTGREY   0xD69A      
#define TFT_DARKGREY    0x7BEF      
#define TFT_BLUE        0x03b5      
#define TFT_GREEN       0x6e20      
#define TFT_MAGENTA     0xF81F      
#define TFT_WHITE       0xFFFF      
#define TFT_ORANGE      0xCD8D      
#define TFT_PINK        0xf88f      
#define TFT_PINK_DARK   0x58E5
#define TFT_BACKGROUND  0x20e3
#define TFT_GREY_01     0xf77c      
#define TFT_GREY_02     0xa4f2      
#define TFT_GREY_03     0x62ca      
#define TFT_MINT        0x244E
#define TFT_RED         0x8163

#define C_PLAYER1 TFT_BLUE
#define C_PLAYER2 TFT_GREEN
#define C_PLAYER3 TFT_PURPLE
#define C_PLAYER4 TFT_ORANGE

#define maxPlayerCount 4

// TFT
MCUFRIEND_kbv tft;

//Inicia a serial por software nos pinos 10 e 11
DFRobotDFPlayerMini myDFPlayer;

// Touch panels 
const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9341
const int TS_LEFT=902,TS_RT=60,TS_TOP=930,TS_BOT=106;

// Touchscreen init
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// Touch vars
int pixel_x, pixel_y;
bool isTouchscreenTouched = false;

// Start button rectangle
int16_t startBtn_x = 10;
int16_t startBtn_y = 190;
int16_t startBtn_w = 300;
int16_t startBtn_h = 40;

// The transfer objects for the board communication
SerialTransfer myTransfer;

// Player defs
PLAYER *Players[4] = { NULL, NULL, NULL, NULL }; // 4 players max

int playerIndex[4] = { 0, 0, 0, 0}; // 0 = inactive, 1 = active
int playerNamesPicked[4] = { -1, -1, -1, -1}; 
// For the communication between the boards, just to test if everything is working
uint16_t playerColors[4] = { C_PLAYER1, C_PLAYER2, C_PLAYER3, C_PLAYER4 };
int16_t motherBoardsAlive[4] = { 0, 0, 0, 0 }; // stages => 0 = init (master -> motherboard), 1 = handshake (motherboard -> master), 2 = done
int16_t playerLives = 10;

// Game logic vars
bool playerEnteredGame = false;
bool playButtonDrawed = false;
bool gameEndPrematurely = false;
int16_t winningPlayerIndex = -1;

int16_t volume = 20;

/** 
  0 = INIT, waiting for reply from main
  1 = JOIN, waiting for key press to join the game
  2 = JOINED, waiting for others (only for slaves!)
  3 = GAME STARTING, countdown for the game to start
  5 = GAME IN PROGRESS, able to move etc (playing the game)
  6 = DEAD, waiting for te game to finish (only for slaves!)
  7 = ENDED, game is finished
  10 = IDLE, not joined the game (waiting on message from main)
  */
int16_t current_game_state = 0;

struct __attribute__((__packed__)) STRUCT {
  int16_t p;   // defines the player index
  int16_t y;   // defines the type
  int16_t v;   // defines the value
};
STRUCT Player_comms;

const char* playerNames[] = { 
  "#1 - BLUE", 
  "#2 - GREEN", 
  "#3 - PURPLE", 
  "#4 - ORANGE" 
};
int16_t x1, y1, tw, th;

/**
 Update the X and Y when the touchscreen is pressed
@return: true/false if the screen has been touched
 */
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
      // PORTRAIT  CALIBRATION     240 x 320
      //pixel_x = map(p.x, TS_LEFT, TS_RT, 0, 240);
      //pixel_y = map(p.y, TS_TOP, TS_BOT, 0, 320);

      // LANDSCAPE CALIBRATION     320 x 240
      //pixel_x = map(p.y, LEFT=70, RT=904, 0, 320)
      //pixel_y = map(p.x, TOP=87, BOT=927, 0, 240)

      // PORTRAIT  CALIBRATION     240 x 320
      // pixel_x = map(p.x, LEFT=106, RT=930, 0, 240)
      // pixel_y = map(p.y, TOP=902, BOT=60, 0, 320)

      // LANDSCAPE CALIBRATION     320 x 240
      pixel_x = map(p.y, TS_LEFT, TS_RT, 0, 320);
      pixel_y = map(p.x, TS_TOP, TS_BOT, 0, 240);
    }
    return pressed;
}

void setupTFTTouchscreen() {
  uint16_t ID = tft.readID();
  if (ID == 0xD3D3) ID = 0x9486; // write-only shield
  tft.begin(ID);
  tft.setRotation(3);            // Rotated Landscape
  tft.fillScreen(TFT_BACKGROUND);
}

void setupGraphics() {
//  start_btn.initButtonUL(&tft, 10, 190, 300, 40, TFT_MINT, TFT_MINT, TFT_GRAY_01, "START", 1); 
}

void setupSerialConnections() {
  // Mega to Mega communication
  Serial.begin(115200);
  
  // Board to board communication
  Serial3.begin(115200);
  myTransfer.begin(Serial3);

  // Audio
  Serial1.begin(9600);
}

void setupAudio() {
  // Try to initialize the SD card
  if (!myDFPlayer.begin(Serial1, true, false))
  {
    Serial.println(F("Not initialized:"));
    Serial.println(F("1. Check the DFPlayer Mini connections"));
    Serial.println(F("2. Insert an SD card"));
    while (true);
  }

  // default audio settings
  myDFPlayer.setTimeOut(500); //Timeout serial 500ms
  myDFPlayer.volume(volume); //Volume 5
  myDFPlayer.EQ(0); //Equalizacao normal

  // Play the welcome audio
  myDFPlayer.play("1");
}

void setup(void)
{
  //Serial.begin(9600);
  setupSerialConnections();
  //setupAudio(); // Disabled due to a humming sound....
  setupTFTTouchscreen();
  setupGraphics();

  // The game header should always be visible, no need to redraw this (only the volume)
  drawGameHeader();

 /* current_game_state = 5; // countdown test
  Players[0] = new PLAYER(0, 29, 10);
  Players[0]->setIsJoined(true);
  Players[1] = new PLAYER(1, 14, 10);
  Players[1]->setIsJoined(true);

  drawGameplayOutline();
  drawInitialPlayerStats();*/
}

void loop(void)
{
  if (current_game_state == 0) // INIT
  {
    handleInit();
  }
  // Waiting for slaves to join
  else if (current_game_state == 1) 
  {
    handleJoinPhase();
  }
  // Countdown phase
  else if (current_game_state == 3) 
  {
    handleCountdownState();
  }
  // Gameplay
  else if (current_game_state == 5) 
  {
    handleGameplayState();
  }
  else if (current_game_state == 7) 
  {
    handleGameEndedState();
  }
}

/* **************************************************************************************** 
  GAMESTATE HELPER FUNCTIONS
*******************************************************************************************/

/*
  Handle the init. Here we send out a message to all the controllers telling we are ready
  */
void handleInit() {
  // Draw the outline on the screen
  drawPlayerOverviewOutline();

  // Draw the names of the players
  for (int i = 0; i < maxPlayerCount; i++) {
    uint16_t yPos = 90 + (i * 24);

    // Draw the name of the player
    drawTextXY(30, yPos, 1, &Font_PixelOperatorSC, playerNames[i], playerColors[i]);
    
    // Draw the status
    if (Players[i] == NULL)
      drawTextXY(220, 94 + (i * 24), 1, &Font_FiveFont, "Waiting to join...", TFT_GREY_02);
    else
      drawTextXY(220, 94 + (i * 24), 1, &Font_FiveFont, "JOINED!", TFT_PINK);

    // Draw a pixel line
    if (i != maxPlayerCount - 1) {
      drawPixelLine(20, yPos + 6, 280, 2, TFT_GREY_03);
    }
  }

  delay(100);

  // Change the state and broadcast it to the slaves
  broadcastStateChange(1);
  current_game_state = 1;
}

/* 
  Handle the state during Join phase.
  Here players use the controller to pick a name and join the game. This is then showed on the screen
  */ 
void handleJoinPhase() {
  // get the touchscreen coords if touched
  isTouchscreenTouched = Touch_getXY();

  // Check if we received data
  if(myTransfer.available())
  {
    myTransfer.rxObj(Player_comms);
    if (Player_comms.y == 1) { // 1 == connection

      // check if that player does not exists yet
      if (Players[Player_comms.p] == NULL) {
        bool is_name_available = true;
        // then we check if the name is still available
        for (int i = 0; i < maxPlayerCount; i++) {
          if (Players[i] != NULL && Players[i]->getPickedNameIndex() == Player_comms.v) {
            is_name_available = false;
          }
        }

        if (is_name_available) {
          // If the name is still available we can construct the players object
          Players[Player_comms.p] = new PLAYER(Player_comms.p, Player_comms.v, 10);
          Players[Player_comms.p]->setIsJoined(true);
          playerEnteredGame = true;

          // Hide the previous text from the player
          tft.fillRoundRect(20, (80 + (Player_comms.p * 24)), 280, 12, 0, TFT_BACKGROUND);

          // Then draw the new name and state
          drawTextXY(30, 90 + (Player_comms.p * 24), 1, &Font_PixelOperatorSC, Players[Player_comms.p]->getName(), playerColors[Player_comms.p]);
          drawTextXY(220, 94 + (Player_comms.p * 24), 1, &Font_FiveFont, "JOINED!", TFT_PINK);

          delay(100); // give some processing time
          sendMessageToMotherboard(Player_comms.p, 1, 1); // Send a message back confirming the player has joined
        } else {
          delay(100); // give some processing time
          sendMessageToMotherboard(Player_comms.p, 1, 0); // Send a message back that the name was not accepted
        }
      }
    }
  }

  // Button click check
  if (isTouchscreenTouched && contains(pixel_x, pixel_y, startBtn_x, startBtn_y, startBtn_w, startBtn_h) && playButtonDrawed) {
    // tft.setFont(&Sinclair_Inverted_S); // button font
    drawGameStartButton(true);
    playButtonDrawed = false;

    if (playerEnteredGame) {
      // We can move to the countdown phase
      broadcastStateChange(3);
      current_game_state = 3;
    }
  } else if (!playButtonDrawed && !isTouchscreenTouched) {
    // draw the normal button
    drawGameStartButton(false);
    playButtonDrawed = true;
  }
}

/*
  Handles the countdown on the screen. Also sends the countdown to the controllers
  */
void handleCountdownState() {
  delay(300);

  // draw the outline box
  drawCountdownOutline();

  // draw the text
  drawTextXY(100, 130, 1, &Font_PixelOperatorSC, "GAME START IN...", TFT_GREY_01);

  // Now we are about to start the game, so we show that on the screen to the players!
  // This will delay every second for the countdown and sends it to the controllers
  drawCountdown(140, 170);

  // We can move to the playing state
  broadcastStateChange(5);
  current_game_state = 5;   

  delay(300);

  // draw the outline for the gameplay session
  drawGameplayOutline();
  drawInitialPlayerStats();
}

/* Handle the state during gameplay */
void handleGameplayState() {
  // Check if we received data
  if(myTransfer.available())
  {
    myTransfer.rxObj(Player_comms);

    if (Player_comms.y == 3) { // 3 == Score update
      if (Players[Player_comms.p] != NULL) {
        PLAYER *p = Players[Player_comms.p];
        if (p->getIsJoined()) {
          // Update the players score
          p->setScore(Player_comms.v);
        }
      }

      // update the TFT with the new score
      drawPlayerScores();
    } 
    // State 5 = that the game has ended and we have a winner
    else if (Player_comms.y == 5) 
    { 
      // set the winning player
      winningPlayerIndex = Player_comms.v;

      // update the gamestate to ended
      current_game_state = 7;
    }
  }

  // Check if the end game button is pressed
  // Button click check
  if (isTouchscreenTouched && contains(pixel_x, pixel_y, startBtn_x, startBtn_y, startBtn_w, startBtn_h) && playButtonDrawed) {
    // tft.setFont(&Sinclair_Inverted_S); // button font
    drawEndGameButton(true);
    playButtonDrawed = false;
    
    // We ended the game, so we must let our controllers know the game has ended
    broadcastStateChange(7);
    current_game_state = 7;
    gameEndPrematurely = true;

  } else if (!playButtonDrawed && !isTouchscreenTouched) {
    // draw the normal button
    drawEndGameButton(false);
    playButtonDrawed = true;
  }

  delay(50);
}

/* 
  Handle the game ended state.
  Show the player that has won and show a button to return to the main screen
  */
void handleGameEndedState() {

  drawGameEndedOutline();

  // Draw the winner text onto the screen
  drawTextXY(100, 130, 1, &Font_PixelOperatorSC, "AND THE WINNER IS", TFT_GREY_01);
  if (gameEndPrematurely) {
    drawTextXY(85, 150, 1, &Font_PixelOperatorSC, "NOBODY! YOU PUSHED THA BUTTON...", TFT_PINK);
  } 
  else if (winningPlayerIndex < 0) 
  {    
    drawTextXY(85, 150, 1, &Font_PixelOperatorSC, "NOBODY! YOU LOST...", TFT_PINK);
  }
  else 
  {
    drawTextXY(85, 150, 1, &Font_PixelOperatorSC, Players[winningPlayerIndex]->getName(), TFT_PINK);
  }

  // Check if the end game button is pressed
  while (current_game_state == 7)
  {
    // Button click check
    if (isTouchscreenTouched && contains(pixel_x, pixel_y, startBtn_x, startBtn_y, startBtn_w, startBtn_h) && playButtonDrawed) {
      drawEndGameButton(true);
      playButtonDrawed = false;

      // Delete all the current players
      for (int i = 0; i < maxPlayerCount; i++)
        Players[i] = NULL;

      // reset some variables
      winningPlayerIndex = -1;
      playerEnteredGame = false;

      current_game_state = 0; // reset to init
      broadcastStateChange(0);

      delay(300);

    } else if (!playButtonDrawed && !isTouchscreenTouched) {
      // draw the normal button
      drawEndGameButton(false);
      playButtonDrawed = true;
    }

    delay(50);
  }
}

/* ****************************************************************************************
 SERIAL COMMUNICATION FUNCTIONS
*******************************************************************************************/

void broadcastStateChange(int16_t state) {
  // broadcast it for all players
  //for (int pointer = 0; pointer < maxPlayerCount; pointer++) {
    sendMessageToMotherboard(0, 2, state);
    delay(50);
  //}
}

void sendMessageToAllMotherboards(int16_t type, int16_t val) {
  // broadcast it for all players
  for (int pointer = 0; pointer < maxPlayerCount; pointer++) {
    sendMessageToMotherboard(pointer, type, val);
    delay(50);
  }
}

/* Sends a message to the given motherboard
 * playerIndex: defines the player index range 0...4
 * type: defines the type of message > 0 = handshake, 1 = connection, 2 = statechange, 3 = scoreupdate
 * val: the value for the message if needed
 */
void sendMessageToMotherboard(int16_t playerIndex, int16_t type, int16_t val) {
  // use this variable to keep track of how many
  // bytes we're stuffing in the transmit buffer
  uint16_t sendSize = 0;

  Player_comms.p = playerIndex;
  Player_comms.v = val;
  Player_comms.y = type;

  ///////////////////////////////////////// Stuff buffer with struct
  sendSize = myTransfer.txObj(Player_comms, sendSize);

  ///////////////////////////////////////// Send buffer
  myTransfer.sendData(sendSize);
}

/* ****************************************************************************************
 TFT HELPER FUNCTIONS
*******************************************************************************************/

/**
  Use this to redraw all the player score without having to redraw the entire section
  **/
void drawPlayerScores() {
  uint16_t playerCount = 0;
  // Draw the names of the players
  for (int i = 0; i < maxPlayerCount; i++) {
    uint16_t yPos = 90 + (playerCount * 24);

    if (Players[i] == NULL) continue;
    PLAYER *p = Players[i];
    if (!p->getIsJoined()) continue;

    // Draw the score itself
    int s = p->getScore();
    for (uint8_t y = 0; y < 10; y++) {
      tft.fillRect(
        250 + (y * 5), 
        81 + (playerCount * 24), 
        4, 
        11, 
        s >= (y+1) ? TFT_PINK : TFT_PINK_DARK);
    }

    playerCount++;
  }
}

/**
  Draws all the player names that have joined onto the screen including the initial scores
  */
void drawInitialPlayerStats() {
  uint16_t playerCount = 0;
  // Draw the names of the players
  for (int i = 0; i < maxPlayerCount; i++) {
    uint16_t yPos = 90 + (playerCount * 24);

    if (Players[i] == NULL) continue;
    PLAYER *p = Players[i];
    if (!p->getIsJoined()) continue;

    // Draw the name of the player
    drawTextXY(30, yPos, 1, &Font_PixelOperatorSC, p->getName(), playerColors[i]);
    // Draw the score text
    drawTextXY(215, yPos + 4, 1, &Font_FiveFont, "LIVES", TFT_GREY_02);
    
    // Draw the score itself
    int s = p->getScore();
    for (uint8_t y = 0; y < 10; y++) {
      tft.fillRect(
        250 + (y * 5), 
        81 + (playerCount * 24), 
        4, 
        11, 
        s >= (y+1) ? TFT_PINK : TFT_PINK_DARK);
    }
    // Draw a pixel line
    if (i != maxPlayerCount - 1) {
      drawPixelLine(20, yPos + 8, 280, 2, TFT_GREY_03);
    }

    playerCount++;
  }
}

void drawCountdown(uint16_t x, uint16_t y) {
  for (int i = 0; i < 3; i++)
  {
    char newChar[2];

    String temp_str = String(3 - i);
    temp_str.toCharArray(newChar,2); 

    sendMessageToAllMotherboards(0, 3 - i); // 0 == countdown type

    tft.fillRect(x - 4, y - 30, 60, 40, TFT_BACKGROUND); // clear the previous number first
    drawTextXY(x, y, 3, &Font_PixelOperatorSC_Bold, newChar, TFT_PINK);
    delay(1000);
  }
}

void drawTextXY(int x, int y, int sz, const GFXfont *f, const char *msg, uint16_t color)
{
    tft.setFont(f);
    tft.setCursor(x, y);
    tft.setTextColor(color);
    tft.setTextSize(sz);
    tft.print(msg);
}

void drawGameHeader() {
  // Backgrounds for the Logo text
  tft.fillRoundRect(10, 24, 140, 22, 4, TFT_GREY_03);
  tft.fillRoundRect(10, 17, 140, 22, 4, TFT_GREY_02);
  tft.fillRoundRect(10, 10, 140, 22, 4, TFT_GREY_01);

  // Draw the Game logo
  drawTextXY(19, 25, 1, &Font_Upheavtt, "HYPERBALL", TFT_PINK);

  tft.drawFastHLine(160, 10, 150, TFT_GREY_01);
  tft.drawFastHLine(160, 11, 150, TFT_GREY_01);
  tft.drawFastHLine(160, 12, 150, TFT_GREY_01);

  uint8_t xPos = 160;
  for (int i = 0; i < 38; i++) {
    tft.drawRect(xPos + (i*4), 30, 2, 2, TFT_GREY_01);
  }

  drawTextXY(160, 30, 1, &Font_Minecraftia, "A 2 TO 4 PLAYER RUMBLE GAME", TFT_GREY_01);

  drawTextXY(160, 49, 1, &Font_Minecraftia, "VOLUME", TFT_GREY_02);

  // Volume outline
  tft.drawFastHLine(200, 34, 53, TFT_GREY_02);
  tft.drawFastHLine(200, 46, 53, TFT_GREY_02);
  tft.drawFastVLine(200, 34, 12, TFT_GREY_02);
  tft.drawFastVLine(252, 34, 12, TFT_GREY_02);
  
  // Volume bars 
  // Todo: needs to become dynamic!
  for (int i = 0; i < 10; i++) {
    tft.fillRect(202 + (i*5), 36, 4, 9, 
      volume >= ((i+1)*2) ? TFT_PINK : TFT_PINK_DARK);
  }
}

void drawGameStartButton(bool inverse) {
  tft.fillRoundRect(startBtn_x, startBtn_y+4, startBtn_w, startBtn_h, 4, TFT_GREY_03);
  if (inverse) {
    tft.fillRoundRect(startBtn_x, startBtn_y  , startBtn_w, startBtn_h, 4, TFT_GREY_01);  
    drawTextXY(80, 215, 1, &Font_Upheavtt, "START THE GAME", TFT_PINK);
  } else {    
    tft.fillRoundRect(startBtn_x, startBtn_y  , startBtn_w, startBtn_h, 4, TFT_MINT);
    drawTextXY(80, 215, 1, &Font_Upheavtt, "START THE GAME", TFT_GREY_01);
  }
}

void drawEndGameButton(bool inverse) {
  tft.fillRoundRect(startBtn_x, startBtn_y+4, startBtn_w, startBtn_h, 4, TFT_GREY_03);
  if (inverse) {
    tft.fillRoundRect(startBtn_x, startBtn_y  , startBtn_w, startBtn_h, 4, TFT_GREY_01);  
    drawTextXY(80, 215, 1, &Font_Upheavtt, "END THE GAME", TFT_PINK);
  } else {    
    tft.fillRoundRect(startBtn_x, startBtn_y  , startBtn_w, startBtn_h, 4, TFT_RED);
    drawTextXY(80, 215, 1, &Font_Upheavtt, "END THE GAME", TFT_GREY_01);
  }
}

bool contains(int16_t tx, int16_t ty, int16_t x, int16_t y, int16_t w, int16_t h) {
  return ((tx >= x) && (tx < (int16_t)(x + w)) && (ty >= y) &&
          (ty < (int16_t)(y + h)));
}

void drawPlayerOverviewOutline() {
  // first clear the space
  tft.fillRect(0, 55, 320, 185, TFT_BACKGROUND);

  tft.drawRoundRect(10, 60, 300, 120, 4, TFT_GREY_01);
  tft.drawRoundRect(11, 61, 298, 118, 4, TFT_GREY_01);

  // remove some of the line for the text
  tft.fillRect(26, 58, 110, 4, TFT_BACKGROUND);

  // the text heading of the outline
  drawTextXY(30, 64, 1, &Font_PixelOperatorSC, "PLAYER OVERVIEW", TFT_PINK);
}

void drawCountdownOutline() {
  // first clear the space
  tft.fillRect(0, 55, 320, 185, TFT_BACKGROUND);

  // then draw the lines
  tft.drawRoundRect(10, 60, 300, 170, 4, TFT_GREY_01);
  tft.drawRoundRect(11, 61, 298, 168, 4, TFT_GREY_01);
}

void drawGameplayOutline() {
  // first clear the space
  tft.fillRect(0, 55, 320, 185, TFT_BACKGROUND);

  tft.drawRoundRect(10, 60, 300, 120, 4, TFT_GREY_01);
  tft.drawRoundRect(11, 61, 298, 118, 4, TFT_GREY_01);

  // remove some of the line for the text
  tft.fillRect(26, 58, 120, 4, TFT_BACKGROUND);

  // the text heading of the outline
  drawTextXY(30, 64, 1, &Font_PixelOperatorSC, "GAME IN PROGRESS", TFT_PINK);
}

void drawGameEndedOutline() {
  // first clear the space
  tft.fillRect(0, 55, 320, 185, TFT_BACKGROUND);

  tft.drawRoundRect(10, 60, 300, 120, 4, TFT_GREY_01);
  tft.drawRoundRect(11, 61, 298, 118, 4, TFT_GREY_01);

  // remove some of the line for the text
  tft.fillRect(26, 58, 120, 4, TFT_BACKGROUND);

  // the text heading of the outline
  drawTextXY(30, 64, 1, &Font_PixelOperatorSC, "GAME HAS ENDED", TFT_PINK);
}

void drawPixelLine(int16_t x, int16_t y, int16_t w, int16_t space, uint16_t color) {
  uint8_t sCount = abs(w / (1+space));
  for (uint8_t i = 0; i < sCount; i++) {
    tft.drawPixel(x + (i*(1+space)), y, color);
  }
}
