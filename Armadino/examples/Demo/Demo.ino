#include <Armadino.h>

Armadino rmdn;
LedMatrix m;
Led7Seg s;
int i;

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
  m.setScrollInterval(120);
  m.printStr("  Aa  Bb  Cc  Dd  Ee  Ff  Gg  Hh  Ii  Jj  Kk  Ll  Mm  Nn  Oo  Pp  Qq  Rr  Ss  Tt  Uu  Vv  Ww  Xx  Yy  Zz    ", RED, GREEN); 
//  m.printStr("  ABCDEFGHIJKLMNOPQRSTUVWXYZ     ", RED, YELLOW); 
//  m.printStr("  abcdefghijklmnopqrstuvwxyz     ", ORANGE, GREEN); 
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 0, RED); m.setPixel(i, 4, RED); }
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 1, ORANGE); m.setPixel(i, 5, ORANGE); }
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 2, GREEN); m.setPixel(i, 6, GREEN); }
  for ( i=0; i<MAX_COLUMN; i++) { m.setPixel(i, 3, YELLOW); m.setPixel(i, 7, YELLOW); }
  s.pattern = 0x7FFF;
}

void loop() {}
