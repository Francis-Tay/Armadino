// Clock using a DS1307 RTC, with LM36 temperature sensor and LDR

#include <Armadino.h>
#include "RTClib.h"
void scrollClock(LedMatrix &m, byte hr, byte mn, byte color1, byte color2);
void menuDateTime(), upDateTime(), downDateTime();
void setDateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0,
    uint8_t min = 0, uint8_t sec = 0);

Armadino rmdn;
LedMatrix m, q;
Led7Seg s;
RTC_DS1307 rtc;
DateTime now;
uint16_t i, t, b, l;
uint8_t hour, min, sec, day, month, year, menu = 0, changed = 0,
  alarmLevel, alarmHour, alarmMin, alarmBuzz;
int alarmMelody[] = { C_6, N_16, C_6, N_16, C_6, N_16, C_6, N_16, Rest, N_4, End };

#define TEMPERATURE_PIN 3
#define LDR_PIN 2

const char string0[] PROGMEM = "To thine own self be true.";
const char string1[] PROGMEM = "All the world's a stage.";
const char string2[] PROGMEM = "Brevity is the soul of wit.";
const char string3[] PROGMEM = "Action is eloquence.";
const char string4[] PROGMEM = "There is a tide in the affairs of men.";
const char string5[] PROGMEM = "Love all, trust a few, do wrong to none.";
const char string6[] PROGMEM = "Life is of a mingled yarn, good and ill together.";
const char string7[] PROGMEM = "The better part of valor is discretion.";
const char string8[] PROGMEM = "Some rise by sin, and some by virtue fall.";
const char string9[] PROGMEM = "Our doubts are traitors.";

/*
const char string0[] PROGMEM = "Love is patient, love is kind.";
const char string1[] PROGMEM = "Love does not envy, it does not boast, it is not proud.";
const char string2[] PROGMEM = "Love is not rude, it is not self-seeking.";
const char string3[] PROGMEM = "Love is not easily angered, it keeps no record of wrongs.";
const char string4[] PROGMEM = "Love does not delight in evil but rejoices with the truth.";
const char string5[] PROGMEM = "Love always protects, always trusts, always hopes, always perseveres.";
const char string6[] PROGMEM = "The Lord is my shepherd, I shall not be in want.";
const char string7[] PROGMEM = "The fear of the Lord is the beginning of wisdom.";
const char string8[] PROGMEM = "Your word is a lamp to my feet and a light for my path.";
const char string9[] PROGMEM = "Those who hope in the Lord will renew their strength.";
*/

#define TABLE_SIZE 10
const char * const stringTable[] PROGMEM = {
  string0, string1, string2, string3, string4,
  string5, string6, string7, string8, string9
};
char buffer[80];

void setup ()
{
  rmdn.begin();
  rmdn.show(m);
  rmdn.show(s);
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
  randomSeed(analogRead(1));
}

void loop ()
{
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
      if (menu > 0) {
        menu = 0;
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
      menu = 0;
      rmdn.delay(500);
      break;
    case BUTTON_B:  
      if (menu > 0) {
        menu = 0;
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
      menu++;
      menuDateTime();
      break;
    case BUTTON_LEFT:
      menu--;
      menuDateTime();
      break;
    case BUTTON_UP:
      if (menu > 0) upDateTime();
      break;
    case BUTTON_DOWN:
      if (menu > 0) downDateTime();
      break;
  }
  if (menu == 0) {
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
  switch(menu) {
    case 9:
    case 1:
      menu = 1;
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
      menu = 0;
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
  switch(menu) {
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
      if (month >=0 && month < 12) month++; else month = 1;
      s.print(month); changed = 1;
      break;
    case 8:
      if (day >=0 && day < 31) day++; else day = 1;
      s.print(day); changed = 1;
      break;
  }  
}

void downDateTime()
{
  switch(menu) {
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
