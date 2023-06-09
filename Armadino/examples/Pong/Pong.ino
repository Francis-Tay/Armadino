#include <Armadino.h>

#define DELAY      300
#define BALL_SPEED 2

#define STAY     0
#define UP       1
#define DOWN     2
#define LEFT     3
#define RIGHT    4
#define GAMEOVER 5

Armadino rmdn;
LedMatrix m;
Led7Seg s;
int direction = GAMEOVER;
int i, ballx, bally, ballcolor, addx, addy, playerx, count = 0, score = 0;

int gameover [] = { C_4, N_4, G_3, N_8, G_3, N_8, A_3, N_4, G_3, N_4, Rest, N_4, B_3, N_4, C_4, N_4, End } ;
int color [] = { RED, YELLOW+BLINK, ORANGE, RED+BLINK, YELLOW,
  ORANGE+BLINK, RED, YELLOW+BLINK, ORANGE, RED+BLINK } ;

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
  randomSeed(analogRead(2));
}

void loop()
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
  else if ( direction != GAMEOVER ) keyaction();
  rmdn.delay(DELAY);
}

void keyaction()
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
  if ( ++count >= BALL_SPEED ) {
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

