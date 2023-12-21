// PLAYER.cpp
#include "PLAYER.h"
#include "player_names.h"

PLAYER::PLAYER(
  uint8_t playerIndex,
  uint8_t pickedNameIndex,
  uint8_t score) 
{
  // set our basic player vars
  this->score = score;
  this->playerIndex = playerIndex;
  this->pickedNameIndex = pickedNameIndex;

  // set the players name
  this->name = availableNames[pickedNameIndex];

  // Converting the name to uppercase 
  for (uint8_t cnt = 0; cnt < strlen(this->name); cnt++) 
    this->name[cnt] = toupper(this->name[cnt]);
}

bool PLAYER::isDead() {
  return this->score <= 0;
}

bool PLAYER::getIsJoined() { return isJoined; }
void PLAYER::setIsJoined(bool val) { isJoined = val; }

char* PLAYER::getName() { return this->name; }
void PLAYER::setName(char val[]) { this->name = val; }

int PLAYER::getScore() { return score; }
void PLAYER::setScore(int val) { score = val; }

int PLAYER::getIndex() { return playerIndex; }
int PLAYER::getPickedNameIndex() { return pickedNameIndex; }
