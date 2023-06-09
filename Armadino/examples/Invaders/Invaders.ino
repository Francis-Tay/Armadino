#include <Armadino.h>

#define DELAY            100
#define STAY               0
#define UP                 1
#define DOWN               2
#define LEFT               3
#define RIGHT              4
#define GAMEOVER           5

#define PLAYER_COLOUR           GREEN
#define SHIELD_COLOUR           LIGHTGREEN
#define PLAYER_MISSILE_COLOUR   YELLOW
#define INVADER_COLOUR          LIGHTRED
#define INVADER_MISSILE_COLOUR  LIGHTORANGE
#define SPACESHIP_COLOUR        MIDRED+BLINK

Armadino rmdn;
LedMatrix m;
Led7Seg s;
const uint8_t space[] PROGMEM = { 0, 0, 0, 0, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, SHIELD_COLOUR, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, 0, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, 0, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, 0, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, 0, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, SHIELD_COLOUR, 0, \
  0, INVADER_COLOUR, INVADER_COLOUR, INVADER_COLOUR, 0, 0, 0, 0, \
  0, 0, 0, 0, 0, 0, 0, 0 };
uint8_t direction = GAMEOVER;
int i, currentX, score, x, y, invaders;

const int jiggly[] = {
  D_5, N_4, A_5, N_8, Fs_5, N_8, D_5, N_8,
  E_5, N_4, Fs_5, N_8, G_5, N_4,
  Fs_5, N_4, E_5, N_8, Fs_5, N_4,
  D_5, N_2,
  D_5, N_4, A_5, N_8, Fs_5, N_8, D_5, N_8,
  E_5, N_4, Fs_5, N_8, G_5, N_4,
  Fs_5, N_1,
  D_5, N_4, A_5, N_8, Fs_5, N_8, D_5, N_8,
  E_5, N_4, Fs_5, N_8, G_5, N_4,
  Fs_5, N_4, E_5, N_8, Fs_5, N_4,
  D_5, N_2,
  D_5, N_4, A_5, N_8, Fs_5, N_8, D_5, N_8,
  E_5, N_4, Fs_5, N_8, G_5, N_4,
  Fs_5, N_1, End
};
const int gameover[] = {      // Game over song
  C_4, N_8, Rest, N_8, Rest, N_8, G_3, N_8, Rest, N_4, E_3, N_4, A_3, N_6, B_3, N_6, A_3, N_6,
  Gs_3, N_6, As_3, N_6, Gs_3, N_6, G_3, N_8, F_3, N_8, G_3, N_4, End };

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
}

void loop()
{
  static uint8_t i = 0;
  
  rmdn.checkButtonsPress();
  if ( rmdn.buttonB ) {   // New game
    randomSeed(analogRead(1));
    m.loadBitmap(space);
    score = 0; s.print(score);
//    rmdn.playMelody(jiggly, REPEAT);
    currentX = 2;
    invaders = 21;
    direction = STAY;
    i = 0;
  }
  else
    if ( direction != GAMEOVER ) {
      player();   // player moves left, right or fire missiles
      switch (i++) {
        case 0: squadron(); break;   // squadron moves left, right and fire missiles
        case 1: spaceship(); break;  // move and launch spaceship
        case 2: missiles(); break;   // move missiles, detect collisions and score points
        case 3: i=0;
      }
      if (invaders == 0) direction = GAMEOVER;
      s.print(score);
    }
  rmdn.delay(DELAY);
}

void player()
{
  if ( rmdn.buttonLeft ) direction = LEFT;
  if ( rmdn.buttonRight ) direction = RIGHT;
  m.setPixel(currentX, 7, 0);
  switch (direction) {
    case LEFT:
      currentX--;
      if (currentX < 0) currentX=0;
      break;
    case RIGHT:
      currentX++;
      if (currentX > 9) currentX=9;
      break;
  }
  m.setPixel(currentX, 7, PLAYER_COLOUR);
  direction = STAY;  
  if ( rmdn.buttonA )  // player fires missile
    if ( m.getPixel(currentX, 6) != SHIELD_COLOUR )
      if ( m.getPixel(currentX, 6) == INVADER_MISSILE_COLOUR )
        m.setPixel(currentX, 6, 0);
      else
        m.setPixel(currentX, 6, PLAYER_MISSILE_COLOUR);      
}

void squadron()
{
  static uint8_t movement = LEFT;
  
  if ( movement == LEFT )
    if (m.getPixel(0, 1) || m.getPixel(0, 2) || m.getPixel(0, 3))
      movement = RIGHT;
    else
      for (y=1; y<=3; y++) {
        for (x=0; x<9; x++)
          if (m.getPixel(x, y)==PLAYER_MISSILE_COLOUR) {
            if (m.getPixel(x+1, y)==INVADER_COLOUR) {
              m.setPixel(x, y, 0);
              rmdn.beep();
              score++; invaders--;
            }
          }  
          else
            if (m.getPixel(x, y)==INVADER_MISSILE_COLOUR)
              {}  // do nothing
            else
              if (m.getPixel(x+1, y)==PLAYER_MISSILE_COLOUR ||
                  m.getPixel(x+1, y)==INVADER_MISSILE_COLOUR)
                m.setPixel(x, y, 0);
              else
                m.setPixel(x, y, m.getPixel(x+1, y));
        if (m.getPixel(9, y)!=PLAYER_MISSILE_COLOUR &&
            m.getPixel(9, y)!=INVADER_MISSILE_COLOUR)
          m.setPixel(9, y, 0);
      }
  else if ( movement == RIGHT )
      if (m.getPixel(9, 1) || m.getPixel(9, 2) || m.getPixel(9, 3))
        movement = LEFT;
      else
        for (y=1; y<=3; y++) {
          for (x=9; x>0; x--)
            if (m.getPixel(x, y)==PLAYER_MISSILE_COLOUR) {
              if (m.getPixel(x-1, y)==INVADER_COLOUR) {
                m.setPixel(x, y, 0);
                rmdn.beep();
                score++; invaders--;
              }
            }  
            else
              if (m.getPixel(x, y)==INVADER_MISSILE_COLOUR)
                {}  // do nothing
              else
                if (m.getPixel(x-1, y)==PLAYER_MISSILE_COLOUR ||
                    m.getPixel(x-1, y)==INVADER_MISSILE_COLOUR)
                  m.setPixel(x, y, 0);
                else
                  m.setPixel(x, y, m.getPixel(x-1, y));
          if (m.getPixel(0, y)!=PLAYER_MISSILE_COLOUR &&
              m.getPixel(0, y)!=INVADER_MISSILE_COLOUR)
            m.setPixel(0, y, 0);
        }

  for (x=0; x<=9; x++)  // invaders fire missiles at random
    for (y=3; y>=1; y--)
      if (m.getPixel(x, y)==INVADER_COLOUR) {
        if ( m.getPixel(x, y+1)!=PLAYER_MISSILE_COLOUR && random(20)==0 )
          m.setPixel(x, y+1, INVADER_MISSILE_COLOUR);
        break;
      } 
}

void spaceship()
{
  for (x=0; x<9; x++)
    if (m.getPixel(x, 0)==PLAYER_MISSILE_COLOUR) {
      if (m.getPixel(x+1, 0)==SPACESHIP_COLOUR) {
        rmdn.beep();
        score+=2;
      }
      m.setPixel(x, 0, 0);
    }  
    else
      if (m.getPixel(x+1, 0)==PLAYER_MISSILE_COLOUR)
        m.setPixel(x, 0, 0);
      else
        m.setPixel(x, 0, m.getPixel(x+1, 0));
  if (random(20)==0)
    m.setPixel(9, 0, SPACESHIP_COLOUR);
  else
    m.setPixel(9, 0, 0); 
}

void missiles()
{
  // move player's missiles
  for (x=0; x<=9; x++)
    if (m.getPixel(x, 0)==PLAYER_MISSILE_COLOUR)
      m.setPixel(x, 0, 0);        
  for (y=1; y<=6; y++)
    for (x=0; x<=9; x++)
      if (m.getPixel(x, y)==PLAYER_MISSILE_COLOUR) {
        switch (m.getPixel(x, y-1)) {
          case INVADER_MISSILE_COLOUR:
            m.setPixel(x, y-1, 0);
            break;
          case INVADER_COLOUR:
            m.setPixel(x, y-1, 0);
            rmdn.beep();
            score++; invaders--;
            break;
          case SPACESHIP_COLOUR:
            m.setPixel(x, y-1, 0);
            rmdn.beep();
            score+=2;
            break;
          default:
            m.setPixel(x, y-1, PLAYER_MISSILE_COLOUR);
        }
        m.setPixel(x, y, 0);  
      }

  // move invaders' missiles
  for (x=0; x<=9; x++)
    if (m.getPixel(x, 7)==INVADER_MISSILE_COLOUR)
      m.setPixel(x, 7, 0);        
  for (y=6; y>=2; y--)      
    for (x=0; x<=9; x++)
      if (m.getPixel(x, y)==INVADER_MISSILE_COLOUR) {
        switch (m.getPixel(x, y+1)) {
          case PLAYER_MISSILE_COLOUR:
            m.setPixel(x, y+1, 0);
            break;
          case SHIELD_COLOUR:
            break;
          case PLAYER_COLOUR:
            m.setPixel(x, y+1, INVADER_MISSILE_COLOUR);
            direction = GAMEOVER;
            rmdn.playMelody(gameover, ONCE);
            break;
          default:
            m.setPixel(x, y+1, INVADER_MISSILE_COLOUR);
        }
        m.setPixel(x, y, 0);  
      }
}
