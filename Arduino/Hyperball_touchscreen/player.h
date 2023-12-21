// Player.h
#ifndef PLAYER_h
#define PLAYER_h

#include <Arduino.h>

class PLAYER {
  private:
    // player vars
    bool isJoined = false;
    uint8_t score;
    char* name;
    uint8_t playerIndex;
    uint8_t pickedNameIndex;

  public:
    PLAYER(
      uint8_t playerIndex,
      uint8_t pickedNameIndex,
      uint8_t score);

    bool isDead();

    /**
      Loop should be called as many times as you can from the main loop
      */
    void loop();

    bool getIsJoined();
    void setIsJoined(bool val);

    char* getName();
    void setName(char name[]);

    int getScore();
    void setScore(int score);

    int getIndex();
    int getPickedNameIndex();
};

#endif