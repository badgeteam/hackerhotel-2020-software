/* Pins */

#define PIN_R0    6  //B5
#define PIN_R1    17 //C5
#define PIN_R2    8  //B3
#define PIN_R3    16 //C4
#define PIN_R4    15 //C3
#define PIN_R5    7  //B4
#define PIN_C0    13 //C1
#define PIN_C1    9  //B2
#define PIN_C2    12 //C0
#define PIN_C3    5  //B6
#define PIN_C4    14 //C2
#define PIN_BTN   4  //B7
#define PIN_HALL  1  //A5
#define PIN_LIGHT 0  //A4
#define PIN_PWR   20 //A3
#define PIN_AUDIO 2  //A6
#define PIN_RX    19 //A2
#define PIN_TX    18 //A1
#define PIN_SDA   10 //B1
#define PIN_SCL   11 //B0

/* State machine */
#define STATE_BOOT                  0
#define STATE_MUSIC                 1
#define STATE_GAME_START            2
#define STATE_GAME_WAIT_LED_ON      3
#define STATE_GAME_WAIT_LED_OFF     4
#define STATE_GAME_WAIT_FOR_INPUT   5
#define STATE_GAME_WAIT_AFTER_INPUT 6
#define STATE_GAME_SHOW_PATTERN     7
#define STATE_GAME_INPUT            8
#define STATE_GAME_ALARM            9
#define STATE_IDLE                  10
#define STATE_MUSIC2                11
#define STATE_GAME_CODE_WAIT        12
#define STATE_MAZE                  13
uint8_t state = STATE_BOOT;


/* Code */
const uint8_t CODE[] = { 1,0,0,2, 1,2,0,1, 1,3,0,3, 1,3,1,0, 1,2,1,1, 1,3,1,0 };

/* LEDs */

const uint8_t LED_ROWS[] = {PIN_R0, PIN_R1, PIN_R2, PIN_R3, PIN_R4, PIN_R5};
const uint8_t LED_COLS[] = {PIN_C0, PIN_C1, PIN_C2, PIN_C3, PIN_C4};

uint8_t leds[sizeof(LED_COLS)] = {0};

inline void setupLeds() {
  for (uint8_t i = 0; i < sizeof(LED_ROWS); i++) pinMode(LED_ROWS[i], OUTPUT);
  for (uint8_t i = 0; i < sizeof(LED_COLS); i++) pinMode(LED_COLS[i], OUTPUT);
}

void writeCol(uint8_t c) {
  for (uint8_t i = 0; i < sizeof(LED_ROWS); i++) {
    digitalWrite(LED_ROWS[i], (PinStatus)((leds[c]>>i)&1));
  }
}

void setHacker(uint8_t state) {
  //Swap bits to match physical LED layout
  int8_t swappedState = ((state&(1<<0))<<0) |
                         ((state&(1<<1))<<4) |
                         ((state&(1<<2))<<2) |
                         ((state&(1<<3))<<0) |
                         ((state&(1<<4))>>2) |
                         ((state&(1<<5))>>4);
  //Turn on green LEDs for set bits and red LEDs for unset bits
  leds[4] = swappedState;
  leds[3] = ~swappedState;
}

void setHackerScore(uint8_t score) {
  uint8_t state = 0;
  //Convert score to bits
  if (score > 0) state |= 1;
  if (score > 1) state |= 2;
  if (score > 2) state |= 4;
  if (score > 3) state |= 8;
  if (score > 4) state |= 16;
  if (score > 5) state |= 32;
  setHacker(state);
}


void setEyes(uint8_t A, uint8_t B) {
  if (A&1) {
    leds[1] |= 4;
  } else {
    leds[1] &= ~4;
  }
  if (A&2) {
    leds[1] |= 2;
  } else {
    leds[1] &= ~2;
  }
  if (B&1) {
    leds[0] |= 4;
  } else {
    leds[0] &= ~4;
  }
  if (B&2) {
    leds[0] |= 2;
  } else {
    leds[0] &= ~2;
  }
}

void setForehead(uint8_t A) {
  if (A&1) {
    leds[0] |= 16;
  } else {
    leds[0] &= ~16;
  }
  if (A&2) {
    leds[0] |= 8;
  } else {
    leds[0] &= ~8;
  }
}

void setHeart(uint8_t B) {
  if (B&1) {
    leds[0] |= 1;
  } else {
    leds[0] &= ~1;
  }
  if (B&2) {
    leds[0] |= 32;
  } else {
    leds[0] &= ~32;
  }
}

/* Audio */
const uint16_t musicData[] = { 0, 659, 498, 0, 6, 493, 232, 0, 20, 523, 231, 0,11, 587, 495, 0, 11, 523, 233, 0, 22, 493, 239, 0, 17, 440, 522, 440, 232, 0, 19, 523, 231, 0, 5, 659, 496, 0, 11, 587, 226, 0, 24, 523, 226, 0, 24, 493, 509, 493, 227, 0, 23, 523, 227, 0, 11, 587, 489, 0, 11, 659, 489, 0, 10, 523, 511, 440, 492, 440, 490, 0, 1020, 587, 489, 698, 232, 0, 16, 880, 471, 0, 279, 783, 243, 0, 19, 698, 230, 0, 19, 659, 994, 523, 226, 0, 23, 659, 760, 587, 226, 0, 22, 523, 228, 0, 11, 493, 742, 0, 6, 523, 232, 0, 19, 587, 493, 0, 16, 659, 494, 523, 488, 0, 13, 440, 487, 0, 13, 440, 510, 0, 1485, 659, 1004, 523, 975, 0, 25, 587, 999, 493, 981, 0, 23, 523, 998, 440, 975, 0, 21, 415, 999, 493, 980, 0, 26, 659, 997, 0, 1, 523, 979, 0, 18, 587, 1000, 0, 1, 493, 979, 0, 24, 523, 511, 659, 487, 0, 1, 880, 982, 0, 18, 830, 1991, 0 };
const uint16_t musicData2[] = { 0, 293, 534, 0, 72, 293, 237, 0, 72, 440, 541, 0, 73, 440, 236, 0, 82, 329, 390, 0, 59, 349, 83, 0, 84, 329, 224, 0, 81, 293, 786, 0, 445, 440, 229, 0, 71, 523, 236, 0, 72, 587, 546, 0, 71, 523, 234, 0, 81, 440, 218, 0, 84, 493, 222, 0, 84, 391, 234, 0, 72, 440, 799, 0, 129, 587, 531, 0, 82, 587, 225, 0, 80, 523, 541, 0, 82, 440, 225, 0, 83, 440, 222, 0, 82, 391, 227, 0, 82, 349, 225, 0, 83, 329, 797, 0, 119, 293, 552, 0, 66, 440, 225, 0, 81, 391, 545, 0, 70, 349, 231, 0, 83, 329, 225, 0, 79, 293, 226, 0, 83, 261, 223, 0, 83, 293, 802, 0 };
//const uint16_t musicData[] = { 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//const uint16_t musicData2[] = { 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 523, 440, 440, 440, 0, 392, 523, 440, 0, 440, 466, 440, 349, 349, 311, 311, 349, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
bool tetrisActive = false;
uint16_t tetrisPos = 0;

/* Sensors */
uint16_t HallCalibration = 0;
uint8_t  HallState = 0;
uint8_t  NewHallState = 0;

void setupSensors() {
  #if F_CPU >= 20000000 // 20 MHz / 128 = 156.250 kHz
    ADC1.CTRLC |= ADC_PRESC_DIV128_gc;
  #elif F_CPU >= 16000000 // 16 MHz / 128 = 125 kHz
    ADC1.CTRLC |= ADC_PRESC_DIV128_gc;
  #elif F_CPU >= 8000000 // 8 MHz / 64 = 125 kHz
    ADC1.CTRLC |= ADC_PRESC_DIV64_gc;
  #elif F_CPU >= 4000000 // 4 MHz / 32 = 125 kHz
    ADC1.CTRLC |= ADC_PRESC_DIV32_gc;
  #elif F_CPU >= 2000000 // 2 MHz / 16 = 125 kHz
    ADC1.CTRLC |= ADC_PRESC_DIV16_gc;
  #elif F_CPU >= 1000000 // 1 MHz / 8 = 125 kHz
    ADC1.CTRLC |= ADC_PRESC_DIV8_gc;
  #else // 128 kHz / 2 = 64 kHz -> This is the closest you can get, the prescaler is 2
    ADC1.CTRLC |= ADC_PRESC_DIV2_gc;
  #endif
  ADC1.CTRLA |= ADC_ENABLE_bm;
  ADC1.CTRLC &= ~(ADC_REFSEL_gm);
  VREF.CTRLA &= ~(VREF_ADC1REFSEL_gm);
  ADC1.CTRLC |= ADC_REFSEL_VDDREF_gc;
  pinMode(PIN_PWR, OUTPUT);
  digitalWrite(PIN_PWR, HIGH);

  HallCalibration = 0;
  uint8_t i;
  for (i=0; i<10; i++) {
    HallCalibration += analogRead(PIN_HALL);
  }
  HallCalibration = HallCalibration/10;
  delay(25);
}

uint16_t readTempAnalog() {
  ADC0.MUXPOS = 0x1E;
  ADC0.COMMAND = ADC_STCONV_bm;
  while(!(ADC0.INTFLAGS & ADC_RESRDY_bm));
  return ADC0.RES;
}

uint16_t readHallAnalog() {
  uint16_t res = analogRead(PIN_HALL);
  return res;
}

uint16_t readLightAnalog() {
  uint16_t res = analogRead(PIN_LIGHT);
  return res;
}

uint8_t heartAnimation = 0;

uint8_t readBtn() {
  return mapBtnAnalogToBits(readBtnAnalog());
}

uint16_t readBtnAnalog() {
  ADC1.MUXPOS = 0x04;
  ADC1.COMMAND = ADC_STCONV_bm;
  while(!(ADC1.INTFLAGS & ADC_RESRDY_bm));
  return ADC1.RES;
}

uint8_t mapBtnAnalogToBits(uint16_t analog) {
  if (analog > 860) return 0;
  if (analog > 604) return (1<<3);                      //RB
  if (analog > 464) return (1<<1);                      //RT
  if (analog > 380) return (1<<1)+(1<<3);               //RT+RB
  if (analog > 320) return (1<<0);                      //LT
  if (analog > 278) return (1<<0)+(1<<3);               //LT+RB
  if (analog > 244) return (1<<0)+(1<<1);               //LT+RT
  if (analog > 219) return (1<<0)+(1<<1)+(1<<3);        //LT+RT+RB
  if (analog > 199) return (1<<2);                      //LB
  if (analog > 180) return (1<<2)+(1<<3);               //LB+RB
  if (analog > 165) return (1<<1)+(1<<2);               //RT+LB
  if (analog > 150) return (1<<1)+(1<<2)+(1<<3);        //RT+LB+RB
  if (analog > 145) return (1<<0)+(1<<2);               //LT+LB
  if (analog > 133) return (1<<0)+(1<<2)+(1<<3);        //LT+LB+RB
  if (analog > 124) return (1<<0)+(1<<1)+(1<<2);        //LT+RT+LB
                    return (1<<0)+(1<<1)+(1<<2)+(1<<3); //LT+RT+LB+RB
}

/* Simon says */
#define SIMON_LENGTH 16
uint16_t simonScore = 0;
uint8_t  simonState[SIMON_LENGTH] = {0};
uint8_t  simonPos = 0;
uint8_t  simonInputPos = 0;
uint8_t  simonLen = 1;

/* Walk like an Egyptian */
const uint8_t mazeCode[] = {1,1,2,1,2,2,1,2,2,1,2,2,2,2,2,1,1,2};
uint8_t  mazePos = 0;
uint8_t  mazeCnt = 0;
boolean  mazeState = true;

void setup() {
  Serial.begin(9600);
  Serial.println("BADGE.TEAM\r\nHackerHotel 2020 badge\r\n");
  setupLeds();
  setupSensors();
  setupSimon();

  uint8_t btnState = mapBtnAnalogToBits(readBtnAnalog());
  if (btnState&1) {
    state = STATE_MUSIC;
  }
  
  if (btnState&2) {
    state = STATE_IDLE;
    heartAnimation = 0xFF;
    for (uint8_t i = 0; i < sizeof(LED_COLS); i++) leds[i] = 0xFF;
  }

  if (btnState&4) {
    state = STATE_MUSIC2;
  }
  
  Serial.print("> ");
}

inline void setupSimon() {
  randomSeed((readTempAnalog()&0x0F)+((readLightAnalog()&0x0F)<<4));
  simonPopulate();
}

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

void simonPopulate() {
  uint8_t last = 0;
  for (uint8_t i = 0; i < SIMON_LENGTH*4; i++) {
    uint8_t val = random(0,4);
    if (val == last) val = random(0,4);
    setSimonValue(i, val);
  }
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
    leds[1] &= ~(1UL << 3);
    leds[1] &= ~(1UL << 0);
    leds[2] &= ~(1UL << 1);
    leds[2] &= ~(1UL << 4);
  }
}

void simonTone(uint8_t val) {
  if (val == 1) {
    tone(PIN_AUDIO, 200);
  } else if (val == 2) {
    tone(PIN_AUDIO, 400);
  } else if (val == 3) {
    tone(PIN_AUDIO, 600);
  } else if (val == 4) {
    tone(PIN_AUDIO, 800);
  } else {
    noTone(PIN_AUDIO);
  }
}

inline void updateLeds() {
  static uint8_t doLeds = 0;
  if (doLeds < 30) {
    doLeds++;
    return;
  }
  doLeds = 0;
  
  static uint8_t currentColumn = 0;
  digitalWrite(LED_COLS[currentColumn], LOW);
  currentColumn++;
  if (currentColumn >= sizeof(LED_COLS)) {
    currentColumn = 0;
  }
  writeCol(currentColumn);
  digitalWrite(LED_COLS[currentColumn], HIGH);
}

void serialCommand(char* input) {
  char* cmdStr = strtok(input, " ");
  char* arg1Str = strtok(NULL, " ");
  char* arg2Str = strtok(NULL, " ");
  if (cmdStr != NULL) {
    if (strcmp(cmdStr, "led")==0) {
      if ((arg1Str != NULL)&&(arg2Str != NULL)) {
        uint8_t col = atoi(arg1Str);
        uint8_t val = atoi(arg2Str);
        if (col < sizeof(LED_COLS)) {
          leds[col] = val;
          Serial.println("OK");
        } else {
          Serial.printf("invalid column, range 0-%u\r\n", sizeof(LED_COLS)-1);
        }
      } else {
        Serial.println("Missing arguments!");
      }
    } else if (strcmp(cmdStr, "btn")==0) {
      uint16_t val = readBtnAnalog();
      Serial.printf("Analog value: %u\r\n", val);
      uint8_t state = mapBtnAnalogToBits(val);
      Serial.printf("Button state: %s %s %s %s\r\n",
        (state&1) ? "LT":"--",
        (state&2) ? "RT":"--",
        (state&4) ? "LB":"--",
        (state&8) ? "RB":"--"
      );
    } else if (strcmp(cmdStr, "temp")==0) {
      uint16_t val = readTempAnalog();
      Serial.printf("Analog value: %u\r\n", val);
    } else if (strcmp(cmdStr, "hall")==0) {
      uint16_t val = readHallAnalog();
      Serial.printf("Analog value: %u\r\n", val);
    } else if (strcmp(cmdStr, "light")==0) {
      uint16_t val = readLightAnalog();
      Serial.printf("Analog value: %u\r\n", val);
    } else if (strcmp(cmdStr, "simon")==0) {
      if (arg1Str != NULL) {
        bool simon = atoi(arg1Str);
        if (simon) {
          simonScore = 0;
          state = STATE_BOOT;
          Serial.println("Simon says enabled!");
        } else {
          state = STATE_IDLE;
          Serial.println("Simon says disabled!");
        }
      } else {
        Serial.println("Missing argument!");
      }
    } else if (strcmp(cmdStr, "hacker")==0) {
      if (arg1Str != NULL) {
        uint8_t state = atoi(arg1Str);
        setHacker(state);
      } else {
        Serial.println("Missing argument!");
      }
    } else if (strcmp(cmdStr, "eyes")==0) {
      if ((arg1Str != NULL)&&(arg2Str != NULL)) {
        uint8_t A = atoi(arg1Str);
        uint8_t B = atoi(arg2Str);
        setEyes(A, B);
      } else {
        Serial.println("Missing argument!");
      }
    } else if (strcmp(cmdStr, "forehead")==0) {
      if (arg1Str != NULL) {
        uint8_t A = atoi(arg1Str);
        setForehead(A);
      } else {
        Serial.println("Missing argument!");
      }
    } else if (strcmp(cmdStr, "heart")==0) {
      if (arg1Str != NULL) {
        uint8_t A = atoi(arg1Str);
        if (A>3) {
          heartAnimation = 0;
        } else {
          heartAnimation = 0xFF;
          setHeart(A);
        }
      } else {
        Serial.println("Missing argument!");
      }
    } else if (strcmp(cmdStr, "score")==0) {
      if (arg1Str != NULL) {
        simonScore = atoi(arg1Str);
        setHackerScore(simonScore);
      } else {
        Serial.println("Missing argument!");
      }
    } else if (strcmp(cmdStr, "help")==0) {
      Serial.println("Set LEDs:          led <column> <value>");
      Serial.println("Read buttons:      btn");
      Serial.println("Read temperature:  temp");
      Serial.println("Read hall sensor:  hall");
      Serial.println("Read light sensor: light");
      Serial.println("Simon says:        simon <enable>");
      Serial.println("Hacker LED status: hacker <value>");
      Serial.println("Simon says score:  score <value>");
      Serial.println("Heart:             heart <value>");
      Serial.println("Forehead:          forehead <value>");
      Serial.println("Eyes:              eyes <left> <right>");
    } else {
      Serial.println("Unrecognized command!");
    }
  }
}

#define SERIAL_BUFFER_SIZE 16
inline void updateSerial() {
  static char buff[SERIAL_BUFFER_SIZE] = {0};
  static uint8_t pos = 0;
  while (Serial.available()) {
    char val = Serial.read();
    if ((val == '\n') || (val == '\r')) {
      Serial.println();
      serialCommand(buff);
      Serial.print("> ");
      memset(buff, 0, SERIAL_BUFFER_SIZE);
      pos = 0;
    } else if (val == 127) { //Backspace
      if (pos > 0) {
        buff[pos] = 0;
        pos--;
        Serial.print("\033[1D \033[1D");
      }
    } else if (pos>=SERIAL_BUFFER_SIZE) {
      Serial.println("UART buffer overrun!");
      Serial.print("> ");
      memset(buff, 0, SERIAL_BUFFER_SIZE);
      pos = 0;
    } else {
      buff[pos] = val;
      pos++;
      Serial.print(val);
    }
  }
}

inline void updateHeart() {
  unsigned long        currentMillis   = millis();
  static unsigned long previousMillis  = 0;
  static uint8_t       counter         = 0;
  if ((heartAnimation!=0xFF)&&(currentMillis - previousMillis >= 100)) {
    previousMillis = currentMillis;
    if ((heartAnimation==0)||(heartAnimation==2)) {
      setHeart(1);
      heartAnimation++;
    } else if (heartAnimation==1) {
      setHeart(0);
      heartAnimation++;
    } else if (heartAnimation < 20) {
      setHeart(0);
      heartAnimation++;
    } else {
      heartAnimation = 0;
    }
  }
}

inline void updateLight() {
  unsigned long        currentMillis   = millis();
  static unsigned long previousMillis  = 0;
  if (currentMillis - previousMillis >= 300) {
    previousMillis = currentMillis;
    if (state != STATE_IDLE) {
      uint16_t light = readLightAnalog();
      if (light < 50) {
        setForehead(1);
      } else if (light > 500) {
        setForehead(3);
      } else if (light > 250) {
        setForehead(2);
      } else {
        setForehead(0);
      }
    }
  }
}

inline void updateHall() {
  unsigned long        currentMillis   = millis();
  static unsigned long previousHallMillis  = 0;
  if (currentMillis - previousHallMillis >= 50) {
    previousHallMillis = currentMillis;
    if ((state != STATE_IDLE) && (state != STATE_MUSIC) && (state != STATE_MUSIC2)) {
      int16_t val = readHallAnalog() - HallCalibration;
      switch (HallState) {
        case 0: {
          if ( -75 > val ) {
            NewHallState = 1;
            setEyes(2,0);
            state = STATE_MAZE;
          } else if ( 75 < val ) {
            NewHallState = 2;
            setEyes(0,2);
            state = STATE_MAZE;
          } else {
            NewHallState = 0;
          }
          break;
        }
        
        case 1: {
          if ( 75 < val ) {
            NewHallState = 2;
            setEyes(0,2);
          } else if ( -30 < val ) {
            NewHallState = 0;
            setEyes(0,0);
          } else {
            NewHallState = 1;
          }
          state = STATE_MAZE;
          break;
        }
        
        case 2: {
          if ( -75 > val ) {
            NewHallState = 1;
            setEyes(2,0);
          } else if ( 30 > val) {
            NewHallState = 0;
            setEyes(0,0);
          } else {
            NewHallState = 2;
          }
          state = STATE_MAZE;
          break;
        }
      }
      simonTone(0);
//      Serial.printf("Hallsensor value %d (state=%d, newstate=%d, gamestate=%d)\r\n", val, HallState, NewHallState, state);
    }
  }
}

void loop() {
  updateSerial();
  updateLeds();
  updateHeart();
  updateLight();
  updateHall();
  
  static uint8_t       counter         = 0;
  unsigned long        currentMillis   = millis();
  static unsigned long previousMillis1 = 0;
  static uint8_t       codePosition    = 0;
  static bool          codeDebounce    = false;

  switch (state) {
    case STATE_BOOT: {
      if (currentMillis - previousMillis1 >= 100) {
        previousMillis1 = currentMillis;
        setEyes(2,2);
        tone(PIN_AUDIO, 50*counter);
        setHacker(1<<(counter&0x3F));
        counter++;
        if (counter > 12) {
          counter = 0;
          state = STATE_GAME_START;
        }
      }
      break;
    }

    case STATE_MUSIC: {
      if (currentMillis - previousMillis1 >= musicData[tetrisPos*2]) {
        previousMillis1 = currentMillis;
        setEyes(1,1);
        tone(PIN_AUDIO, musicData[tetrisPos*2+1]);
        setHacker(musicData[tetrisPos*2+1]);
        tetrisPos++;
        if (tetrisPos >= sizeof(musicData)/4) {
          tetrisPos = 0;
          state = STATE_BOOT;
        }
      }
      break;
    }

    case STATE_MUSIC2: {
      if (currentMillis - previousMillis1 >= musicData2[tetrisPos*2]) {
        previousMillis1 = currentMillis;
        setEyes(1,1);
        tone(PIN_AUDIO, musicData2[tetrisPos*2+1]);
        setHacker(musicData2[tetrisPos*2+1]);
        tetrisPos++;
        if (tetrisPos >= sizeof(musicData2)/4) {
          tetrisPos = 0;
          state = STATE_BOOT;
        }
      }
      break;
    }

    case STATE_GAME_START: {
      setEyes(1,1);
      simonInputPos = 0;
      simonPos = 0;
      simonLen = 1;
      simonScore = 0;
      setHackerScore(simonScore);
      simonTone(0);
      simonPopulate();
      state = STATE_GAME_SHOW_PATTERN;
      break;
    }

    case STATE_GAME_SHOW_PATTERN: {
      simonInputPos = 0;
      uint8_t btnState = readBtn();
      if (btnState > 0) {
        setEyes(3,3);
        simonLed(0);
        counter = 0;
        state = STATE_GAME_INPUT;
      } else if (currentMillis - previousMillis1 >= 200) {
        previousMillis1 = currentMillis;
        if (simonPos >= simonLen) {
          simonLed(0);
          state = STATE_GAME_WAIT_FOR_INPUT;
        } else {
          simonLed(getSimonValue(simonPos)+1);
          simonPos++;
          state = STATE_GAME_WAIT_LED_ON;
        }
      }
      break;
    }

    case STATE_GAME_WAIT_FOR_INPUT: {
      simonLed(0);
      uint8_t btnState = readBtn();
      if (btnState > 0) {
        setEyes(3,3);
        simonLed(0);
        state = STATE_GAME_INPUT;
      } else if (currentMillis - previousMillis1 >= 1000) {
          previousMillis1 = currentMillis;
          simonPos = 0;
          state = STATE_GAME_SHOW_PATTERN;
      }
      break;
    }

    case STATE_GAME_WAIT_LED_ON: {
      uint8_t btnState = readBtn();
      if (btnState > 0) {
        setEyes(3,3);
        simonLed(0);
        state = STATE_GAME_INPUT;
      } else if (currentMillis - previousMillis1 >= 100) {
          simonLed(0);
          previousMillis1 = currentMillis;
          state = STATE_GAME_WAIT_LED_OFF;
      }
      break;
    }

    case STATE_GAME_WAIT_LED_OFF: {
      uint8_t btnState = readBtn();
      if (btnState > 0) {
        setEyes(3,3);
        simonLed(0);
        state = STATE_GAME_INPUT;
      } else if (currentMillis - previousMillis1 >= 100) {
          previousMillis1 = currentMillis;
          state = STATE_GAME_SHOW_PATTERN;
      }
      break;
    }

    case STATE_GAME_WAIT_AFTER_INPUT: {
      uint8_t btnState = readBtn();
      if (btnState > 0) {
        previousMillis1 = currentMillis;
      } else if (currentMillis - previousMillis1 >= 100) {
          previousMillis1 = currentMillis;
          state = STATE_GAME_INPUT;
      }
      break;
    }

    case STATE_GAME_INPUT: {
      if (currentMillis - previousMillis1 >= 100) {
        simonPos = 0;
        previousMillis1 = currentMillis;
        uint8_t btnState = readBtn();
        uint8_t simonInput = 0;
        if (btnState&1) {
          simonInput = 1;
        } else if (btnState&2) {
          simonInput = 2;
        } else if (btnState&4) {
          simonInput = 3;
        } else if (btnState&8) {
          simonInput = 4;
        }
        simonLed(simonInput);
        if (simonInput) {
          counter = 0;
          if ((getSimonValue(simonInputPos)+1)==simonInput) {
            setEyes(2,2);
            simonInputPos++;
            if (simonInputPos>=simonLen) {
              simonLen++;
              simonInputPos = 0;
              simonScore++;
              setHackerScore(simonScore>>1);
              if (simonScore > 12) {
                state = STATE_MUSIC;
              } else {
                state = STATE_GAME_WAIT_AFTER_INPUT;
              }
            } else {
              state = STATE_GAME_WAIT_AFTER_INPUT;
            }
          } else {
              counter = 0;
              codePosition = 0;
              state = STATE_GAME_ALARM;
          }
        } else {
          counter++;
          if (counter > 10) {
            state = STATE_GAME_SHOW_PATTERN;
          }
        }
      }
      break;
    }

    case STATE_GAME_ALARM: {
      if (currentMillis - previousMillis1 >= 50) {
        previousMillis1 = currentMillis;        
        simonLed(0);
        counter++;
        uint8_t counterTarget = 20;
        if (codePosition>0) counterTarget = 200;
        if (counter > counterTarget) {
          setEyes(0, 0);
          simonTone(0);
          counter = 0;
          state = STATE_BOOT;
        } else {
          /* Code */
          uint8_t btnState = readBtn();
          uint8_t codeInput = 0;
          if (btnState&1) {
            codeInput = 1;
          } else if (btnState&2) {
            codeInput = 2;
          } else if (btnState&4) {
            codeInput = 3;
          } else if (btnState&8) {
            codeInput = 4;
          }
          if ((codeInput > 0)&&(simonPos==0)) {
            if (codeInput == (CODE[codePosition]+1)) {
              codePosition++;
              tone(PIN_AUDIO, 100*codePosition);
            } else {
              codePosition = 0;
              noTone(PIN_AUDIO);
            }
            if (codePosition >= sizeof(CODE)) {
              state = STATE_MUSIC2;
            } else {
              state = STATE_GAME_CODE_WAIT;
            }
            counter = 1;
          }
          /* ---- */
          if (codePosition) {
            setEyes((counter&1)<<1, (counter&1)<<1);
          } else {
            setEyes(counter&1, counter&1);
            simonTone((counter&1) ? 3 : 0);
          }
        }
      }
      break;
    }

    case STATE_GAME_CODE_WAIT: {
      if (currentMillis - previousMillis1 >= 200) {
        previousMillis1 = currentMillis;        
        counter = 0;
        state = STATE_GAME_ALARM;
      }
      break;
    }

    case STATE_MAZE: {
      if (NewHallState != HallState) {
        HallState = NewHallState;
        
        if (HallState != 0) {
          simonTone(HallState);         
          if (HallState == mazeCode[mazePos])
            mazeState = (mazeState && true);
          else
            mazeState = false;

          mazePos++;
          mazeCnt++;            
          if (mazeCnt >= 3) {
            if (mazeState) {
              setHackerScore((mazePos)/3);
            } else {
              mazeState = true;
              mazePos   = 0;
              setEyes(1,1);
              setHackerScore(0);
//              state = STATE_GAME_ALARM;
            }
            mazeCnt = 0;
          }
        } else {
          if (mazePos == sizeof(mazeCode))
            state = STATE_MUSIC;
        }
      }
      break;
    }
    
    case STATE_IDLE: {
      break;
    }
  }
}
