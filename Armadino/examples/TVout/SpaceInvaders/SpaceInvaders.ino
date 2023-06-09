/*
  Space Invaders
  Copyright (C) 2013 nootropic design, LLC
  All rights reserved.
 
  Copyright (C) 2015 Francis Tay.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Requires TVout library at Google Code:
  http://code.google.com/p/arduino-tvout/downloads/detail?name=TVoutBeta1.zip

  Requires Hackvision Controllers library:
  http://nootropicdesign.com/downloads/Controllers.zip

  Requires Armadino library:
  http://armadino.com/downloads/Armadino.zip
*/
#include <avr/pgmspace.h>
#include <TVout.h>
#include <fontALL.h>
#include <EEPROM.h>
#define ARMADINO
#ifdef ARMADINO
#include <Armadino.h>
Armadino rmdn;
LedMatrix m;
const uint8_t icon[] PROGMEM = ARMADINO_ICON;
#define W 120
#define H 98
#else
#include <Controllers.h>
#define W 136
#define H 98
#endif
#define LEFT_BUTTON 3
#define RIGHT_BUTTON 2
#define UP_BUTTON 4
#define DOWN_BUTTON 5
#define FIRE_BUTTON 10
#define CANNON_WIDTH 7
#define CANNON_MUZZLE 3
#define CANNON_HEIGHT 3
#define CANNON_Y 88
#define LASER_HEIGHT 3
#define FOOTER_Y 92
#define INVADER_ROWS 5
#define INVADER_COLS 9
#define ROW_SPACING 2
#define COL_SPACING 0
#define BUNKER_WIDTH 14
#define BUNKER_HEIGHT 10
#define BUNKER_Y 76
#define INVADER_HEIGHT 8
#define INVADER_WIDTH 12
//#define DEBUG_INTERVAL 50000
#define LASER_DRAW_INTERVAL 75
#define BOMB_DRAW_INTERVAL 300
#define MYSTERY_SHIP_WIDTH 11
#define MYSTERY_SHIP_DRAW_INTERVAL 300
#define MYSTERY_SHIP_INTERVAL 75000
#define BITMAP_MYSTERY_SHIP 48
#define BITMAP_EXPLOSION 56
#define BITMAP_BLANK 64
#define BITMAP_INVERTED_Y 104
#define INIT_INVADER_ADVANCE_INTERVAL 3000

const uint16_t bitmaps[] PROGMEM = { 
#include "bitmaps.h"
};
const char s0[] PROGMEM = "HIGH SCORES";
const char s1[] PROGMEM = "PLA";
const char s2[] PROGMEM = "SPACE INVADERS";
const char s3[] PROGMEM = " =  10";
const char s4[] PROGMEM = " =  20";
const char s5[] PROGMEM = " =  30";
const char s6[] PROGMEM = " =  ??";
const char s7[] PROGMEM = "GAME";
const char s8[] PROGMEM = "OVER";

const char * const strings[] PROGMEM = {s0, s1, s2, s3, s4, s5, s6, s7, s8};

#ifdef ARMADINO
#else
boolean useNunchuk = false;
#endif
char s[16]; // general string buffer
boolean scored = false;
byte tickFreq[4] = {73, 65, 49, 55};
unsigned long clock = 0;
byte cannonX = 60;
int cannonXF = 6000;
byte oldCannonX = 0;
unsigned int invaders[INVADER_ROWS];
byte bottomRow = INVADER_ROWS-1;
byte leftCol = 0; // leftmost column of invaders
byte rightCol = INVADER_COLS-1; // rightmost column of invaders
char invaderDir = 1;
byte invaderXLeft = 0; 
byte invaderXRight = (INVADER_COLS * INVADER_WIDTH) - 1;
byte invaderY;
byte invaderTypeToggle = 0;
int invaderAdvanceInterval = INIT_INVADER_ADVANCE_INTERVAL;
byte explosionX, explosionY;
byte mysteryShipX = W;
char mysteryShipDir;
byte currentTonePriority = 0;
byte tickFreqCounter = 0;
byte level = 1;

TVout tv;


byte laserX;
char laserY;
byte bombX[3];
char bombY[3] = {-1, -1, -1};
boolean fired = false;
int currentFreq = 0;
byte soundStep = 0;

unsigned int score;
char remainingLives;
char initials[3];

//unsigned long debugTime = 2000;
unsigned long invaderAdvanceTime = invaderAdvanceInterval;
unsigned long laserTime = -1;
unsigned long mysteryShipTime = MYSTERY_SHIP_INTERVAL;
int bombDropInterval = 20000;
unsigned long bombDropTime = bombDropInterval;
unsigned long bombDrawTime = BOMB_DRAW_INTERVAL;
unsigned long eraseExplosionTime = -1;
unsigned long eraseMysteryScoreTime = -1;
unsigned long soundTime = -1;

// Allow the overall speed of the game to be adjusted.
// Higher number (like 1.5) slow the game down.  Lower numbers (like 0.6) speed it up.
float speedAdjust = 1.0;

void setup()  {

#ifdef ARMADINO
  rmdn.begin();
  // If pin A2 is pulled LOW, then the PAL jumper is shorted.
  pinMode(A2, INPUT); digitalWrite(A2, HIGH);
  if (digitalRead(A2) == LOW) {
    tv.begin(_PAL, W, H);
    speedAdjust = 1.4;
  } else tv.begin(_NTSC, W, H);
  tv.set_hbi_hook(Armadino::ledRefresh);
  m.loadBitmap(icon);
  rmdn.show(m);
  randomSeed(analogRead(2));
#else
  // If pin 12 is pulled LOW, then the PAL jumper is shorted.
  pinMode(12, INPUT);
  digitalWrite(12, HIGH);

  if (digitalRead(12) == LOW) {
    tv.begin(_PAL, W, H);
    // Since PAL processing is faster, we need to slow the game play down.
    speedAdjust = 1.4;
  } else {
    tv.begin(_NTSC, W, H);
  }

  randomSeed(analogRead(0));

  // Detect whether nunchuk is connected.  Poll the nunchuk every 4th frame.
  useNunchuk = Nunchuk.init(tv, 4);
  if (useNunchuk) {
    // Speed up game play a bit because of the extra time it takes to
    // communicate with the nunchuk.
    speedAdjust *= 0.8;
  }
#endif

  initSpaceInvaders(false);
      
}


void runSchedule() {

  if (clock > invaderAdvanceTime) {
    advanceInvaders();
    invaderAdvanceTime = clock + (invaderAdvanceInterval * speedAdjust);
  }
  if (clock > laserTime) {
    laserTime = clock + (LASER_DRAW_INTERVAL * speedAdjust);
    drawLaser();
  }
  /*
  if (clock > debugTime) {
    //debug(getMemory());
    debugTime = clock + DEBUG_INTERVAL;
  }
  */
  if (clock > eraseExplosionTime) {
    eraseExplosionTime = -1;
    drawBitmap(explosionX, explosionY, BITMAP_BLANK);
    drawInvaders();
  }

  if (clock > eraseMysteryScoreTime) {
    eraseMysteryScoreTime = -1;
    drawBitmap(mysteryShipX, 0, BITMAP_BLANK);
    mysteryShipX = W;
  }

  if (clock > bombDropTime) {
    dropBomb();
    bombDropTime = clock + (bombDropInterval * speedAdjust);
  }

  if (clock > bombDrawTime) {
    bombDrawTime = clock + (BOMB_DRAW_INTERVAL * speedAdjust);
    if ((bombY[0] != -1) || ((bombY[1] != -1)) || ((bombY[2] != -1))) {
      drawBombs();
    }
  }

  if (clock > mysteryShipTime) {
    if (invaderY < 6) {
      mysteryShipTime = clock + (MYSTERY_SHIP_INTERVAL * speedAdjust);
    } else {
      if ((mysteryShipX == W) || (eraseMysteryScoreTime != -1)) {
	// if the mystery ship is not already on screen
	mysteryShipDir = (random(0, 2) == 0) ? -1 : 1;
	if (mysteryShipDir == -1) {
	  mysteryShipX = W-MYSTERY_SHIP_WIDTH;
	} else {
	  mysteryShipX = 0;
	}
      }
      mysteryShipTime = clock + (MYSTERY_SHIP_DRAW_INTERVAL * speedAdjust);
      handleMysteryShip();
    }
  }

  if (clock > soundTime) {
    playTone(currentFreq, 20, 5);
    currentFreq -= 30;
    soundStep++;
    if (soundStep > 8) {
      soundStep = 0;
      soundTime = -1;
    } else {
      soundTime = clock + (20 * speedAdjust);
    }
  }

}

void handleMysteryShip() {
  /*
  // This sound is too annoying
  if (mysteryShipDir > 0) {
    playTone(880 + (constrain((mysteryShipX % 10), 0, 7) * 22), 100);
  } else {
    playTone(880 + ((9 - constrain((mysteryShipX % 10), 2, 9)) * 22), 100);
  }
  */

  if (mysteryShipDir == 1) {
    tv.draw_line(mysteryShipX-1, 0, mysteryShipX-1, 3, 0);
  }
  drawBitmap(mysteryShipX, 0, BITMAP_MYSTERY_SHIP);
  mysteryShipX += mysteryShipDir;
  if (mysteryShipX > (W-MYSTERY_SHIP_WIDTH)) {
    mysteryShipX = W;
    drawBitmap(0, 0, BITMAP_BLANK);
    drawBitmap((W-MYSTERY_SHIP_WIDTH), 0, BITMAP_BLANK);
    mysteryShipTime = clock + MYSTERY_SHIP_INTERVAL;
  }
}

void loop() {
  spaceInvaders();
}

void spaceInvaders() {
  clock++;
  runSchedule();

  if (remainingLives < 0) {
    gameOver();
    initSpaceInvaders(displayHighScores(0));
  }
  if (leftCol > rightCol) {
    newLevel();
  }

  getInput();
  if (scored) {
    scored = false;
    drawScoreLine();
  }
}


void gameOver() {
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[7])));
  tv.print(38, 40, s);
  strcpy_P(s, (char *)pgm_read_word(&(strings[8])));
  tv.print(66, 40, s);
  tv.delay(3000);

  if (score == 0) {
    return;
  }
  enterHighScore(0); // Space Invaders high scores are in file 0

}

void enterInitials() {
  char index = 0;

  tv.fill(0);
  strcpy_P(s, (char *)pgm_read_word(&(strings[0])));
  s[10] = '\0'; // hack: truncate the final 'S' off of the string "HIGH SCORES"
  tv.print(16, 0, s);
  sprintf(s, "%d", score);
  tv.print(88, 0, s);

  initials[0] = ' ';
  initials[1] = ' ';
  initials[2] = ' ';
  
  while (true) {
    tv.print_char(56, 20, initials[0]);
    tv.print_char(64, 20, initials[1]);
    tv.print_char(72, 20, initials[2]);
    for(byte i=0;i<3;i++) {
      tv.draw_line(56 + (i*8), 27, 56 + (i*8) + 4, 27, 1);
    }
    tv.draw_line(56, 28, 88, 28, 0);
    tv.draw_line(56 + (index*8), 28, 56 + (index*8) + 4, 28, 1);
    tv.delay(160);
#ifdef ARMADINO
    rmdn.checkButtonsDown();
    if (rmdn.buttonLeft) {
#else
    if (useNunchuk) {
      Nunchuk.getData();
    }
    if ((Controller.leftPressed()) || (useNunchuk && (Nunchuk.getJoystickX() < 100))) {
#endif
      index--;
      if (index < 0) {
	index = 0;
      } else {
	playTone(1046, 20);
      }
    }
#ifdef ARMADINO
    if (rmdn.buttonRight) {
#else
    if ((Controller.rightPressed()) || (useNunchuk && (Nunchuk.getJoystickX() > 200))) {
#endif
      index++;
      if (index > 2) {
	index = 2;
      } else {
	playTone(1046, 20);
      }
    }
#ifdef ARMADINO
    if (rmdn.buttonUp) {
#else
    if ((Controller.upPressed()) || (useNunchuk && (Nunchuk.getJoystickY() > 200))) {
#endif
      initials[index]++;
      playTone(523, 20);
      // A-Z 0-9 :-? !-/ ' '
      if (initials[index] == '0') {
	initials[index] = ' ';
      }
      if (initials[index] == '!') {
	initials[index] = 'A';
      }
      if (initials[index] == '[') {
	initials[index] = '0';
      }
      if (initials[index] == '@') {
	initials[index] = '!';
      }
    }
#ifdef ARMADINO
    if (rmdn.buttonDown) {
#else
    if ((Controller.downPressed()) || (useNunchuk && (Nunchuk.getJoystickY() < 100))) {
#endif
      initials[index]--;
      playTone(523, 20);
      if (initials[index] == ' ') {
	initials[index] = '?';
      }
      if (initials[index] == '/') {
	initials[index] = 'Z';
      }
      if (initials[index] == 31) {
	initials[index] = '/';
      }
      if (initials[index] == '@') {
	initials[index] = ' ';
      }
    }
#ifdef ARMADINO
    if (rmdn.buttonA) {
#else
    if ((Controller.firePressed()) || (useNunchuk && (Nunchuk.getButtonZ() == 1))) {
#endif
      if (index < 2) {
	index++;
	playTone(1046, 20);
      } else {
	playTone(1046, 20);
	return;
      }
    }
  }

}

void enterHighScore(byte file) {
  // Each block of EEPROM has 10 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file*10*5;
  byte hi, lo;
  char tmpInitials[3];
  unsigned int tmpScore = 0;

  // High score processing
  for(byte i=0;i<10;i++) {
    hi = EEPROM.read(address + (5*i));
    lo = EEPROM.read(address + (5*i) + 1);
    if ((hi == 0xFF) && (lo == 0xFF)) {
      // The values are uninitialized, so treat this entry 
      // as a score of 0.
      tmpScore = 0;
    } else {
      tmpScore = (hi << 8) | lo;
    }
    if (score > tmpScore) {
      enterInitials();
      for(byte j=i;j<10;j++) {
	hi = EEPROM.read(address + (5*j));
	lo = EEPROM.read(address + (5*j) + 1);
	if ((hi == 0xFF) && (lo == 0xFF)) {
	  tmpScore = 0;
	} else {
	  tmpScore = (hi << 8) | lo;
	}
	tmpInitials[0] = (char)EEPROM.read(address + (5*j) + 2);
	tmpInitials[1] = (char)EEPROM.read(address + (5*j) + 3);
	tmpInitials[2] = (char)EEPROM.read(address + (5*j) + 4);

	// tmpScore and tmpInitials now hold what we want to write in the next slot.

	// write score and initials to current slot
	EEPROM.write(address + (5*j), ((score >> 8) & 0xFF));
	EEPROM.write(address + (5*j) + 1, (score & 0xFF));
	EEPROM.write(address + (5*j) + 2, initials[0]);
	EEPROM.write(address + (5*j) + 3, initials[1]);
	EEPROM.write(address + (5*j) + 4, initials[2]);

	score = tmpScore;
	initials[0] = tmpInitials[0];
	initials[1] = tmpInitials[1];
	initials[2] = tmpInitials[2];
      }
      score = 0;
      initials[0] = ' ';
      initials[1] = ' ';
      initials[2] = ' ';
      return;
    }
  }
}


boolean displayHighScores(byte file) {
  byte y = 10;
  byte x = 18;
  char s[16];
  // Each block of EEPROM has 10 high scores, and each high score entry
  // is 5 bytes long:  3 bytes for initials and two bytes for score.
  int address = file*10*5;
  byte hi, lo;
  tv.fill(0);
  tv.select_font(font6x8);
  strcpy_P(s, (char *)pgm_read_word(&(strings[0])));
  tv.print(34, 0, s);
  for(int i=0;i<10;i++) {
    sprintf(s, "%2d", i+1);
    tv.print(x, y+(i*8), s);

    hi = EEPROM.read(address + (5*i));
    lo = EEPROM.read(address + (5*i) + 1);
    if ((hi == 0xFF) && (lo == 0xFF)) {
      score = 0;
    } else {
      score = (hi << 8) | lo;
    }
    initials[0] = (char)EEPROM.read(address + (5*i) + 2);
    initials[1] = (char)EEPROM.read(address + (5*i) + 3);
    initials[2] = (char)EEPROM.read(address + (5*i) + 4);

    if (score > 0) {
      sprintf(s, "%c%c%c %d", initials[0], initials[1], initials[2], score);
      tv.print(x+24, y+(i*8), s);
    }
  }

  if (pollFireButton(500)) {
    return true;
  }
  return false;
}



boolean titleScreen() {
  byte x, y;
  int d;

  tv.fill(0);
  tv.delay(32);
  tv.select_font(font6x8);

  d = 10;

  for(x=1;x<=3;x++) {
    strcpy_P(s, (char *)pgm_read_word(&(strings[1])));
    // cleverness to make the letters appear one at a time.
    // truncate the string with a null character.
    s[x] = '\0';
    tv.print(50, 20, s);
    if (pollFireButton(d)) {
      return true;
    }
  }
  drawBitmap(68, 20, BITMAP_INVERTED_Y);
  if (pollFireButton(d)) {
    return true;
  }

  for(x=1;x<=14;x++) {
    if (x == 6) continue; // don't delay on the space char
    strcpy_P(s, (char *)pgm_read_word(&(strings[2])));
    s[x] = '\0';
    tv.print(22, 30, s);
    if (pollFireButton(d)) {
      return true;
    }
  }

  x = 74;

  tv.select_font(font4x6);
  
  d = 60;
  if (pollFireButton(d)) {
    return true;
  }

  y = 49;
  drawBitmap(42, y, BITMAP_MYSTERY_SHIP);
  strcpy_P(s, (char *)pgm_read_word(&(strings[6])));
  tv.print(58, y, s);
  if (pollFireButton(d)) {
    return true;
  }


  y = 58;
  drawBitmap(41, y, 0);
  strcpy_P(s, (char *)pgm_read_word(&(strings[5])));
  tv.print(58, y+1, s);
  if (pollFireButton(d)) {
    return true;
  }


  y = 68;
  drawBitmap(42, y, 16);
  strcpy_P(s, (char *)pgm_read_word(&(strings[4])));
  tv.print(58, y+1, s);

  if (pollFireButton(d)) {
    return true;
  }

  y = 78;
  drawBitmap(42, y, 32);
  strcpy_P(s, (char *)pgm_read_word(&(strings[3])));
  tv.print(58, y+1, s);

  y = 19;
  for(x=W+6;x>73;x--) {
    drawBitmap(x, y, BITMAP_BLANK);
    drawBitmap(x, y, 0);
    if (pollFireButton(2)) {
      return true;
    }
    x--;
    drawBitmap(x, y, BITMAP_BLANK);
    drawBitmap(x, y, 8);
    if (pollFireButton(2)) {
      return true;
    }
  }
  tv.delay(160);

  for(x=74;x<W+5;x++) {
    drawBitmap(x-6, y+1, BITMAP_BLANK);
    drawBitmap(x-1, y, BITMAP_BLANK);
    drawBitmap(x-5, y+1, BITMAP_INVERTED_Y);
    drawBitmap(x, y, 0);
    if (pollFireButton(2)) {
      return true;
    }
    x++;
    drawBitmap(x-6, y+1, BITMAP_BLANK);
    drawBitmap(x-1, y, BITMAP_BLANK);
    drawBitmap(x-5, y+1, BITMAP_INVERTED_Y);
    drawBitmap(x, y, 8);
    if (pollFireButton(2)) {
      return true;
    }
  }
  tv.delay(320);
  
  for(x=W+5;x>73;x--) {
    drawBitmap(x-6, y+1, BITMAP_BLANK);
    drawBitmap(x-1, y, BITMAP_BLANK);
    drawBitmap(x-5, y+1, BITMAP_INVERTED_Y+8);
    drawBitmap(x, y, 0);
    if (pollFireButton(2)) {
      return true;
    }
    x--;

    drawBitmap(x-6, y+1, BITMAP_BLANK);
    drawBitmap(x-1, y, BITMAP_BLANK);
    drawBitmap(x-5, y+1, BITMAP_INVERTED_Y+8);
    drawBitmap(x, y, 8);
    if (pollFireButton(2)) {
      return true;
    }
  }
  tv.delay(500);
  drawBitmap(74, y, BITMAP_BLANK);

  if (pollFireButton(120)) {
    return true;
  }
  return false;
}

boolean pollFireButton(int n) {
  for(int i=0;i<n;i++) {
#ifdef ARMADINO
    rmdn.checkButtonsDown();
    tv.delay(16);
    if (rmdn.buttonA) {
      return true;
    }
#else
    if (useNunchuk) {
      Nunchuk.getData();
    }
    tv.delay(16);
    if ((Controller.firePressed()) || (useNunchuk && (Nunchuk.getButtonZ() == 1))) {
      return true;
    }
#endif
  }
  return false;
}


void drawFrame() {
  tv.draw_line(0, 0, W-1, 0, 1);
  tv.draw_line(0, 0, 0, H-1, 1);
  tv.draw_line(W-1, H-1, W-1, 0, 1);
  tv.draw_line(W-1, H-1, 0, H-1, 1);
}

void newLevel() {
  level++;
  tv.delay(1000);
  tv.fill(0);
  tv.select_font(font6x8);
  char s[4];
  sprintf(s, "%d", level);
  tv.print(42, 40, "LEVEL ");
  tv.print(74, 40, s);
  tv.delay(3000);
  tv.fill(0);
  tv.select_font(font4x6);

  initVars();
  drawRemainingLives();
  drawBunkers();
  oldCannonX = cannonX-1;
  drawCannon(cannonX, CANNON_Y, 1);
}

void initSpaceInvaders(boolean start) {
  tv.fill(0);

  while (!start) {
    start = titleScreen();
    if (!start) {
      start = displayHighScores(0);
    }
  }
  tv.fill(0);
  tv.select_font(font4x6);

  level = 1;
  remainingLives = 2;
  score = 0;
  initVars();
  drawBunkers();
  oldCannonX = cannonX-1;
  drawCannon(cannonX, CANNON_Y, 1);
  drawScoreLine();
  drawRemainingLives();
  tv.delay(500);
}

void initVars() {
  clock = 0;
  //  debugTime = DEBUG_INTERVAL;
  eraseExplosionTime = -1;
  eraseMysteryScoreTime = -1;
  soundTime = -1;
  bombDropInterval = 20000;
  bombDropTime = bombDropInterval;
  bombDrawTime = BOMB_DRAW_INTERVAL;
  bombY[0] = -1;
  bombY[1] = -1;
  bombY[2] = -1;
  mysteryShipX = W;
  mysteryShipTime = MYSTERY_SHIP_INTERVAL;
  invaderAdvanceInterval = INIT_INVADER_ADVANCE_INTERVAL - ((level-1) * 200);
  invaderAdvanceTime = invaderAdvanceInterval;
  leftCol = 0;
  rightCol = INVADER_COLS-1;
  bottomRow = INVADER_ROWS-1;
  invaderY = 4 + (level-1)*2;
  invaderXLeft = 0; 
  invaderXRight = (INVADER_COLS * INVADER_WIDTH) - 1;
  for(byte r=0;r<INVADER_ROWS;r++) {
    invaders[r] = 0xff80;
  }
}


void drawScoreLine() {
  // First, erase;
  for(byte y=FOOTER_Y;y<H;y++) {
    tv.draw_line(W/2, FOOTER_Y+y, W-1, FOOTER_Y+y, 0);
  }
  sprintf(s, "%d", score);
  byte len = strlen(s);
  byte xx = W  - (len*4) - 6;
  tv.print(xx, FOOTER_Y, s);
}

void drawCannon(byte x, byte y, byte color) {
  tv.set_pixel(x+CANNON_MUZZLE, y, color);
  for(byte r=y+1;r<(y+CANNON_HEIGHT);r++) {
    for(byte c=x;c<x+CANNON_WIDTH;c++) {
      tv.set_pixel(c, r, color);
    }
  }
}

void drawRemainingLives() {
  for(byte i=0;i<3;i++) {
    drawCannon(i*(CANNON_WIDTH+1), H-CANNON_HEIGHT-1, 0);
  }
  for(byte i=0;i<remainingLives;i++) {
    drawCannon(i*(CANNON_WIDTH+1), H-CANNON_HEIGHT-1, 1);
  }
}

/*
void debug(int m) {
  sprintf(s, "       ");
  tv.print(64, FOOTER_Y, s);
  sprintf(s, "%d", m);
  tv.print(64, FOOTER_Y, s);
}

void debug(char *ss) {
  for(byte y=FOOTER_Y;y<H;y++) {
    tv.draw_line(48, y, 90, y, 0);
  }
  tv.print(64, FOOTER_Y, ss);
}
*/

void advanceInvaders() {
  invaderTypeToggle = ++invaderTypeToggle % 2;  // toggle between 0 and 1

  if (invaderTypeToggle == 0) {
    tick();
  }

  if (((invaderXLeft == 0) && (invaderDir < 0)) || ((invaderXRight == W-1) && (invaderDir > 0))) {
    // reverse direction
    invaderDir = -invaderDir;

    for(byte r=0;r<=bottomRow;r++) {
      byte offset = r*(INVADER_HEIGHT+ROW_SPACING);
      tv.draw_line(0, invaderY+offset, W-1, invaderY+offset, 0);
      tv.draw_line(0, invaderY+offset+1, W-1, invaderY+offset+1, 0);
    }
    invaderY += 2;
    if ((invaderY + ((INVADER_HEIGHT+ROW_SPACING)*(bottomRow+1))) >= (BUNKER_Y + BUNKER_HEIGHT + 4)) {
      drawInvaders();
      // if invaders reach bottom of screen, game is over.
      remainingLives = -1;
      destroyCannon();
    }
  } else {
    invaderXLeft += invaderDir;
    invaderXRight += invaderDir;
  }

  drawInvaders();

}


void drawInvaders() {
  byte x;
  byte y;

  for(byte r=0;r<=bottomRow;r++) {
    for(byte c=leftCol;c<=rightCol;c++) {

      //smooths input
      for(byte i=0;i<5;i++) {
	getInput();
      }

      x = invaderXLeft + ((c-leftCol) * (INVADER_WIDTH+COL_SPACING));
      y = invaderY + (r * (INVADER_HEIGHT+ROW_SPACING));
      if (invaders[r] & (1<<(15-c))) {
	clock += 10;
	// invader here, so draw it
	tv.draw_line(x-invaderDir, y, x-invaderDir, y+INVADER_HEIGHT-1, 0);
	switch (r) {
	case 0:
	  drawBitmap(x, y, (invaderTypeToggle*8)); // invader type 0
	  break;
	case 1:
	case 2:
	  drawBitmap(x, y, 16+(invaderTypeToggle*8)); // invader type 1
	  break;
	case 3:
	case 4:
	  drawBitmap(x, y, 32+(invaderTypeToggle*8)); // invader type 2
	  break;
	}
      }
    }
  }
}


void drawBitmap(byte x, byte y, unsigned int bitmapIndex) {
  const uint16_t *index = bitmaps + bitmapIndex;
  unsigned int ii;
  byte hi, lo;
  byte pixelOffset;
  int byteIndex;
  byte rows = 8;

  if ((x >= W) || (y >= H)) {
    return;
  }

  if (bitmapIndex == BITMAP_MYSTERY_SHIP) {
    rows = 5;
  }

  byteIndex = (y*(W/8)) + (x/8);
  pixelOffset = x % 8;
  boolean nowrap1 = ((byteIndex+1) % (W/8)) != 0;
  boolean nowrap2 = ((byteIndex+2) % (W/8)) != 0;
  for(byte l=0;l<rows;l++) {
    ii = pgm_read_word(index+l);
    hi = ((ii >> 8) & 0xFF);
    lo = (ii & 0xFF);
    display.screen[byteIndex] &= (0xFF << (8-pixelOffset)); // keep upper 'pixelOffset' bits
    display.screen[byteIndex] |= (hi >> pixelOffset);
    if (pixelOffset <= 4) {
      if (nowrap1) {
	display.screen[byteIndex+1] &= (0x0F >> pixelOffset); // keep lower 4-pixelOffset bits
	display.screen[byteIndex+1] = ((hi << (8-pixelOffset) | (lo >> pixelOffset)));
      }
    } else {
      // The 12 bit row spans 3 bytes
      if (nowrap1) {
	display.screen[byteIndex+1] = (hi << (8-pixelOffset)) | (lo >> pixelOffset);
	if (nowrap2) {
	  display.screen[byteIndex+2] &= (0xFF >> (pixelOffset-4));
	  display.screen[byteIndex+2] |= (lo << (8-pixelOffset));
	}
      }
    }
    byteIndex += (W/8);
  }      
}


void drawBunkers() {
  for(byte i=0;i<4;i++) {
    drawBunker(((W/4)*i) +  (W/4/2) - (BUNKER_WIDTH/2), BUNKER_Y);
  }
}

void drawBunker(byte x, byte y) {
  tv.draw_line(x+3, y, x+BUNKER_WIDTH-3, y, 1);
  tv.draw_line(x+2, y+1, x+BUNKER_WIDTH-2, y+1, 1);
  tv.draw_line(x+1, y+2, x+BUNKER_WIDTH-1, y+2, 1);
  for(byte r=y+3;r<(y+BUNKER_HEIGHT);r++) {
    tv.draw_line(x, r, x+BUNKER_WIDTH, r, 1);
  }
  //  tv.draw_line(x+7, y+BUNKER_HEIGHT-4, x+9, y+BUNKER_HEIGHT-4, 0);
  tv.draw_line(x+5, y+BUNKER_HEIGHT-3, x+9, y+BUNKER_HEIGHT-3, 0);
  tv.draw_line(x+4, y+BUNKER_HEIGHT-2, x+10, y+BUNKER_HEIGHT-2, 0);
  tv.draw_line(x+4, y+BUNKER_HEIGHT-1, x+10, y+BUNKER_HEIGHT-1, 0);
}

int getMemory() {
  int size = 512;
  byte *buf;
  while ((buf = (byte *) malloc(--size)) == NULL);
  free(buf);
  return size;
} 

void drawLaser() {
  byte x, y;
  byte r, c;

  if (fired) {
    // erase lowest pixel of laser
    tv.set_pixel(laserX, laserY+2, 0);
    laserY--;

    if (tv.get_pixel(laserX, laserY) != 0) {
      // we hit something!
      tv.set_pixel(laserX, laserY, 0);
      tv.set_pixel(laserX, laserY+1, 0);
      tv.set_pixel(laserX, laserY+2, 0);
      fired = false;
      laserTime = -1;

      for(r=0;r<=bottomRow;r++) {
	for(c=leftCol;c<=rightCol;c++) {
	  if (invaders[r] & (1<<(15-c))) {
	    // (x,y) is upper left corner of invader
	    x = invaderXLeft + ((c-leftCol) * (INVADER_WIDTH+COL_SPACING));
	    y = invaderY + (r * (INVADER_HEIGHT+ROW_SPACING));
	    if ((laserX >= x) && (laserX <= x+INVADER_WIDTH) && (laserY >= y) && (laserY <= y+INVADER_HEIGHT)) {
	      // We hit an invader!
	      invaders[r] &= ~(1<<15-c);
	      if (eraseExplosionTime != -1) {
		// There is already an explosion displayed.  Erase it.
		drawBitmap(explosionX, explosionY, BITMAP_BLANK);
	      }
	      drawBitmap(x, y, BITMAP_EXPLOSION);

	      soundTime = clock;
	      currentFreq = 1046;

	      eraseExplosionTime = clock + 1000;
	      explosionX = x;
	      explosionY = y;
	      bombDropInterval -= 500;
	      if (bombDropInterval < 1000) {
		bombDropInterval = 1000;
	      }

	      invaderAdvanceInterval -= 60;
	      if (invaderAdvanceInterval < 30) {
		invaderAdvanceInterval = 30;
	      }
	      score += 10;
	      if (r <= 2) {
		score += 10;
	      }
	      if (r == 0) {
		score += 10;
	      }
	      scored = true;

	      // Adjust values of leftCol, rightCol, and bottomRow if needed
	      // If we destroyed the last invader, then leftCol > rightCol.
	      if (c == leftCol) {
		while ((columnEmpty(leftCol)) && (leftCol < INVADER_COLS)) {
		  invaderXLeft += (INVADER_WIDTH+COL_SPACING);
		  leftCol++;
		}
	      }
	      if (c == rightCol) {
		while ((columnEmpty(rightCol)) && (rightCol > 0)) {
		  invaderXRight -= (INVADER_WIDTH+COL_SPACING);
		  rightCol--;
		}
	      }
	      if (r == bottomRow) {
		while ((rowEmpty(bottomRow)) && (bottomRow > 0)) {
		  bottomRow--;
		}
	      }
	      return;
	    }
	  }
	}
      }

      // Check to see if we hit a falling bomb.
      for(byte i=0;i<3;i++) {
	if ((laserX == bombX[i]) && (laserY <= bombY[i]) && (laserY >= bombY[i]-2)) {
	  // erase the laser
	  tv.set_pixel(laserX, laserY, 0);
	  tv.set_pixel(laserX, laserY+1, 0);
	  tv.set_pixel(laserX, laserY+2, 0);
	  // erase the bomb
	  tv.set_pixel(bombX[i], bombY[i], 0);
	  tv.set_pixel(bombX[i], bombY[i]-1, 0);
	  tv.set_pixel(bombX[i], bombY[i]-2, 0);
	  bombY[i] = -1;
	  return;
	}
      }

      // Check to see if we hit the mystery ship
      if ((laserY < 5) && (laserY >= 0) && (laserX >= mysteryShipX) && (laserX < mysteryShipX+MYSTERY_SHIP_WIDTH) && (eraseMysteryScoreTime == -1)) {
	playTone(1046, 20);
	tv.delay(16);
	playTone(1318, 20);
	tv.delay(16);
	playTone(1568, 20);
	tv.delay(16);
	playTone(2093, 20);

	byte bitmap = BITMAP_BLANK + 8;
	byte mysteryScore = random(0, 4);
	bitmap += mysteryScore * 8;
	switch (mysteryScore) {
	case 0:
	  score += 50;
	  break;
	case 1:
	  score += 100;
	  break;
	case 2:
	  score += 150;
	  break;
	case 3:
	  score += 300;
	}
	scored = true;
	drawBitmap(mysteryShipX, 0, bitmap);
	tv.draw_line(mysteryShipX-1, 0, mysteryShipX-1, 3, 0);
	eraseMysteryScoreTime = clock + 10000;
	mysteryShipTime = clock + MYSTERY_SHIP_INTERVAL;
	return;
      }

      // We hit a bunker.
      if (laserY >= BUNKER_Y) {
	damage(laserX, laserY);
      }
      return;
    }

    if (laserY >= 0) {
      tv.set_pixel(laserX, laserY, 1);
    } else {
      // laser has reached top of screen
      tv.set_pixel(laserX, laserY+1, 0);
      tv.set_pixel(laserX, laserY+2, 0);
      fired = false;
      laserTime = -1;
    }
  }
}

boolean columnEmpty(byte c) {
  for(byte r=0;r<INVADER_ROWS;r++) {
    if (invaders[r] & (1<<(15-c))) {
      return false;
    }
  }
  return true;
}

boolean rowEmpty(byte r) {
  for(byte c=0;c<INVADER_COLS;c++) {
    if (invaders[r] & (1<<(15-c))) {
      return false;
    }
  }
  return true;
}

void dropBomb() {
  boolean done = false;
  char c, r;
  byte i = 0;
  while (bombY[i] != -1) {
    i++;
    if (i >= 3) {
      return;
    }
  }

  while (!done) {
    // randomly choose a column of invaders
    c = (byte)random(leftCol, rightCol+1);
    // look for the bottom-most invader in that column
    // note that there may not be any invaders in the column
    for(r=bottomRow;r>=0;r--) {
      if (invaders[r] & (1<<(15-c))) {
	done = true;
	break; // break out of for loop
      }
    }
  }
  bombX[i] = invaderXLeft + ((c-leftCol) * (INVADER_WIDTH+COL_SPACING)) + INVADER_WIDTH/2;
  bombY[i] = invaderY + ((r+1) * (INVADER_HEIGHT+ROW_SPACING)) - 2;
  tv.set_pixel(bombX[i], bombY[i], 1);
  tv.set_pixel(bombX[i], bombY[i]-1, 1);
  tv.set_pixel(bombX[i], bombY[i]-2, 1);
}

void drawBombs() {
  // draw the bombs and detect collisions
  for(byte i=0;i<3;i++) {
    if (bombY[i] == -1) {
      continue;
    }
    tv.set_pixel(bombX[i], bombY[i]-2, 0);
    bombY[i]++;

    if (bombY[i] >= FOOTER_Y) {
      // bomb has reached bottom of screen
      tv.set_pixel(bombX[i], bombY[i]-1, 0);
      tv.set_pixel(bombX[i], bombY[i]-2, 0);
      bombY[i] = -1;
      return;
    }

    if (tv.get_pixel(bombX[i], bombY[i]) == 1) {
      // bomb hit something!
      if ((bombX[i] == laserX) && (bombY[i] == laserY)) {
	// collision between laser and bomb is handled in drawLaser()
	return;
      }

      // process collisions
      if ((bombX[i] >= cannonX) && (bombX[i] < cannonX+CANNON_WIDTH) && (bombY[i] >= CANNON_Y) && (bombY[i] <= CANNON_Y+CANNON_HEIGHT)) {
	// laser cannon destroyed!
	tv.set_pixel(bombX[i], bombY[i], 0);
	tv.set_pixel(bombX[i], bombY[i]-1, 0);
	tv.set_pixel(bombX[i], bombY[i]-2, 0);
	destroyCannon();
	fired = false;
	tv.set_pixel(laserX, laserY, 0);
	tv.set_pixel(laserX, laserY+1, 0);
	tv.set_pixel(laserX, laserY+2, 0);
	remainingLives--;
	drawRemainingLives();
	bombY[i] = -1;
	return;
      }

      tv.set_pixel(bombX[i], bombY[i], 0);
      tv.set_pixel(bombX[i], bombY[i]-1, 0);
      tv.set_pixel(bombX[i], bombY[i]-2, 0);
      damage(bombX[i], bombY[i]);

      bombY[i] = -1;
      return;
    }

    tv.set_pixel(bombX[i], bombY[i], 1);
  }

}

void damage(byte x, byte y) {
  for(byte yy=(y-2);yy<=(y+2);yy++) {
    for(byte xx=(x-2);xx<=(x+2);xx++) {
      if (random(0, 1 + abs(xx-x) + abs(yy-y)) < 1) {
	tv.set_pixel(xx, yy, 0);
      }
    }
  }
}

void destroyCannon() {
  for(byte i=0;i<50;i++) {
    playTone(random(20, 50), 10);
    tv.delay(1);
    tv.set_pixel(random(cannonX, cannonX+CANNON_WIDTH), CANNON_Y + random(0, CANNON_HEIGHT-1), random(0, 2));
  }
  for(byte y=CANNON_Y;y<(CANNON_Y+2);y++) {
    for(byte x=cannonX;x<cannonX+CANNON_WIDTH;x++) {
      tv.set_pixel(x, y, 0);
    }
  }
  tv.delay(1000);
  drawCannon(cannonX, CANNON_Y, 1);
}

boolean getInput() {
  boolean input = false;

#ifdef ARMADINO
  rmdn.checkButtonsDown();
  if (rmdn.buttonA) {
#else
  if (useNunchuk) {
    Nunchuk.getData();
  }

  if ((Controller.firePressed()) || (useNunchuk && (Nunchuk.getButtonZ() == 1))) {
#endif
    if (!fired) {
      fired = true;
      laserTime = clock;
      laserX = cannonX + (CANNON_WIDTH/2);
      laserY = CANNON_Y-LASER_HEIGHT;
      input = true;
    }
  }

#ifdef ARMADINO
  if ((rmdn.buttonLeft)
#else
  if (((Controller.leftPressed()) || (useNunchuk && ((Nunchuk.getJoystickX() < 100) || (Nunchuk.getAccelerometerX() < 100))))
#endif
    && (cannonX > 0)) {
    oldCannonX = cannonX;
    cannonXF--;
    cannonX = (cannonXF +5) / 100;
    if (oldCannonX != cannonX) {
      // the cannon moved
      // erase it
      drawCannon(oldCannonX, CANNON_Y, 0);
      oldCannonX = cannonX;
      // draw it
      drawCannon(cannonX, CANNON_Y, 1);
      clock += 60/speedAdjust;
    }
    return true;
  } else {
#ifdef ARMADINO
    if ((rmdn.buttonRight) 
#else
    if (((Controller.rightPressed()) || (useNunchuk && ((Nunchuk.getJoystickX() > 200) || (Nunchuk.getAccelerometerX() > 150))))
#endif
      && (cannonX < (W-CANNON_WIDTH))) {
      oldCannonX = cannonX;
      cannonXF++;
      cannonX = (cannonXF + 5) / 100;
      if (oldCannonX != cannonX) {
	// the cannon moved
	// erase it
	drawCannon(oldCannonX, CANNON_Y, 0);
	oldCannonX = cannonX;
	// draw it
	drawCannon(cannonX, CANNON_Y, 1);
	clock += 60/speedAdjust;
      }
      return true;
    }
  }
  return input;
}


void tick() {
  playTone(tickFreq[tickFreqCounter++], 60, 1);
  tickFreqCounter = tickFreqCounter % 4;
}



void playTone(unsigned int frequency, unsigned long duration_ms) {
  // Default is to play tone with highest priority.
  playTone(frequency, duration_ms, 9);
}

void playTone(unsigned int frequency, unsigned long duration_ms, byte priority) {
  // priority is value 0-9, 9 being highest priority
#ifdef ARMADINO
  if (rmdn.makingTone()) {
#else
  if (TCCR2B > 0) {
#endif
    // If a tone is currently playing, check priority
    if (priority < currentTonePriority) {
      return;
    }
  }
  currentTonePriority = priority;
#ifdef ARMADINO
  rmdn.tone(frequency, duration_ms);
#else
  tv.tone(frequency, duration_ms);
#endif
}
