/*
 * simon.c
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 *　　　　　　 ＿＿
 *　　　　　／＞　　フ
 *　　　　　|  _　 _l  My name is Bastet, not Simon!
 *　 　　　／`ミ＿xノ
 *　　 　 /　　　♥　\
 *　　　 /　 ヽ　　 ﾉ
 *　 　 │　　|　|　|
 *　／￣|　　 |　|　|
 *　| (￣ヽ＿_ヽ_)__)
 *　＼二つ
 */

#include <simon.h>
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

uint16_t simonScore = 0;
uint8_t  simonState[SIMON_LENGTH] = {0};
uint8_t  simonPos = 0;
uint8_t  simonInputPos = 0;
uint8_t  simonLen = 1;

uint8_t getSimonValue(uint8_t pos) {
  uint8_t b = simonState[pos>>2];
  uint8_t a1 = b&3;
  uint8_t a2 = (b>>2)&3;
  uint8_t a3 = (b>>4)&3;
  uint8_t a4 = (b>>6)&3;
  //Serial.printf("%d = %d\r\n", pos, (pos&1)?((pos&2)?(a4):(a3)):((pos&2)?(a2):(a1)));
  return (pos&1)?((pos&2)?(a4):(a3)):((pos&2)?(a2):(a1));
}

void setSimonValue(uint8_t pos, uint8_t val) {
  uint8_t b = simonState[pos>>2];
  uint8_t a1 = b&3;
  uint8_t a2 = (b>>2)&3;
  uint8_t a3 = (b>>4)&3;
  uint8_t a4 = (b>>6)&3;
  if ((pos&3) == 0) a1 = val;
  if ((pos&3) == 1) a2 = val;
  if ((pos&3) == 2) a3 = val;
  if ((pos&3) == 3) a4 = val;
  simonState[pos>>2] = (a4<<6) | (a3<<4) | (a2<<2) | a1;
}

inline void setupSimon() {
  for (uint8_t i = 0; i < SIMON_LENGTH*4; i++) {
    uint8_t val = (lfsr() % 4);
    setSimonValue(i, val);
  }
}

void simonTone(uint8_t val) {
  // if (val == 1) {
  //   tone(PIN_AUDIO, 200);
  // } else if (val == 2) {
  //   tone(PIN_AUDIO, 400);
  // } else if (val == 3) {
  //   tone(PIN_AUDIO, 600);
  // } else if (val == 4) {
  //   tone(PIN_AUDIO, 800);
  // } else {
  //   noTone(PIN_AUDIO);
  // }
}

void simonLed(uint8_t val) {
  simonTone(val);
  if (val == 1) {
    leds[1] |= (1UL << 3);
  } else if (val == 2) {
    leds[2] |= (1UL << 1);
  } else if (val == 3) {
    leds[1] |= (1UL << 0);
  } else if (val == 4) {
    leds[2] |= (1UL << 4);
  } else {
    for (uint8_t n=0; n<5; n++){
      iLED[WING[L][n]] = 0;
      iLED[WING[R][n]] = 0;
    }
  }
}


// Main game loop
uint8_t BastetDictates(){
  if (CheckState(BASTET_COMPLETED))
      return 0;

  if ( (gameNow != TEXT) && (gameNow != BASTET) )
      return 0;



  return 0;
}
