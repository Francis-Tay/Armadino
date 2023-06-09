#include <Armadino.h>
#include <Wire.h>
#include "RTClib.h"

Armadino rmdn;
LedMatrix m;
Led7Seg s;
int i;

void (*func)(void);

void setup () {
  rmdn.begin();
  rmdn.show(m);
  rmdn.show(s);
  rmdn.setToneLevel(8);
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 0, RED); m.setPixel(i, 4, RED); }
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 1, ORANGE); m.setPixel(i, 5, ORANGE); }
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 2, GREEN); m.setPixel(i, 6, GREEN); }
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 3, YELLOW); m.setPixel(i, 7, YELLOW); }
  s.pattern = 0x7FFF;
  rmdn.delay(1000);
  for ( i=0; i<MAX_ROW; i++) { m.shiftUp(); rmdn.delay(250); }
  s.pattern = 0;
  m.printStr("Ar", RED, YELLOW);
  m.printStr("ma", GREEN, ORANGE);
  m.printStr("di", RED, YELLOW);
  m.printStr("no  ", GREEN, ORANGE);
  randomSeed(analogRead(1));
  switch (menu()) {
    case 1: clock_setup(); func = clock_loop; break;
    case 2: pong_setup(); func = pong_loop; break;
    case 3: snake_setup(); func = snake_loop; break;
    case 4: test_setup(); func = test_loop; break;
  }
}

#define MENU_ITEMS 4
int menu() {
  int i = 1;
  while (-1) {
    rmdn.checkButtonsPress();
    if ( rmdn.buttonA ) {
      m.clearMatrix();
      return i;
    }
    else
      if ( rmdn.buttonUp && --i == 0 ) i = MENU_ITEMS;
      else
        if ( rmdn.buttonDown && ++i > MENU_ITEMS) i = 1;
        else {
          m.printStr("  ");
          switch (i) {
            case 1: m.printStr("Clock", RED, GREEN); break;
            case 2: m.printStr("Pong", GREEN, ORANGE); break;
            case 3: m.printStr("Snake", ORANGE, RED); break;
            case 4: m.printStr("Test", YELLOW, GREEN); break;
          }
        }
    rmdn.delay(250);
  }
}

void loop() {
  func();
}

void scrollClock(LedMatrix &m, byte hr, byte mn, byte color1, byte color2);
void menuDateTime(), upDateTime(), downDateTime();
void setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0,
    uint8_t min = 0, uint8_t sec = 0);

LedMatrix q;
RTC_DS1307 rtc;
DateTime now;
uint16_t t, b, l;
uint8_t hour, min, sec, day, month, year, clock_menu = 0, changed = 0,
  alarmLevel, alarmHour, alarmMin, alarmBuzz;
int alarmMelody[] = { C_6, N_16, C_6, N_16, C_6, N_16, C_6, N_16, Rest, N_4, End };

#define TEMPERATURE_PIN 3
#define LDR_PIN 2

#define TABLE_SIZE 10
const char string0[] PROGMEM = "To thine own self be true.";
const char string1[] PROGMEM = "All the world's a stage.";
const char string2[] PROGMEM = "Brevity is the soul of wit.";
const char string3[] PROGMEM = "Action is eloquence.";
const char string4[] PROGMEM = "There is a tide in the affairs of men...";
const char string5[] PROGMEM = "Love all, trust a few, do wrong to none.";
const char string6[] PROGMEM = "Life is of a mingled yarn, good and ill together.";
const char string7[] PROGMEM = "The better part of valor is discretion.";
const char string8[] PROGMEM = "Some rise by sin, and some by virtue fall.";
const char string9[] PROGMEM = "Our doubts are traitors.";
const char * const stringTable[] PROGMEM = {
  string0, string1, string2, string3, string4,
  string5, string6, string7, string8, string9
};
char buffer[80];

void clock_setup () {
  PORTC &= ~(1<<TEMPERATURE_PIN);    // Disable pull-up on temperature sensor pin
  PORTC |= (1<<LDR_PIN);    // Enable pull-up on LDR sensor pin, 20kohms on ATMEGA328
  analogReference(DEFAULT);
  rtc.begin();
  if (!rtc.isrunning()) {
    m.printStr("RTC is not running!", ORANGE);
  }
  // The following line initialises the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(__DATE__, __TIME__));
  // setAlarm(0, 0, 0);
  alarmLevel=rtc.readnvram(0);
  if (alarmLevel<0 || alarmLevel>3) alarmLevel=0;
  alarmHour=rtc.readnvram(1);
  if (alarmHour<0 || alarmHour>23) alarmHour=0;
  alarmMin=rtc.readnvram(2);
  if (alarmMin<0 || alarmMin>59) alarmMin=0;
  analogRead(TEMPERATURE_PIN);
  l = analogRead(LDR_PIN);
}

void clock_loop () {
  b = rmdn.checkButtonsPress();
  if (alarmBuzz==1)
    if (b || min!=alarmMin) {
      rmdn.stopMelody();
      rmdn.setToneLevel(8);
      alarmBuzz = 0;
      b = 0;
    }
  switch (b) {
    case BUTTON_A:
      if (clock_menu > 0) {
        clock_menu = 0;
        menuDateTime();
      }  
      t = ((analogRead(TEMPERATURE_PIN)*5000L)/1023-500)/10; // LM36
      s.print(t);
      m.printStr("   ");
      switch (now.dayOfTheWeek()) {
        case 1: m.printStr("Mon", RED, GREEN); break;
        case 2: m.printStr("Tue", RED, GREEN); break;
        case 3: m.printStr("Wed", RED, GREEN); break;
        case 4: m.printStr("Thu", RED, GREEN); break;
        case 5: m.printStr("Fri", RED, GREEN); break;
        case 6: m.printStr("Sat", RED, GREEN); break;
        case 7: m.printStr("Sun", RED, GREEN); break;
      }
      m.print(" ");
      m.setPenColor(RED); m.print(day/10);
      m.setPenColor(GREEN); m.print(day%10);
      m.print(" ");
      switch (month) {
        case 1: m.printStr("Jan", RED, GREEN); break;
        case 2: m.printStr("Feb", RED, GREEN); break;
        case 3: m.printStr("Mar", RED, GREEN); break;
        case 4: m.printStr("Apr", RED, GREEN); break;
        case 5: m.printStr("May", RED, GREEN); break;
        case 6: m.printStr("Jun", RED, GREEN); break;
        case 7: m.printStr("Jul", RED, GREEN); break;
        case 8: m.printStr("Aug", RED, GREEN); break;
        case 9: m.printStr("Sep", RED, GREEN); break;
        case 10: m.printStr("Oct", RED, GREEN); break;
        case 11: m.printStr("Nov", RED, GREEN); break;
        case 12: m.printStr("Dec", RED, GREEN); break;
      }
      m.print(" ");
      m.setPenColor(RED); m.print(year/10);
      m.setPenColor(GREEN); m.print(year%10);
      clock_menu = 0;
      rmdn.delay(500);
      break;
    case BUTTON_B:  
      if (clock_menu > 0) {
        clock_menu = 0;
        menuDateTime();
      }  
      s.pattern = 0x00;
      m.printStr("   ");
      strcpy_P( buffer, (char *)pgm_read_word(&(stringTable[random(TABLE_SIZE)])) );
      m.printStr(buffer, RED, GREEN);
      m.printStr("   ");
      rmdn.delay(500);
      break;
    case BUTTON_RIGHT:
      clock_menu++;
      menuDateTime();
      break;
    case BUTTON_LEFT:
      clock_menu--;
      menuDateTime();
      break;
    case BUTTON_UP:
      if (clock_menu > 0) upDateTime();
      break;
    case BUTTON_DOWN:
      if (clock_menu > 0) downDateTime();
      break;
  }
  if (clock_menu == 0) {
    now = rtc.now();
    day = now.day();
    month = now.month();
    year = now.year()-2000;
    sec = now.second();
    min = now.minute();
    hour = now.hour();
    if (hour==alarmHour && min==alarmMin && sec<2 &&
        alarmLevel>0 && alarmBuzz==0 ) {
      alarmBuzz = 1;
      rmdn.setToneLevel(16<<alarmLevel);
      rmdn.playMelody(alarmMelody, REPEAT);
    }
    l=analogRead(LDR_PIN);
    if (l > 900) { // very bright ambient
      rmdn.setBrightLevel(HIGH_BRIGHTLEVEL);
      scrollClock(q, hour, min, RED, GREEN);
      s.brightness=MAX_BRIGHTNESS; s.print(sec);
    }
    else
      if (l > 750) { // bright ambient
        rmdn.setBrightLevel(HIGH_BRIGHTLEVEL);
        scrollClock(q, hour, min, ORANGE, GREEN);
        s.brightness=MAX_BRIGHTNESS; s.print(sec);
      }
      else
        if (l > 500) { // shaded ambient
          rmdn.setBrightLevel(LOW_BRIGHTLEVEL);
          scrollClock(q, hour, min, ORANGE, GREEN);
          s.brightness=4; s.print(sec);
        }
        else
          if (l > 300) { // very shaded ambient
            rmdn.setBrightLevel(LOW_BRIGHTLEVEL);
            scrollClock(q, hour, min, MIDORANGE, MIDGREEN);
            s.brightness=2; s.print(sec);
            // if (sec%10==0) s.pattern=0x8000; else s.pattern=0x0000;
          }
          else
            { // dark ambient
              rmdn.setBrightLevel(LOW_BRIGHTLEVEL);
              scrollClock(q, hour, min, LIGHTORANGE, LIGHTGREEN);
              // s.brightness=1; s.print(sec);
              if (sec%10==0) s.pattern = 0x8000; else s.pattern=0x0000;
            }
    if (alarmBuzz==1) {
      for (t=0; t<DISPLAY_BUFFER_SIZE; t++) q.pixArray[t]+=BLINK;
      s.pattern+=BLINK;
    }
    m = q;
  }
  rmdn.delay(500);
}
           
void menuDateTime()
{
  setAlarm(alarmLevel, alarmHour, alarmMin);
  if (changed) {
    setDateTime(year+2000, month, day, hour, min, 30);
    changed = 0;
  }
  switch(clock_menu) {
    case 9:
    case 1:
      clock_menu = 1;
      m.printStr("  al", MIDRED+BLINK, MIDORANGE+BLINK);
      s.print(alarmLevel); s.pattern+=BLINK;
      break;
    case 2:
      m.printStr("  hr", MIDRED+BLINK, MIDORANGE+BLINK);
      s.print(alarmHour); s.pattern+=BLINK;
      break;
    case 3:
      m.printStr("  mi", MIDRED+BLINK, MIDORANGE+BLINK);
      s.print(alarmMin); s.pattern+=BLINK;
      break;
    case 4:
      m.printStr("  hr", MIDRED, GREEN);
      s.print(hour);
      break;
    case 5:
      m.printStr("  mi", MIDRED, GREEN);
      s.print(min);
      break;
    case 6:
      m.printStr("  yr", MIDORANGE, GREEN);
      s.print(year);
      break;
    case 7:
      m.printStr("  mo", MIDORANGE, GREEN);
      s.print(month);
      break;
    case 8:
      m.printStr("  dy", MIDORANGE, GREEN);
      s.print(day);
      break;
    default:
      clock_menu = 0;
  }  
}

void setAlarm(uint8_t level, uint8_t hour, uint8_t min)
{
  rtc.writenvram(0, level);
  rtc.writenvram(1, hour);
  rtc.writenvram(2, min);
}

void setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour,
           uint8_t min, uint8_t sec)
{
  if (month==2 && day>28) {
    if (year&0x03==0) day=29; else day=28;  // leap year
  }
  else
    if (day==31 && !(month==1 || month==3 || month==5 || month==7 || month==8 ||
      month==10 || month==12)) day=30; 
  DateTime c(year, month, day, hour, min, sec);
  rtc.adjust(c);
}

void upDateTime()
{
  switch(clock_menu) {
    case 1:
      if (alarmLevel < 3) alarmLevel++; else alarmLevel = 0;
      s.print(alarmLevel); s.pattern+=BLINK;
      break;
    case 2:
      if (alarmHour < 23) alarmHour++; else alarmHour = 0;
      s.print(alarmHour); s.pattern+=BLINK;
      break;
    case 3:
      if (alarmMin < 59) alarmMin++; else alarmMin = 0;
      s.print(alarmMin); s.pattern+=BLINK;
      break;
    case 4:
      if (hour >=0 && hour < 23) hour++; else hour = 0;
      s.print(hour); changed = 1;
      break;
    case 5:
      if (min >=0 && min < 59) min++; else min = 0;
      s.print(min); changed = 1;
      break;
    case 6:
      if (year >=0 && year < 99) year++; else year = 0;
      s.print(year); changed = 1;
      break;
    case 7:
      if (month >=1 && month < 12) month++; else month = 1;
      s.print(month); changed = 1;
      break;
    case 8:
      if (day >=1 && day < 31) day++; else day = 1;
      s.print(day); changed = 1;
      break;
  }  
}

void downDateTime()
{
  switch(clock_menu) {
    case 1:
      if (alarmLevel > 0) alarmLevel--; else alarmLevel = 3;
      s.print(alarmLevel); s.pattern+=BLINK;
      break;
    case 2:
      if (alarmHour > 0) alarmHour--; else alarmHour = 23;
      s.print(alarmHour); s.pattern+=BLINK;
      break;
    case 3:
      if (alarmMin > 0) alarmMin--; else alarmMin = 59;
      s.print(alarmMin); s.pattern+=BLINK;
      break;
    case 4:
      if (hour > 0 && hour <=23) hour--; else hour = 23;
      s.print(hour); changed = 1;
      break;
    case 5:
      if (min > 0 && min <= 59) min--; else min = 59;
      s.print(min); changed = 1;
      break;
    case 6:
      if (year > 0 && year <= 99) year--; else year = 99;
      s.print(year); changed = 1;
      break;
    case 7:
      if (month > 1 && month <= 12) month--; else month = 12;
      s.print(month); changed = 1;
      break;
    case 8:
      if (day > 1 && day <= 31) day--; else day = 31;
      s.print(day); changed = 1;
      break;
  }  
}

/*
Scrolls a clock across the LED matrix.
*/
#define NUM_WIDTH 3
const unsigned char clockSet1[] PROGMEM = {
      B01111111,
      B01000001,
      B01111111 ,  // Digit 0
      B00000000,
      B01111111,
      B00000000 ,  // Digit 1
      B01111001,
      B01001001,
      B01001111 ,  // Digit 2
      B01001001,
      B01001001,
      B01111111 ,  // Digit 3
      B00001111,
      B00001000,
      B01111111 ,  // Digit 4
      B01001111,
      B01001001,
      B01111001 ,  // Digit 5
      B01111111,
      B01001001,
      B01111001 ,  // Digit 6
      B00000001,
      B00000001,
      B01111111 ,  // Digit 7
      B01111111,
      B01001001,
      B01111111 ,  // Digit 8
      B01001111,
      B01001001,
      B01111111 ,  // Digit 9
      B00000000,
      B00000000,
      B01111111 ,  // Digit 10, for the hour part
      B00000000,
      B00000000,
      B00000000    // Space ' ' character
};
const unsigned char clockSet2[] PROGMEM = {
      B11111110,
      B10000010,
      B11111110 ,  // Digit 0
      B00000000,
      B11111110,
      B00000000 ,  // Digit 1
      B11110010,
      B10010010,
      B10011110 ,  // Digit 2
      B10010010,
      B10010010,
      B11111110 ,  // Digit 3
      B00011110,
      B00010000,
      B11111110 ,  // Digit 4
      B10011110,
      B10010010,
      B11110010 ,  // Digit 5
      B11111110,
      B10010010,
      B11110010 ,  // Digit 6
      B00000010,
      B00000010,
      B11111110 ,  // Digit 7
      B11111110,
      B10010010,
      B11111110 ,  // Digit 8
      B10011110,
      B10010010,
      B11111110 ,  // Digit 9
      B00000000,
      B00000000,
      B11111110 ,  // Digit 10, for the hour part
      B00000000,
      B00000000,
      B00000000    // Space ' ' character
};

void scrollClock(LedMatrix &m, byte hr, byte mn, byte color1, byte color2)
{
  byte pattern, clock[4], j, col;

  if ( hr > 12 ) hr = hr - 12;  // Converts to 12-hr time
  if ( hr < 10 ) {
    clock[0] = 11;              // Space ' ' character
    clock[1] = hr;
  }
  else {
    clock[0] = 10;
    clock[1] = hr - 10;
  }  
  clock[2] = mn/10;
  clock[3] = mn%10;

  // For each character, scroll left one column at a time and fill in with the
  // next column in the character's display pattern.
  for ( col = 0; col < NUM_WIDTH; col++ ) {
    pattern = pgm_read_byte(clockSet1+NUM_WIDTH*clock[0]+col);
    m.shiftLeft();
    for ( j = 0; j < MAX_ROW; j++ ) {
      if ( (pattern&0x01) == 0x01 )
        m.pixArray[(DISPLAY_BUFFER_SIZE-MAX_ROW)+j] = color1;
      pattern >>= 1;
    }
  }
  for ( col = 0; col < NUM_WIDTH; col++ ) {
    pattern = pgm_read_byte(clockSet1+NUM_WIDTH*clock[1]+col);
    m.shiftLeft();
    for ( j = 0; j < MAX_ROW; j++ ) {
      if ( (pattern&0x01) == 0x01 )
        m.pixArray[(DISPLAY_BUFFER_SIZE-MAX_ROW)+j] = color2;
      pattern >>= 1;
    }
  }
  for ( col = 0; col < NUM_WIDTH; col++ ) {
    pattern = pgm_read_byte(clockSet2+NUM_WIDTH*clock[2]+col);
    m.shiftLeft();
    for ( j = 0; j < MAX_ROW; j++ ) {
      if ( (pattern&0x01) == 0x01 )
        m.pixArray[(DISPLAY_BUFFER_SIZE-MAX_ROW)+j] = color1;
      pattern >>= 1;
    }
  }
  for ( col = 0; col < NUM_WIDTH; col++ ) {
    pattern = pgm_read_byte(clockSet2+NUM_WIDTH*clock[3]+col);
    m.shiftLeft();
    for ( j = 0; j < MAX_ROW; j++ ) {
      if ( (pattern&0x01) == 0x01 )
        m.pixArray[(DISPLAY_BUFFER_SIZE-MAX_ROW)+j] = color2;
      pattern >>= 1;
    }
  }
}

#define DELAY  250
#define COUNT_DELAY 2

#define STAY     0
#define UP       1
#define DOWN     2
#define LEFT     3
#define RIGHT    4
#define GAMEOVER 5
int gameover[] = { C_4, N_4, G_3, N_8, G_3, N_8, A_3, N_4, G_3, N_4, Rest, N_4, B_3, N_4, C_4, N_4, Rest, N_2, End } ;

int direction = GAMEOVER;
int ballx, bally, ballcolor, addx, addy, playerx, count = 0, score = 0;

int color [] = { RED, YELLOW+BLINK, ORANGE, RED+BLINK, YELLOW,
  ORANGE+BLINK, RED, YELLOW+BLINK, ORANGE, RED+BLINK } ;

void pong_setup()
{
  s.print(0);
}

void pong_loop()
{
  rmdn.checkButtonsPress();
  if ( rmdn.buttonA || rmdn.buttonB ) {   // New game
    m.clearMatrix();
    score = 0; s.print(score);
    playerx = 5;
    m.setPixel( playerx, MAX_ROW-1, GREEN );
    ballx = random( 0, MAX_COLUMN ); bally = 0;
    ballcolor = color[ballx];
    m.setPixel( ballx, 0, ballcolor );
    addx = ( random(0, 2 ) ? -1 : 1 );
    addy = 1;
    direction = STAY;
  }
  else if ( direction != GAMEOVER ) pong_keyaction();
  rmdn.delay(DELAY);
}

void pong_keyaction()
{
  if ( rmdn.buttonLeft ) direction = LEFT;
  if ( rmdn.buttonRight ) direction = RIGHT;
  if ( direction == LEFT ) {
    if ( playerx > 0 ) {
      m.setPixel( playerx--, MAX_ROW-1, CLEAR );
      m.setPixel( playerx, MAX_ROW-1, GREEN );
    }
  }
  if ( direction == RIGHT ) {
    if ( playerx < MAX_COLUMN-1 ) {
      m.setPixel( playerx++, MAX_ROW-1, CLEAR );
      m.setPixel( playerx, MAX_ROW-1, GREEN );
    }
  }
  if ( ++count == COUNT_DELAY ) {
    count = 0;
    m.setPixel( ballx, bally, CLEAR );
    ballx += addx;
    bally += addy;
    if ( ballx < 0 ) {
      ballx = 1;
      addx = 1;
    }
    if ( ballx == MAX_COLUMN ) {
      ballx = MAX_COLUMN - 2;
      addx = -1;
    }
    if ( bally < 0 ) {
      bally = 1;
      addy = 1;
    }
    if ( bally == MAX_ROW - 1 ) {
      if ( ballx == playerx ) {
        rmdn.tone(B_3, DELAY);
        if ( ++score >= 100 ) score = 0;
        s.print(score);
        bally = MAX_ROW - 2;
        addy = -1;
        if ( ballx == 0 ) {
          ballx = 1;
          addx = 1;
        }
        else if ( ballx == MAX_COLUMN - 1 ) {
          ballx = MAX_COLUMN - 2;
          addx = -1;
          } else ballx += addx;
      }
      else direction = GAMEOVER;
    }
    m.setPixel(ballx, bally, ballcolor);
    if ( direction != GAMEOVER )
      direction = STAY;
    else
      rmdn.playMelody(gameover, ONCE);
  }
}

#define SNAKE_MAX_LENGTH  20

byte *a;
int xy[SNAKE_MAX_LENGTH], tail;
int currentX, currentY;

void snake_setup()
{
  a = m.pixArray;
}

void snake_loop()
{
  rmdn.checkButtonsPress();
  if ( rmdn.buttonA || rmdn.buttonB ) {   // New game
    m.clearMatrix();
    score = 0; s.print(score);
    currentX = 2; currentY = 3;
    xy[0] = currentX * MAX_ROW + currentY; a[xy[0]] = GREEN;
    xy[1] = xy[0] - MAX_ROW; a[xy[1]] = MIDGREEN;
    xy[2] = xy[1] - MAX_ROW; a[xy[2]] = LIGHTGREEN;
    tail = 2;
    a[ random(xy[0] + 1, DISPLAY_BUFFER_SIZE) ] = RED+BLINK;
    direction = STAY;
  }
  else if ( direction != GAMEOVER ) snake_keyaction();
  rmdn.delay(DELAY);
}

void snake_keyaction()
{
  int j;
  
  if ( rmdn.buttonUp ) direction = UP;
  if ( rmdn.buttonDown ) direction = DOWN;
  if ( rmdn.buttonLeft ) direction = LEFT;
  if ( rmdn.buttonRight ) direction = RIGHT;
  switch (direction) {
    case UP:
      currentY--;
      if (currentY < 0) currentY=MAX_ROW-1;
      break;
    case DOWN:
      currentY++;
      if (currentY >= MAX_ROW) currentY=0;
      break;
    case LEFT:
      currentX--;
      if (currentX < 0) currentX=MAX_COLUMN-1;
      break;
    case RIGHT:
      currentX++;
      if (currentX >= MAX_COLUMN) currentX=0;
      break;
  }

  if ( direction != STAY ) {
    a[xy[tail]] = CLEAR;
    for (i=tail, j=tail/3+1; i>0; i--) {  // move snake
      xy[i] = xy[i-1];
      a[xy[i]] = (i>j? LIGHTGREEN: MIDGREEN);
    } 
    xy[0] = currentX * MAX_ROW + currentY;
    if ( a[xy[0]] == RED+BLINK || a[xy[0]] == ORANGE+BLINK ||
      a[xy[0]] == YELLOW+BLINK ) {  // caught food!
      rmdn.tone(B_3, DELAY);
      if ( ++score >= 100 ) score = 0;
      s.print(score);
      do {                          // place new food
        i = random(0, DISPLAY_BUFFER_SIZE);
      } while ( a[i] != CLEAR );
      j = i&0x03; a[i] = ( j==3? YELLOW+BLINK: (j==2? ORANGE+BLINK: RED+BLINK) );
      if ( tail < SNAKE_MAX_LENGTH && score%3==0) {  // grow the snake
        xy[tail+1] = xy[tail];        
        tail++; a[xy[tail]] = LIGHTGREEN;  
      }
    }
    else 
      if ( a[xy[0]] == LIGHTGREEN || a[xy[0]] == MIDGREEN ||
        a[xy[0]] == GREEN ) {  // bite itself!
        rmdn.playMelody(gameover, ONCE);
        s.pattern+=BLINK;
        direction = GAMEOVER;
      }
    a[xy[0]] = GREEN;
  }
}

// Mario main theme melody
const int mario_theme[] = {
  E_7, N_12, E_7, N_12, Rest, N_12, E_7, N_12,
  Rest, N_12, C_7, N_12, E_7, N_12, Rest, N_12,
  G_7, N_12, Rest, N_12, Rest, N_12, Rest, N_12,
  G_6, N_12, Rest, N_12, Rest, N_12, Rest, N_12,

  C_7, N_12, Rest, N_12, Rest, N_12, G_6, N_12,
  Rest, N_12, Rest, N_12, E_6, N_12, Rest, N_12,
  Rest, N_12, A_6, N_12, Rest, N_12, B_6, N_12,
  Rest, N_12, As_6, N_12, A_6, N_12, Rest, N_12,

  G_6, N_9, E_7, N_9, G_7, N_9,
  A_7, N_12, Rest, N_12, F_7, N_12, G_7, N_12,
  Rest, N_12, E_7, N_12, Rest, N_12, C_7, N_12,
  D_7, N_12, B_6, N_12, Rest, N_12, Rest, N_12,

  C_7, N_12, Rest, N_12, Rest, N_12, G_6, N_12,
  Rest, N_12, Rest, N_12, E_6, N_12, Rest, N_12,
  Rest, N_12, A_6, N_12, Rest, N_12, B_6, N_12,
  Rest, N_12, As_6, N_12, A_6, N_12, Rest, N_12,

  G_6, N_12, E_7, N_12, G_7, N_12,
  A_7, N_12, Rest, N_12, F_7, N_12, G_7, N_12,
  Rest, N_12, E_7, N_12, Rest, N_12, C_7, N_12,
  D_7, N_12, B_6, N_12, Rest, N_12, Rest, N_12,
  End 
};

void test_setup()
{
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 0, RED);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 1, ORANGE);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 2, GREEN);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 3, YELLOW);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 4, RED);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 5, ORANGE);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 6, GREEN);
  for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 7, YELLOW);
  s.pattern = 0x7FFF;
}

void test_loop()
{
  int i;
  
  rmdn.checkButtonsPress();
  if ( rmdn.buttonA ) {
    rmdn.playMelody(mario_theme, ONCE);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 0, RED);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 1, ORANGE);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 2, GREEN);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 3, YELLOW);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 4, RED);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 5, ORANGE);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 6, GREEN);
    for ( i=0; i<MAX_COLUMN; i++) m.setPixel(i, 7, YELLOW);
    s.pattern = 0x7FFF;
  }
  if ( rmdn.buttonB ) {
    if ( rmdn.getBrightLevel() == LOW_BRIGHTLEVEL )
      rmdn.setBrightLevel(HIGH_BRIGHTLEVEL);
    else rmdn.setBrightLevel(LOW_BRIGHTLEVEL);
  }
  if ( rmdn.buttonLeft ) {
    s.pattern = 0;
    m.setScrollInterval(200);
    m.printStr(" ABCDEFGHIJKLMNOPQRSTUVWXYZ     ", RED, YELLOW); rmdn.delay(DELAY);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(0, i, RED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(1, i, RED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(2, i, RED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(3, i, MIDRED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(4, i, MIDRED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(5, i, MIDRED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(6, i, MIDRED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(7, i, LIGHTRED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(8, i, LIGHTRED);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(9, i, LIGHTRED);
    s.pattern = 0x7F7F;
  }    
  if ( rmdn.buttonDown ) {
    s.pattern = 0;
    m.setScrollInterval(200);
    m.printStr(" abcdefghijklmnopqrstuvwxyz     ", ORANGE, GREEN); rmdn.delay(DELAY);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(0, i, ORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(1, i, ORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(2, i, ORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(3, i, MIDORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(4, i, MIDORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(5, i, MIDORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(6, i, MIDORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(7, i, LIGHTORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(8, i, LIGHTORANGE);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(9, i, LIGHTORANGE);
    s.pattern = 0x7F7F;
  }
  if ( rmdn.buttonUp ) {
    s.pattern = 0;
    m.setScrollInterval(200);
    m.printStr(" 0123456789 !#$%&(*)+,-./ :;<=>?@ [\\]^_{|}~  ", RED, ORANGE); rmdn.delay(DELAY);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(0, i, GREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(1, i, GREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(2, i, GREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(3, i, MIDGREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(4, i, MIDGREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(5, i, MIDGREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(6, i, MIDGREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(7, i, LIGHTGREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(8, i, LIGHTGREEN);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(9, i, LIGHTGREEN);
    s.pattern = 0x7F7F;
  }
  if ( rmdn.buttonRight ) {
    s.pattern = 0;
    m.setScrollInterval(120); // Set scroll interval back to the default of 120ms
    m.printStr(" The quick brown fox jumps over the lazy dog.     ", GREEN, YELLOW); rmdn.delay(DELAY);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(0, i, YELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(1, i, YELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(2, i, YELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(3, i, MIDYELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(4, i, MIDYELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(5, i, MIDYELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(6, i, MIDYELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(7, i, LIGHTYELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(8, i, LIGHTYELLOW);
    for ( i=0; i<MAX_ROW; i++) m.setPixel(9, i, LIGHTYELLOW);
    s.pattern = 0x7F7F;
  }
  rmdn.delay(DELAY);
}

