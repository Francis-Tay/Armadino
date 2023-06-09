/*
	Armadino.cpp - software library for the Armadino
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

#include "Armadino.h"

static uint8_t sreg;		 // Interrupt flag and counters
void (*Armadino::hook)( ) = NULL;
unsigned int Armadino::hookPeriod = 0;
int Armadino::brightLevel = HIGH_BRIGHTLEVEL;
uint8_t Armadino::soundLevel = 8;
LedMatrix *Armadino::currentMatrix = NULL;
Led7Seg *Armadino::current7Seg = NULL;

static boolean toneEnabled = true, screenEnabled = true, segLedEnabled = true;

void Armadino::begin( )
{
	DDRC |= B00000001;		 // PC0 is output
	DDRC &= B11000001;		 // Rest are inputs, no pull-ups

	// Pins PD2-7 for button inputs.
	DDRD = B00000010;
	PORTD |= B11111100; 	 // Pull-ups on these pins

	DDRB |= B00111111;		 // All are outputs
	PORTB &= B11000000;

	SPSR = (1<<SPI2X);								 // Enable double-speed SPI
	SPCR = (1 << SPE) | (1 << MSTR);	 // Enable SPI
	SPDR = 0;
	while ( !(SPSR & (1<<SPIF)) );		 // Wait for bitshift to complete
	SPDR = 0;
	while ( !(SPSR & (1<<SPIF)) );
	SPCR = 0; 												 // Turn off SPI
	PORTB |= B00010001; 	 // Strobe Latch Enable on shift register and reset 4017
	PORTB &= B11101110;

#ifndef INCLUDE_TVOUT
	TCCR2A = (1<<WGM21);	 // Set Timer2 to CTC mode, thereby PWM 3 and 11 are affected
	TCCR2B = (1<<CS21); 	 // Divisor of 8
	OCR2A  = 200; 				 // (F_CPU / divisor) / MAX_BRIGHTNESS / MAX_COLUMN / FPS,
												 // round to 200 so that the interrupt is at 10000 Hz
												 // for a system clock of 16MHz
	TIMSK2 = (1<<OCIE2A);  // Enable interrupts
	sei( );
#endif

#ifdef SOFTWARE_TONE
	Armadino::soundLevel = 1;
#else
	Armadino::soundLevel = 8;
#endif
	// If button A or B is pressed at startup, disable sound.
	if ( (PIND & B11000000) != B11000000 )
		toneEnabled = false;
	else
		toneEnabled = true;
}

// Button states
static byte buttonState = 0, buttonLastState = 0, buttonPress = 0;

byte Armadino::checkButtonsDown( )
{ 
	byte i = buttonState;
	i>>=2; Armadino::buttonRight = (i & 1);
	i>>=1; Armadino::buttonDown  = (i & 1);
	i>>=1; Armadino::buttonUp 	 = (i & 1);
	i>>=1; Armadino::buttonLeft  = (i & 1);
	i>>=1; Armadino::buttonA		 = (i & 1);
	i>>=1; Armadino::buttonB		 = (i & 1);
	return buttonState;
}

byte Armadino::checkButtonsPress( )
{
	byte i = buttonPress, j = buttonPress;
	i>>=2; Armadino::buttonRight = (i & 1);
	i>>=1; Armadino::buttonDown  = (i & 1);
	i>>=1; Armadino::buttonUp 	 = (i & 1);
	i>>=1; Armadino::buttonLeft  = (i & 1);
	i>>=1; Armadino::buttonA		 = (i & 1);
	i>>=1; Armadino::buttonB		 = (i & 1);
	buttonPress = 0;
	return j;
}

static unsigned int toneDuration = 0; // How long the tone lasts, in msec
#ifdef SOFTWARE_TONE
static uint8_t tonePeriod = 0;				// For one wavelength, in ISR counts
static uint8_t periodTimer = 0;
#endif

// Play a tone of frequency (as defined in Armadino.h) for duration (in
// milliseconds). To stop, call startTone(0, 0).
void startTone( unsigned int frequency, unsigned int duration )
{
	if ( duration > 0 )
		toneDuration = duration;
	else
		frequency = toneDuration = 0;
	if ( frequency != 0 && toneEnabled && Armadino::soundLevel != 0 )
#ifdef SOFTWARE_TONE
		periodTimer = tonePeriod = frequency;
	else
		periodTimer = tonePeriod = 0;
	PORTB &= B11111011;
#else
	{ // thereby PWM 9 and 10 are affected
		DDRB |= 4;
		// COM1B1:0 = 10 : Clear OC1B on compare match when up-counting, set on down
		// WGM11:0 = 00 : Phase+frequency correct PWM, TOP = ICR1
		TCCR1A = (1<<COM1B1);
		// WGM13:2 = 10 : Phase+frequency correct PWM, TOP = ICR1
		// CS12,CS11,CS10 = 010 : Count at CLKio/8 (prescaled by 8)
		TCCR1B = (1<<WGM13) | (1<<CS11);
		TCNT1 = 0;
		// Frequency is given by the following formula:
		// f = CLKio / (2 x Prescaling x divisor)
		//	 = 1 MHz/divisor
		// Thus divisor = 1 MHz/f.
		ICR1 = (int) ( 1000000L / frequency );
		OCR1B = Armadino::soundLevel;
	}
	else {
		TCCR1A = 0;
		TCCR1B = 0;
		PORTB &= B11111011;
	}
#endif
}

void Armadino::tone( unsigned int frequency, unsigned int duration )
{
	startTone( frequency, duration );
}

void Armadino::beep( )
{
	startTone( Beep, 125 );
}

void Armadino::toneOff( )
{
	toneEnabled = false;
}

void Armadino::toneOn( )
{
	toneEnabled = true;
}

boolean Armadino::toneState( )
{
	return toneEnabled;
}

void Armadino::setToneLevel( uint8_t level )
{
	if ( level > MAX_TONELEVEL )
		Armadino::soundLevel = MAX_TONELEVEL;
	else
		Armadino::soundLevel = level;
}

boolean Armadino::makingTone( )
{
	return ( toneDuration > 0 );
}

static int *melodyNotes = NULL, noteIndex = 0;
static uint8_t repeatMelody = ONCE;

// Play a melody.
// tune is an integer array of tone frequencies (defined in Armadino.h) and
// duration: N4 = quarter note, N8 = eighth note, etc. For example,
//	 int tune[] = { C4, N4, G3, N8, G3, N8 , ... , End }
// where End (defined as -1) signals the end of the tune.
// The second argument causes the tune to be repeated when set to non-zero.
void Armadino::playMelody( int *tune, uint8_t repeat )
{
	sreg = SREG;									 // Remember interrupt flag
	cli();												 // Clear interrupt flag
	melodyNotes = tune;
	SREG = sreg;									 // Restore interrupt flag
	noteIndex = 0;
	repeatMelody = repeat;
}

void Armadino::stopMelody( )
{
	melodyNotes = NULL;
}

boolean Armadino::makingMelody( )
{
	return ( melodyNotes != NULL );
}

static uint8_t currentCol = 0, currentBrightness = 0, reset = 0;
static byte *currentColPtr = NULL, *ptr, p, bits, blinkMask = 0;
volatile static unsigned int delayTimer = 0;
volatile static unsigned int hookTimer = 0;
volatile static unsigned long sysUpTime = 0UL; // msec tick since program starts
static uint8_t debounceTimer = 0, blinkTimer = BLINK_DELAY;
static unsigned int pauseBetweenNotes = 0;	 // In msec, for distinctive notes

// Milliseconds since the program starts
unsigned long Armadino::millis( )
{
	return sysUpTime;
}

void Armadino::delay( unsigned int duration )  // in milliseconds
{
	sreg = SREG;									 // Remember interrupt flag
	cli();												 // Clear interrupt flag
	delayTimer = duration;
	SREG = sreg;									 // Restore interrupt flag
	while ( delayTimer != 0 ) 	 // May terminate prematurely due to race
		while ( delayTimer != 0 ); // condition with ISR, hence the dual loop
}

void Armadino::show( LedMatrix &m )
{
	sreg = SREG;									 // Remember interrupt flag
	cli();												 // Clear interrupt flag
	currentMatrix = &m;
	currentCol = 0;
	currentColPtr = (byte *) currentMatrix->pixArray;
	SREG = sreg;									 // Restore interrupt flag
}

void Armadino::show( Led7Seg &s )
{
	current7Seg = &s;
}

void Armadino::screenOff( )
{
	screenEnabled = false;
}

void Armadino::screenOn( )
{
	screenEnabled = true;
}

boolean Armadino::screenState( )
{
	return screenEnabled;
}

void Armadino::segLedOff( )
{
	PORTC &= B11111110; 		// Turn off 7-segment display
	segLedEnabled = false;
}

void Armadino::segLedOn( )
{
	segLedEnabled = true;
}

boolean Armadino::segLedState( )
{
	return segLedEnabled;
}

// The function, (*func)(), is hooked to a timer interrupt and will be called
// every millisecond. It should therefore finish its work quickly.
void Armadino::setHook( void (*func)(), unsigned int msec )
{
	if ( func != NULL && msec > 0L ) {
		hook = func;
		hookPeriod = msec;
	}
	else {
		hook = NULL;
		hookPeriod = 0;
	}
}

#ifdef INCLUDE_TVOUT
#include "led_refresh.inc"
#else
ISR( TIMER2_COMPA_vect )
{
	// There are Armadino::brightLevel passes through this interrupt for each
	// column per frame. At Armadino::brightLevel = HIGH_BRIGHTLEVEL (8), this is
	// thus 8x10 = 80 times per frame.
	// During each pass, a lED can be on or off. If the pixel is set at 0, the
	// perceived brightness is 0/8, i.e. off the entire time. At 7 (binary 111),
	// the perceived brightness is 7/8, giving a total of 8 average brightness
	// levels.
	// currentBrightness is a comparison variable, used to determine if a certain
	// pixel should be turned on or off during any one of those 8 cycles.
	// For a nominal frame rate of 120Hz, this must execute at 8*10*120=9600
	// times per second. Timer2 is thus set to interrupt at 10000 times per
	// second in begin().
	// Set Armadino::brightLevel to LOW_BRIGHTLEVEL (16) to dim the LED, at half
	// the frame rate. 

	// Software-generated tone
#ifdef SOFTWARE_TONE
	if ( periodTimer != 0 )
		if ( --periodTimer == 1 ) 					 // Pulse width of 1 ISR count
			PORTB |= B00000100;
		else if ( periodTimer == 0 ) {
			PORTB &= B11111011;
			periodTimer = tonePeriod;
		}
#endif

	static uint8_t tickTimer = (ISR_FREQ / 1000);
	// 1 millisecond has passed. Handle timers, tone duration and button debounce
	if ( --tickTimer == 0 ) {
		tickTimer = (ISR_FREQ / 1000);						// Reset the counter
		if (Armadino::hookPeriod!=0 && Armadino::hookPeriod==hookTimer &&
			Armadino::hook!=NULL) {
			hookTimer = 0;
			(*Armadino::hook)();
		} 		 
		sysUpTime++; hookTimer++; 							// for Armadino::millis() and hook()
		if ( delayTimer != 0 ) delayTimer--;	// for Armadino::delay()
		if ( toneDuration == 0 ) {
			if ( melodyNotes )
				if ( pauseBetweenNotes != 0 )
					pauseBetweenNotes--;
				else																	// Continue with next note
					if ( melodyNotes[noteIndex] < 0 ) { // Melody ends
						noteIndex = 0;
						if ( repeatMelody == ONCE ) melodyNotes = NULL;
					}
					else {
						// Note duration is one second divide by note type, e.g.
						// quarter note = 1000/4, eighth note = 1000/8, etc
						// To distinguish the notes, set a pause between them that is
						// equal to the note's duration.
						startTone( melodyNotes[noteIndex],
							pauseBetweenNotes = melodyNotes[noteIndex+1] );
						noteIndex+=2;
					}
		}
		else
			if ( --toneDuration == 0 ) startTone(0, 0); // Stop making tone
		if ( debounceTimer != 0 ) {
			if ( --debounceTimer == 0 ) {
				// Use a byte with a bit set for each button that is down/pressed
				buttonState = ~PIND & B11111100; // B, A, left, up, down, right
				buttonPress |= (buttonState & ~buttonLastState);
				buttonLastState = buttonState;
			}
		}
		else
			if ( (~PIND & B11111100) != buttonLastState ) // A button has changed state,
				debounceTimer = DEBOUNCE_TIME;							// start the debounce timer
		// Alternate the blinkMask at interval of BLINK_DELAY
		if ( --blinkTimer == 0 ) {
			blinkTimer = BLINK_DELAY;
			blinkMask = ( blinkMask? 0: BLINK );
		}
	}

	// Perform tasks during 4017 reset
	switch ( reset ) {
		// Do other tasks, e.g. light the 7-segment display
		case 1:
			SPCR = (1 << SPE) | (1 << MSTR);	// Enable SPI, MSB first, Master
			if ( Armadino::current7Seg == NULL || !segLedEnabled ||
					Armadino::current7Seg->pattern&blinkMask ||
					Armadino::current7Seg->brightness <= currentBrightness )
				SPDR = 0;
			else
				SPDR = Armadino::current7Seg->pattern & 0x7F;
			while ( !(SPSR & (1<<SPIF)) );		// Wait for prior bitshift to complete
			if ( Armadino::current7Seg == NULL || !segLedEnabled ||
					Armadino::current7Seg->pattern&blinkMask ||
					Armadino::current7Seg->brightness <= currentBrightness )
				SPDR = 0;
			else
				SPDR = Armadino::current7Seg->pattern>>8;
			while ( !(SPSR & (1<<SPIF)) );
			SPCR = 0; 						 // Turn off SPI
			PORTB |= B00010000; 	 // Strobe LE on shift register
			PORTB &= B11101110; 	 // Deactivate Reset on 4017 and Col 0
			if ( segLedEnabled )
				PORTC |= B00000001;  // Turn on 7-seg LEDs
			reset++;
			return;
		case 2:
			if ( segLedEnabled )
				PORTC &= B11111110;  // Turn off 7-segment display
		default:
			reset = 0;						 // Clear the reset flag and continue
	}

	// Display LED matrix
	// Parse a column of pixel and write out the bits via SPI
	if ( ++currentCol == MAX_COLUMN ) {
		currentCol = 0;
		if ( ++currentBrightness == Armadino::brightLevel )
			currentBrightness = 0;
	}
	if ( Armadino::currentMatrix != NULL )
		if ( currentCol == 0 )
			currentColPtr = (byte *) Armadino::currentMatrix->pixArray;
		else
			currentColPtr += MAX_ROW;
	SPCR = (1 << SPE) | (1 << MSTR);
	bits = 0;
	if ( currentColPtr != NULL && screenEnabled ) {
		ptr = currentColPtr + MAX_ROW - 1;	// Work from bottom row up
		// Red LED
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 128;
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 64;
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 32;
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 16;
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 8;
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 4;
		p = *ptr--;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 2;
		p = *ptr;
		if ( !(p&blinkMask) && (p&0x07) > currentBrightness ) bits |= 1;
	}
	SPDR = bits;
	bits = 0;
	if ( currentColPtr != NULL && screenEnabled ) {
		ptr = currentColPtr + MAX_ROW - 1;
		// Green LED
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 128;
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 64;
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 32;
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 16;
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 8;
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 4;
		p = *ptr--;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 2;
		p = *ptr;
		if ( !(p&blinkMask) && ((p>>4)&0x07) > currentBrightness ) bits |= 1;
	}
	while ( !(SPSR & (1<<SPIF)) );
	SPDR = bits;
	while ( !(SPSR & (1<<SPIF)) );
	SPCR = 0;
	if ( currentCol == 0 ) {
		reset = 1;
		PORTB |= B00010001; // Strobe Latch Enable on shift register and reset 4017
	}
	else
		PORTB |= B00010000; // Strobe Latch Enable on shift register and 4017 clock
	PORTB &= B11101111;
}
#endif

#include "char_set.inc"

Led7Seg& Led7Seg::operator=( const Led7Seg &rhs )
{
	if ( this != &rhs ) this->pattern = rhs.pattern;
	return *this;
}

Led7Seg& Led7Seg::operator=( const unsigned int p )
{
	this->pattern = p;
	return *this;
}

void Led7Seg::print( int i )
{
	if ( i<0 && i>=-9 )
		pattern = 0x4000 | pgm_read_byte(ledNum-i);
	else
		if ( i>=0 && i<=99 )
			pattern = (pgm_read_byte(ledNum+i/10)<<8) | pgm_read_byte(ledNum+i%10);
		else
			pattern = 0x4040 | BLINK;  // Blinking dashes
}

void Led7Seg::print( float f )
{
	int8_t i;
	if ( f>=0.0F && f<100.0F ) {
		if ( f>10.0F ) {
			i = (int) f;
			Led7Seg::print(i);
		} 		 
		else {
		 i = (int) (f*10.0F);
		 Led7Seg::print(i);
		 pattern |= 0x8000; 				 // Insert decimal point
		}
	}
	else
		pattern = 0xC040 | BLINK; 	// Blinking dashes and decimal point
}

LedMatrix::LedMatrix( uint8_t pen_color, uint8_t scroll_interval )
{
	penColor = pen_color;
	scrollInterval = scroll_interval;
	clearMatrix();
}

LedMatrix& LedMatrix::operator=( const LedMatrix &rhs )
{
	if ( this != &rhs )
		memcpy(this->pixArray, rhs.pixArray, DISPLAY_BUFFER_SIZE);
	return *this;
}

LedMatrix& LedMatrix::operator=( const byte *m )
{
	if ( this->pixArray != m )
		memcpy(this->pixArray, m, DISPLAY_BUFFER_SIZE);
	return *this;
}

void LedMatrix::loadBitmap( const byte *m )
{
	memcpy_P(pixArray, m, DISPLAY_BUFFER_SIZE);
}

#define PIXEL(x, y) pixArray[(x)*MAX_ROW + (y)]

void LedMatrix::setPixel( uint8_t x, uint8_t y, uint8_t color )
{
	PIXEL(x, y) = ( color==PENCOLOR? penColor: color );
}

uint8_t LedMatrix::getPixel( uint8_t x, uint8_t y )
{
	return PIXEL(x, y);
}

void LedMatrix::drawBox( uint8_t x, uint8_t y, uint8_t l, uint8_t w, uint8_t fill )
{
	uint8_t i, j;
	for ( i = 0; i < l; i++ ) PIXEL(x+i, y) = penColor;
	for ( j = 1; j < w; j++ ) {
		PIXEL(x, y+j) = penColor;
		if ( fill )
			for ( i = 1; i < l; i++ ) PIXEL(x+i, y+j) = penColor;
		else
			PIXEL(x+l-1, y+j) = penColor;
	}
	for ( y = y+w-1, i = 1; i < l; i++ ) PIXEL(x+i, y) = penColor;
}

void LedMatrix::clearMatrix( )
{
	uint8_t i;
	for ( i = 0; i < DISPLAY_BUFFER_SIZE; i++ ) pixArray[i] = 0;
}

void LedMatrix::shiftLeft( )
{
	uint8_t i = MAX_ROW, j = 0;
	while ( i < DISPLAY_BUFFER_SIZE ) pixArray[j++] = pixArray[i++];
	for ( j = DISPLAY_BUFFER_SIZE - MAX_ROW; j < DISPLAY_BUFFER_SIZE; j++ )
		pixArray[j] = 0;
}

void LedMatrix::shiftRight( )
{
	uint8_t i = DISPLAY_BUFFER_SIZE-1-MAX_ROW, j = DISPLAY_BUFFER_SIZE-1;
	while ( i > 0 ) pixArray[j--] = pixArray[i--];
	for ( j = 0; j < MAX_ROW; j++ ) pixArray[j] = 0;
}

void LedMatrix::shiftUp( )
{
	uint8_t i, j, k;
	for ( i = 0; i < MAX_COLUMN; i++ )	{
		for ( j = i*MAX_ROW, k = 1; k < MAX_ROW; j++, k++ )
			pixArray[j] = pixArray[j+1];
		pixArray[j]=0;
	}
}

void LedMatrix::shiftDown( )
{
	uint8_t i, j, k;
	for (i = 1; i <= MAX_COLUMN; i++)  {
		for ( j = i*MAX_ROW-1, k = 1; k < MAX_ROW; j--, k++ )
			pixArray[j] = pixArray[j-1];
		pixArray[j]=0;
	}
}

// Scroll a text string across the LED matrix, with alternating colours
void LedMatrix::printStr( char *str, uint8_t color1, uint8_t color2 )
{
	uint8_t i, pen_color;
	char c;

	pen_color = penColor;
	if ( color1 == PENCOLOR ) color1 = penColor;
	if ( color2 == 0 ) color2 = color1;
	else if ( color2 == PENCOLOR ) color2 = penColor;
	penColor = color1;
	i = 0;
	while ( c = str[i] )
	{
		write((uint8_t) c);
		penColor = ( penColor==color1? color2: color1 );
		i++;
	}
	penColor = pen_color;
}

/* Write a character, from ASCII 33 to 126, on the LED matrix.
Each character is an 5 columns x 8 row bitmap. A byte is used to represent each
column. 4th and 5th columns are 'optional' such that subsequent columns will be
skipped over too when any of these columns contains all 0's. */
size_t LedMatrix::write( uint8_t value )
{
	uint8_t i, j;
	byte pattern;

	if ( value < 33 || value > 126 ) { // Print as space character
		shiftLeft(); Armadino::delay(scrollInterval);
		shiftLeft(); Armadino::delay(scrollInterval);
	}
	else {
		value = value - 33; // Offset by 33 into the character lookup table
		// For each character, shift left one column at a time and fill in with the
		// next column in the character's display pattern.
		for ( i = 0; i < CHAR_WIDTH; i++ ) {
			pattern = pgm_read_byte(charSet+CHAR_WIDTH*value+i);
			if ( i >= CHAR_WIDTH-2 && pattern == 0 ) break;
			else {
				shiftLeft();
				for ( j = 0; j < MAX_ROW; j++ ) {
					if ( (pattern&0x01) == 0x01 )
						pixArray[DISPLAY_BUFFER_SIZE-MAX_ROW+j] = penColor;
					pattern >>= 1;
				}
				Armadino::delay(scrollInterval);
			}
		}
	}
	return 1;
}
