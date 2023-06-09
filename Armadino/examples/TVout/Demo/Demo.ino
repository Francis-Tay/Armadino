//
// Remember to modify armadino_setup.h to define INCLUDE_TVOUT
//
#include <Armadino.h>
#include <TVout.h>
#include <fontALL.h>
#include "schematic.h"
#include "TVOlogo.h"

Armadino rmdn;
LedMatrix m;
Led7Seg s;
TVout TV;
const uint8_t icon[] PROGMEM = ARMADINO_ICON;
int melody [] = { C_4, N_4, C_4, N_4, G_4, N_4, G_4, N_4, A_4, N_4, A_4, N_4, G_4, N_4, Rest, N_1, End } ;

int zOff = 150;
int xOff = 0;
int yOff = 0;
int cSize = 50;
int view_plane = 64;
float angle = PI/60;

float cube3d[8][3] = {
  {xOff - cSize,yOff + cSize,zOff - cSize},
  {xOff + cSize,yOff + cSize,zOff - cSize},
  {xOff - cSize,yOff - cSize,zOff - cSize},
  {xOff + cSize,yOff - cSize,zOff - cSize},
  {xOff - cSize,yOff + cSize,zOff + cSize},
  {xOff + cSize,yOff + cSize,zOff + cSize},
  {xOff - cSize,yOff - cSize,zOff + cSize},
  {xOff + cSize,yOff - cSize,zOff + cSize}
};
unsigned char cube2d[8][2];

int tick = 0;
void tick_hook() {
  static int j = 0;
  if ( ++j > 1000 ) { if (++tick > 99) tick = 0; j = 0; }
}

void setup() {
  rmdn.begin();
  // If pin A2 is pulled LOW, then the PAL jumper is shorted.
  pinMode(A2, INPUT); digitalWrite(A2, HIGH);
  if (digitalRead(A2) == LOW) TV.begin(PAL,120,90); else TV.begin(NTSC,120,90);
  TV.set_hbi_hook(Armadino::ledRefresh);
  rmdn.setHook(tick_hook);
  rmdn.show(m);
  TV.select_font(font6x8);
  intro();
  m.printStr("Ar", RED, YELLOW);
  m.printStr("ma", GREEN, ORANGE);
  m.printStr("di", RED, YELLOW);
  m.printStr("no", GREEN, ORANGE);
  m.loadBitmap(icon);
  rmdn.playMelody(melody, REPEAT);
  TV.clear_screen();
  TV.println("I am the TVout\nlibrary running on Armadino");
  TV.delay(2500);
  TV.println("\nMy schematic:");
  TV.delay(1500);
  TV.bitmap(0,0,schematic);
  TV.delay(5000);
  TV.clear_screen();
  TV.println("What can I do?");
  TV.delay(2000);

  //fonts
  TV.println("\nMultiple fonts:");
  TV.select_font(font4x6);
  TV.println("4x6 Font");
  TV.select_font(font6x8);
  TV.println("6x8 Font");
  TV.select_font(font8x8);
  TV.println("8x8 Font");
  TV.select_font(font6x8);
  TV.delay(2000);

  TV.println("\nDraw shapes...");
  TV.delay(2000);
  //circles
  TV.clear_screen();
  TV.draw_circle(TV.hres()/2,TV.vres()/2,TV.vres()/3,WHITE);
  TV.delay(500);
  TV.draw_circle(TV.hres()/2,TV.vres()/2,TV.vres()/2,WHITE,INVERT);
  TV.delay(2000);
  //rectangles and lines
  TV.clear_screen();
  TV.draw_rect(20,20,80,56,WHITE);
  TV.delay(500);
  TV.draw_rect(10,10,100,76,WHITE,INVERT);
  TV.delay(500);
  TV.draw_line(60,20,60,76,INVERT);
  TV.draw_line(20,48,100,48,INVERT);
  TV.delay(500);
  TV.draw_line(10,10,110,86,INVERT);
  TV.draw_line(10,86,110,10,INVERT);
  TV.delay(2000);

  //random cube forever.
  TV.clear_screen();
  TV.set_cursor(29,30); TV.print("Rotate cube...");
  TV.delay(2000);

  s = 0x7FFF;
  rmdn.show(s);
  randomSeed(analogRead(2));
}

void loop() {
  int rsteps;
  s.print(tick);
  rmdn.checkButtonsPress();
  if ( rmdn.buttonA || rmdn.buttonB ||
    rmdn.buttonLeft || rmdn.buttonUp || rmdn.buttonDown || rmdn.buttonRight ) {
    m.printStr(" Ar", RED, YELLOW);
    m.printStr("ma", GREEN, ORANGE);
    m.printStr("di", RED, YELLOW);
    m.printStr("no", GREEN, ORANGE);
  }
  rsteps = random(10,60);
  m.clearMatrix();
  switch(random(6)) {
    case 0:
      m.setPenColor(RED);
      m.drawBox(0, 0, 10, 8, rsteps&0x01);
      for (int i = 0; i < rsteps; i++) {
        zrotate(angle);
        printcube();
      }
      break;
    case 1:
      m.setPenColor(GREEN);
      m.drawBox(1, 1, 8, 6, rsteps&0x01);
      for (int i = 0; i < rsteps; i++) {
        zrotate(2*PI - angle);
        printcube();
      }
      break;
    case 2:
      m.setPenColor(YELLOW);
      m.drawBox(2, 2, 6, 4, rsteps&0x01);
      for (int i = 0; i < rsteps; i++) {
        xrotate(angle);
        printcube();
      }
      break;
    case 3:
      m.setPenColor(RED);
      m.drawBox(1, 0, 8, 8, rsteps&0x01);
      for (int i = 0; i < rsteps; i++) {
        xrotate(2*PI - angle);
        printcube();
      }
      break;
    case 4:
      m.setPenColor(GREEN);
      m.drawBox(2, 0, 6, 8, rsteps&0x01);
      for (int i = 0; i < rsteps; i++) {
        yrotate(angle);
        printcube();
      }
      break;
    case 5:
      m.setPenColor(YELLOW);
      m.drawBox(3, 0, 4, 8, rsteps&0x01);
      for (int i = 0; i < rsteps; i++) {
        yrotate(2*PI - angle);
        printcube();
      }
      break;
  }
}

void intro() {
  unsigned char w,l,wb;
  int index;
  w = pgm_read_byte(TVOlogo);
  l = pgm_read_byte(TVOlogo+1);
  if (w&7)
    wb = w/8 + 1;
  else
    wb = w/8;
  index = wb*(l-1) + 2;
  for ( unsigned char i = 1; i < l; i++ ) {
    TV.bitmap((TV.hres() - w)/2,0,TVOlogo,index,w,i);
    index-= wb;
    TV.delay(50);
  }
  for (unsigned char i = 0; i < (TV.vres() - l)/2; i++) {
    TV.bitmap((TV.hres() - w)/2,i,TVOlogo);
    TV.delay(50);
  }
}

void printcube() {
  //calculate 2d points
  for(byte i = 0; i < 8; i++) {
    cube2d[i][0] = (unsigned char)((cube3d[i][0] * view_plane / cube3d[i][2]) + (TV.hres()/2));
    cube2d[i][1] = (unsigned char)((cube3d[i][1] * view_plane / cube3d[i][2]) + (TV.vres()/2));
  }
  TV.delay_frame(1);
  TV.clear_screen();
  draw_cube();
}

void zrotate(float q) {
  float tx,ty,temp;
  for(byte i = 0; i < 8; i++) {
    tx = cube3d[i][0] - xOff;
    ty = cube3d[i][1] - yOff;
    temp = tx * cos(q) - ty * sin(q);
    ty = tx * sin(q) + ty * cos(q);
    tx = temp;
    cube3d[i][0] = tx + xOff;
    cube3d[i][1] = ty + yOff;
  }
}

void yrotate(float q) {
  float tx,tz,temp;
  for(byte i = 0; i < 8; i++) {
    tx = cube3d[i][0] - xOff;
    tz = cube3d[i][2] - zOff;
    temp = tz * cos(q) - tx * sin(q);
    tx = tz * sin(q) + tx * cos(q);
    tz = temp;
    cube3d[i][0] = tx + xOff;
    cube3d[i][2] = tz + zOff;
  }
}

void xrotate(float q) {
  float ty,tz,temp;
  for(byte i = 0; i < 8; i++) {
    ty = cube3d[i][1] - yOff;
    tz = cube3d[i][2] - zOff;
    temp = ty * cos(q) - tz * sin(q);
    tz = ty * sin(q) + tz * cos(q);
    ty = temp;
    cube3d[i][1] = ty + yOff;
    cube3d[i][2] = tz + zOff;
  }
}

void draw_cube() {
  TV.draw_line(cube2d[0][0],cube2d[0][1],cube2d[1][0],cube2d[1][1],WHITE);
  TV.draw_line(cube2d[0][0],cube2d[0][1],cube2d[2][0],cube2d[2][1],WHITE);
  TV.draw_line(cube2d[0][0],cube2d[0][1],cube2d[4][0],cube2d[4][1],WHITE);
  TV.draw_line(cube2d[1][0],cube2d[1][1],cube2d[5][0],cube2d[5][1],WHITE);
  TV.draw_line(cube2d[1][0],cube2d[1][1],cube2d[3][0],cube2d[3][1],WHITE);
  TV.draw_line(cube2d[2][0],cube2d[2][1],cube2d[6][0],cube2d[6][1],WHITE);
  TV.draw_line(cube2d[2][0],cube2d[2][1],cube2d[3][0],cube2d[3][1],WHITE);
  TV.draw_line(cube2d[4][0],cube2d[4][1],cube2d[6][0],cube2d[6][1],WHITE);
  TV.draw_line(cube2d[4][0],cube2d[4][1],cube2d[5][0],cube2d[5][1],WHITE);
  TV.draw_line(cube2d[7][0],cube2d[7][1],cube2d[6][0],cube2d[6][1],WHITE);
  TV.draw_line(cube2d[7][0],cube2d[7][1],cube2d[3][0],cube2d[3][1],WHITE);
  TV.draw_line(cube2d[7][0],cube2d[7][1],cube2d[5][0],cube2d[5][1],WHITE);
}

