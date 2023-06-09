/*
	Armadino.h - software library for the Armadino
	Copyright (c) 2020 Francis Tay.  All right reserved.

		This library is free software: you can redistribute it and/or modify
		it under the terms of the GNU General Public License as published by
		the Free Software Foundation, either version 3 of the License, or
		(at your option) any later version.

		This library is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
		GNU General Public License for more details.

		You should have received a copy of the GNU General Public License
		along with this library. If not, see <http://www.gnu.org/licenses/>.
	
	Thanks to
	a) Windell H. Oskay for code written for the MeggyJr; and Arthur J. Dahm III
		 and Jay Clegg for code written for the Peggy 2.0;
	b) Ronald Willem Besinger's blog "AVR Twinkle Twinkle Using PWM Project";
	c) Tom Igoe's tutorial on generating tone
	part of which are adapted for this library.
*/

#ifndef ARMADINO_H
#define ARMADINO_H

#include <Arduino.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h> 
#include <avr/pgmspace.h>
#include "Print.h"

#ifndef NULL
#define NULL 0
#endif
#ifndef byte
#define byte uint8_t
#endif
#ifndef boolean
#define boolean uint8_t
#define true	 -1
#define false 	0
#endif

#include "armadino_setup.h"

#ifdef INCLUDE_TVOUT	 // See armadino_setup.h for side effects
#define SOFTWARE_TONE
#define ISR_FREQ 16000 // See note in led_refresh.inc
#else
#define ISR_FREQ 10000 // Timer2 is set to CTC mode (see Armadino.cpp)
											 // (F_CPU / divisor) / MAX_BRIGHTNESS / MAX_COLUMN / FPS,
											 // round to 200 so that the interrupt is at 10000 Hz
											 // for a system clock of 16MHz
#endif

#define FPS 							 120 // Frames per second
#define MAX_COLUMN					10
#define MAX_ROW 						 8
#define DISPLAY_BUFFER_SIZE 80 // MAX_COLUMN*MAX_ROW
#define MAX_BRIGHTNESS			 8 // 8 steps of brightness per pixel, from 0-7
#define HIGH_BRIGHTLEVEL		 8 // Intensity of MAX_BRIGHNESS on the LED matrix, 
#define LOW_BRIGHTLEVEL 		16 // see Armadino.cpp.

#ifdef SOFTWARE_TONE					 // Restrict tone range to C3 to B6.
#define MAX_TONELEVEL 			 1 // i.e no volume control
#else
#define MAX_TONELEVEL 		 128 // Using Timer1 gives fine-grain volume control
#endif
#define ONCE								 0
#define REPEAT							 1

// LED Matrix colors
#define BLINK 					0x80
#define PENCOLOR				0x08
#define REDLEVEL(x) 		(x<=7? x: 0x07)
#define GREENLEVEL(x) 	(x<=7? x<<4: 0x70)
#define CLEAR 					0x00
#define RED 						0x07
#define MIDRED					0x03
#define LIGHTRED				0x01
#define GREEN 					0x70
#define MIDGREEN				0x30
#define LIGHTGREEN			0x10
#define ORANGE					0x33
#define MIDORANGE 			0x22
#define LIGHTORANGE 		0x11
#define YELLOW					0x73
#define MIDYELLOW 			0x42
#define LIGHTYELLOW 		0x41

class LedMatrix : public Print
{
	public:
		/* An array of byte is used to specify the colour.
			 +-------+-------+-------+-------+-------+-------+-------+-------+
			 |	MSB  |			 |			 |			 |			 |			 |			 |	LSB  |
			 +-------+-------+-------+-------+-------+-------+-------+-------+
				Blink?	<------  Green	------> 	 X		<------ 	Red  ------>
		Each byte is made up of two 3-bit-depth of green and red. The MSB is used to
		indicate whether the pixel should blink. The 4th bit is not used.
		Pixels are stored column-wise in the array, (0,0) being the top left corner,
		i.e. array[0] is (0,0), array[1] (0,1) ...*/
		uint8_t pixArray[DISPLAY_BUFFER_SIZE];
		/* scrollInterval, in milliseconds, determines how fast successive columns
			 of a character are printed (or scrolled) across the LED matrix */
		uint8_t penColor, scrollInterval;
		LedMatrix( uint8_t pen_color = RED, uint8_t scroll_interval = 120 );
		LedMatrix& operator= ( const LedMatrix &rhs );
		LedMatrix& operator= ( const byte *m );
		void loadBitmap ( const byte *m ); // m is a PROGMEM array
		void setPenColor ( uint8_t color ) { penColor = color; };
		uint8_t getPenColor ( ) { return penColor; };
		// Top left corner is co-ordinates (0, 0)
		void setPixel ( uint8_t x, uint8_t y, uint8_t color = PENCOLOR );
		uint8_t getPixel ( uint8_t x, uint8_t y );
#define NO_FILL 0
#define FILL		1
		void drawBox(uint8_t x, uint8_t y, uint8_t length, uint8_t width, uint8_t fill = NO_FILL);
		void clearMatrix ( );
		void shiftLeft ( );
		void shiftRight ( );
		void shiftUp ( );
		void shiftDown ( );
		void setScrollInterval ( uint8_t interval = 120 )
			{ scrollInterval = interval; };
		uint8_t getScrollInterval ( ) { return scrollInterval; };
		virtual size_t write( uint8_t value );
		void printStr ( char *str, uint8_t color1 = PENCOLOR, uint8_t color2 = 0 ); // in alternating colours
};

class Led7Seg
{
	public:
		/* An unsigned integer (16 bits) is used to represent the two 7-segment LEDs,
		where the left byte is for the left LED, right byte for the right. Each bit
		indicates the state of each segment, the LSB corresponding to segment 'a'
		and so forth: 						 a
															---
														f|	 |b
														 | g |
															---
														e|	 |c
														 | d |
															---  *(dp)
		The MSB of the left byte for the left decimal point; and the MSB of the
		right byte for blinking. */
		uint16_t pattern;
		uint8_t brightness;
		Led7Seg ( uint16_t p=0, uint8_t b=2 ) { pattern = p; brightness = b; };
		Led7Seg& operator= ( const Led7Seg &rhs );
		Led7Seg& operator= ( const uint16_t p );
		void setPattern ( uint16_t p ) { pattern = p; };
		uint16_t getPattern ( ) { return pattern; };
		void setBrightness ( uint8_t b ) {
			if (b>MAX_BRIGHTNESS) brightness=MAX_BRIGHTNESS; else brightness=b; };
		uint8_t getBrightness ( ) { return brightness; };
		void print ( int i );
		void print ( uint16_t i ) { print ((int) i); };
		void print ( uint8_t i ) { print ((int) i); };
		void print ( float f );
};

#define BUTTON_RIGHT	0x04		 // Button B1
#define BUTTON_DOWN 	0x08		 // Button B2
#define BUTTON_UP 		0x10		 // Button B4
#define BUTTON_LEFT 	0x20		 // Button B3
#define BUTTON_A			0x40		 // Button B5
#define BUTTON_B			0x80		 // Button B6

class Armadino
{
	public :
		byte buttonRight;
		byte buttonDown;
		byte buttonUp;
		byte buttonLeft;
		byte buttonA;
		byte buttonB;
		static int brightLevel;
		static uint8_t soundLevel;
		static Led7Seg *current7Seg;
		static LedMatrix *currentMatrix;
		static void (*hook) ( );
		static unsigned int hookPeriod;

		void begin ( );
		byte checkButtonsDown ( );
		byte checkButtonsPress ( );
		void show ( LedMatrix &m );
		void show ( Led7Seg &s );
		void screenOff ( ); void screenOn ( ); boolean screenState ( );
		void setBrightLevel( int l ) { brightLevel = l; };
		int getBrightLevel ( ) { return brightLevel; };
		void segLedOff ( ); void segLedOn ( ); boolean segLedState ( );
		void tone ( unsigned int frequency, unsigned int duration );
		void beep ( );
		void toneOn ( ); void toneOff ( ); boolean toneState ( );
		void setToneLevel ( uint8_t level );
		uint8_t getToneLevel ( ) { return soundLevel; };
		boolean makingTone ( );
		// tune is an integer array of tone frequencies (defined in Armadino.h) and
		// duration: N4 = quarter note, N8 = eighth note, etc. For example,
		//	 int tune[] = { C4, N4, G3, N8, G3, N8 , ... , End }
		// where End (defined as -1) signals the end of the tune.
		// The second argument causes the tune to be repeated when set to non-zero.
		void playMelody ( int *tune, uint8_t repeat = ONCE );
		void stopMelody ( );
		boolean makingMelody ( );
		// if hooked, (*func)() is called every msec (milli-seconds).
		void setHook ( void (*func)() = NULL, unsigned int msec = 0 );
		static void delay ( unsigned int duration );
		static unsigned long millis ( );
#ifdef INCLUDE_TVOUT
		static void ledRefresh ( );
#endif
};

#define ARMADINO_ICON 										 \
{ 0, 0, 0, 0, 0, GREEN, 0, 0, 						 \
	0, 0, 0, 0, ORANGE, ORANGE, 0, 0, 			 \
	0, 0, 0, RED, GREEN+BLINK, RED, 0, 0, 	 \
	0, 0, 0, YELLOW, YELLOW, YELLOW, 0, 0,	 \
	0, 0, GREEN, GREEN, GREEN, GREEN, 0, 0,  \
	0, 0, 0, ORANGE, ORANGE, ORANGE, 0, 0,	 \
	0, 0, 0, RED, RED, RED, 0, 0, 					 \
	0, 0, 0, 0, YELLOW, YELLOW, 0, GREEN, 	 \
	0, 0, 0, 0, 0, GREEN, 0, GREEN, 				 \
	0, 0, 0, 0, 0, 0, GREEN, 0 }

#ifdef SOFTWARE_TONE
#define FREQ(x) 	(ISR_FREQ / x)	// number of ISR calls
#else
#define FREQ(x) 	(x)
#endif
/* Tone frequencies, in Hz from http://www.phy.mtu.edu/~suits/notefreqs.html */
#define End		-1
#define Rest	0
#define Beep	FREQ( 500)
#define B_0		FREQ(  31)
#define C_1		FREQ(  33)
#define Cs_1 	FREQ(  35)
#define D_1		FREQ(  37)
#define Ds_1 	FREQ(  39)
#define E_1		FREQ(  41)
#define F_1		FREQ(  44)
#define Fs_1 	FREQ(  46)
#define G_1		FREQ(  49)
#define Gs_1 	FREQ(  52)
#define A_1		FREQ(  55)
#define As_1 	FREQ(  58)
#define B_1		FREQ(  62)
#define C_2		FREQ(  65)
#define Cs_2 	FREQ(  69)
#define D_2		FREQ(  73)
#define Ds_2 	FREQ(  78)
#define E_2		FREQ(  82)
#define F_2		FREQ(  87)
#define Fs_2 	FREQ(  93)
#define G_2		FREQ(  98)
#define Gs_2 	FREQ( 104)
#define A_2		FREQ( 110)
#define As_2 	FREQ( 117)
#define B_2		FREQ( 123)
#define C_3		FREQ( 131)
#define Cs_3 	FREQ( 139)
#define D_3		FREQ( 147)
#define Ds_3 	FREQ( 156)
#define E_3		FREQ( 165)
#define F_3		FREQ( 175)
#define Fs_3 	FREQ( 185)
#define G_3		FREQ( 196)
#define Gs_3 	FREQ( 208)
#define A_3		FREQ( 220)
#define As_3 	FREQ( 233)
#define B_3		FREQ( 247)
#define C_4		FREQ( 262)
#define Cs_4 	FREQ( 277)
#define D_4		FREQ( 294)
#define Ds_4 	FREQ( 311)
#define E_4		FREQ( 330)
#define F_4		FREQ( 349)
#define Fs_4 	FREQ( 370)
#define G_4		FREQ( 392)
#define Gs_4 	FREQ( 415)
#define A_4		FREQ( 440)
#define As_4 	FREQ( 466)
#define B_4		FREQ( 494)
#define C_5		FREQ( 523)
#define Cs_5 	FREQ( 554)
#define D_5		FREQ( 587)
#define Ds_5 	FREQ( 622)
#define E_5		FREQ( 659)
#define F_5		FREQ( 698)
#define Fs_5 	FREQ( 740)
#define G_5		FREQ( 784)
#define Gs_5 	FREQ( 831)
#define A_5		FREQ( 880)
#define As_5 	FREQ( 932)
#define B_5		FREQ( 988)
#define C_6		FREQ(1047)
#define Cs_6 	FREQ(1109)
#define D_6		FREQ(1175)
#define Ds_6 	FREQ(1245)
#define E_6		FREQ(1319)
#define F_6		FREQ(1397)
#define Fs_6 	FREQ(1480)
#define G_6		FREQ(1568)
#define Gs_6 	FREQ(1661)
#define A_6		FREQ(1760)
#define As_6 	FREQ(1865)
#define B_6		FREQ(1976)
#define C_7		FREQ(2093)
#define Cs_7 	FREQ(2217)
#define D_7		FREQ(2349)
#define Ds_7 	FREQ(2489)
#define E_7		FREQ(2637)
#define F_7		FREQ(2794)
#define Fs_7 	FREQ(2960)
#define G_7		FREQ(3136)
#define Gs_7 	FREQ(3322)
#define A_7		FREQ(3520)
#define As_7 	FREQ(3729)
#define B_7		FREQ(3951)
#define C_8		FREQ(4186)
#define Cs_8 	FREQ(4435)
#define D_8		FREQ(4699)
#define Ds_8 	FREQ(4978)

// Note types or duration. The note duration is one second divide by note type,
// e.g. quarter note = 1000/4, eighth note = 1000/8, etc

#define N_1		1000
#define N_2		500
#define N_3		333
#define N_4		250
#define N_6		167
#define N_8		125
#define N_9		111
#define N_10 	100
#define N_12 	83
#define N_16 	63
#define N_18 	56
#define N_32 	31

#endif
