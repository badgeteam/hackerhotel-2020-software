/*
 * simon.c
 *
 * Created: 21/01/2020 15:42:42
 * Author: Badge.team
 */

#include <simon.h>
#include <main_def.h>
#include <resources.h>
#include <I2C.h>

// Main game loop
uint8_t BastetDictates(){
  if (CheckState(BASTET_COMPLETED))
      return 0;

  if ( (gameNow != TEXT) && (gameNow != BASTET) )
      return 0;

  

  return 0;
}
