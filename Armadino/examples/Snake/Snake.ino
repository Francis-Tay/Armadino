#include <Armadino.h>

#define DELAY            250
#define SNAKE_MAX_LENGTH  32
#define STAY               0
#define UP                 1
#define DOWN               2
#define LEFT               3
#define RIGHT              4
#define GAMEOVER           5

Armadino rmdn;
LedMatrix m;
Led7Seg s;
byte *a;
int i, direction = GAMEOVER, xy[SNAKE_MAX_LENGTH], tail;
int currentX, currentY, score;

int gameover [] = { C_4, N_4 , G_3, N_8 , G_3, N_8, A_3, N_4, G_3, N_4, Rest, N_4, B_3, N_4, C_4, N_4, Rest, N_2, End } ;

void setup()
{
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
  m.printStr("no     ", GREEN, ORANGE);
  a = m.pixArray;
  randomSeed(analogRead(1));
}

void loop()
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
  else if ( direction != GAMEOVER ) keyaction();
  rmdn.delay(DELAY);
}

void keyaction()
{
  int i, j;
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
