// Attribution Non-commercial Share Alike (by-nc-sa)
// This license lets others remix, tweak, and build upon your work
// non-commercially, as long as they credit you and license their
// new creations under the identical terms.Others can download and
// redistribute your work just like the by-nc-nd license, but they
// can also translate,make remixes, and produce new stories based
// on your work. All new work based on yours will carry the same license,
// so any derivatives will also be non-commercial in nature.
// --------------------------------------------------
// | original author: Pete Lamonica                 |
// | atari input mod: kyle brinkerhoff              |
// | armadino input mod: francis tay                |
// --------------------------------------------------
//
#include <TVout.h>
#include <fontALL.h>
#include <Armadino.h>

#define PADDLE_HEIGHT 12
#define PADDLE_WIDTH 2

#define RIGHT_PADDLE_X (TV.hres()-4)
#define LEFT_PADDLE_X 2

#define IN_GAME 0   // in game state
#define IN_MENU 1   // in menu state
#define GAME_OVER 2 // game over state

#define LEFT_SCORE_X (TV.hres()/2-15)
#define RIGHT_SCORE_X (TV.hres()/2+10)
#define SCORE_Y 4

#define MAX_Y_VELOCITY 3
#define PLAY_TO 7

#define LEFT 0
#define RIGHT 1

Armadino rmdn;
LedMatrix m;
Led7Seg s;
const uint8_t icon[] PROGMEM = ARMADINO_ICON;
TVout TV;

unsigned char x,y;
int wheelOnePosition = 0;
int wheelTwoPosition = 0;
int rightPaddleY = 0;
int leftPaddleY = 0;
unsigned char ballX = 0;
unsigned char ballY = 0;
char ballVolX = 1;
char ballVolY = 1;
int initialpos=500;
int leftPlayerScore = 0;
int rightPlayerScore = 0;
int frame = 0;
int state = IN_MENU;

void processInputs() {
  rmdn.checkButtonsDown();
  if(rmdn.buttonDown)
    initialpos=initialpos+10;
  if(rmdn.buttonUp)
    initialpos=initialpos-10;
  if (initialpos>=1000)
    initialpos=999;
  if (initialpos<=0)
    initialpos=1;
  wheelOnePosition =initialpos;
  wheelTwoPosition =initialpos                                                                                                                                                                                                                                                                                                                ;
}

void drawGameScreen()
{
  TV.clear_screen();
  // draw right paddle
  rightPaddleY = ((wheelOnePosition / 8) * (TV.vres()-PADDLE_HEIGHT))/ 128;
  x = RIGHT_PADDLE_X;
  for (int i=0; i<PADDLE_WIDTH; i++) {
    TV.draw_line(x+i,rightPaddleY,x+i,rightPaddleY+PADDLE_HEIGHT,1);
  }

  // draw left paddle
  leftPaddleY = ((wheelTwoPosition / 8) * (TV.vres()-PADDLE_HEIGHT))/ 128;
  x = LEFT_PADDLE_X;
  for (int i=0; i<PADDLE_WIDTH; i++) {
    TV.draw_line(x+i,leftPaddleY,x+i,leftPaddleY+PADDLE_HEIGHT,1);
  }

  // draw score
  TV.print_char(LEFT_SCORE_X,SCORE_Y,'0'+leftPlayerScore);
  TV.print_char(RIGHT_SCORE_X,SCORE_Y,'0'+rightPlayerScore);
  s.print(leftPlayerScore*10+rightPlayerScore);

  // draw net
  for(int i=1; i<TV.vres() - 4; i+=6) {
    TV.draw_line(TV.hres()/2,i,TV.hres()/2,i+3, 1);
  }

  // draw ball
  TV.set_pixel(ballX, ballY, 2);
}

// player == LEFT or RIGHT
void playerScored(byte player)
{
  if (player == LEFT) leftPlayerScore++;
  if (player == RIGHT) rightPlayerScore++;

  // check for win
  if (leftPlayerScore == PLAY_TO || rightPlayerScore == PLAY_TO) {
    state = GAME_OVER;
  }
  ballVolX = -ballVolX;
}

void drawMenu() {
  x = 0;
  y = 0;
  char volX = 1;
  char volY = 1;
  TV.clear_screen();
  TV.select_font(font8x8);
  TV.print(10, 20, "Armadino Pong");
  TV.select_font(font6x8);
  TV.print(10, 50, "Press A to start");
  while (!rmdn.buttonA) {
    processInputs();
    TV.delay_frame(3);
    if (x + volX < 1 || x + volX > TV.hres() - 1) volX = -volX;
    if (y + volY < 1 || y + volY > TV.vres() - 1) volY = -volY;
    if (TV.get_pixel(x + volX, y + volY)) {
      TV.set_pixel(x + volX, y + volY, 0);
      if (TV.get_pixel(x + volX, y - volY) == 0) volY = -volY;
        else if (TV.get_pixel(x - volX, y + volY) == 0) volX = -volX;
          else {
            volX = -volX;
            volY = -volY;
          }
    }
    TV.set_pixel(x, y, 0);
    x += volX;
    y += volY;
    TV.set_pixel(x, y, 1);
  }
  TV.select_font(font6x8);
  state = IN_GAME;
}

void setup() {
//  Serial.begin(9600);
  x=0;
  y=0;
  rmdn.begin(); TV.begin(NTSC, 120, 90); TV.set_hbi_hook(Armadino::ledRefresh);
  rmdn.show(m);
  rmdn.show(s);
  m.printStr(" Ar", RED, YELLOW);
  m.printStr("ma", GREEN, ORANGE);
  m.printStr("di", RED, YELLOW);
  m.printStr("no     ", GREEN, ORANGE);
  m.loadBitmap(icon);
  ballX = TV.hres() / 2;
  ballY = TV.vres() / 2;
}

void loop() {
  processInputs();
  if (state == IN_MENU)
    drawMenu();
  if (state == IN_GAME) {
    if (frame % 3 == 0) { // every third frame
      ballX += ballVolX;
      ballY += ballVolY;
      if (ballY <= 1 || ballY >= TV.vres()-1) ballVolY = -ballVolY;
      if (ballVolX < 0 && ballX == LEFT_PADDLE_X+PADDLE_WIDTH-1 &&
        ballY >= leftPaddleY && ballY <= leftPaddleY + PADDLE_HEIGHT) {
        ballVolX = -ballVolX;
        ballVolY += 2 * ((ballY - leftPaddleY) -
          (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);
      }
      if (ballVolX > 0 && ballX == RIGHT_PADDLE_X && ballY >= rightPaddleY &&
        ballY <= rightPaddleY + PADDLE_HEIGHT) {
        ballVolX = -ballVolX;
        ballVolY += 2 * ((ballY - rightPaddleY) -
          (PADDLE_HEIGHT / 2)) / (PADDLE_HEIGHT / 2);
      }
      // limit vertical speed
      if (ballVolY > MAX_Y_VELOCITY) ballVolY = MAX_Y_VELOCITY;
      if (ballVolY < -MAX_Y_VELOCITY) ballVolY = -MAX_Y_VELOCITY;
      if (ballX <= 1) playerScored(RIGHT);
      if (ballX >= TV.hres() - 1) playerScored(LEFT);
    }
//    if (rmdn.buttonA) Serial.println((int)ballVolX);
    drawGameScreen();
  }
  if (state == GAME_OVER) {
    drawGameScreen();
    TV.select_font(font8x8);
    TV.print(25,25,"GAME");
    TV.print(65,25,"OVER");
    while (!rmdn.buttonA) {
      processInputs();
      TV.delay(50);
    }
    TV.select_font(font6x8); // reset the font
    // reset the scores
    leftPlayerScore = 0;
    rightPlayerScore = 0;
    state = IN_MENU;
  }
  TV.delay_frame(1);
  if (++frame == 60) frame = 0; // increment and/or reset frame counter
}
