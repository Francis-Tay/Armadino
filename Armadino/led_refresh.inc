// This set of functions are used in place of Timer2 interrupt when Armadino
// is used with the TVout library. TVout::set_hbi_hook() is set to call
// Armadino::ledRefresh(). Each scan line is about 64 usec, approximately 16kHz.
// Hence a cycle of 16 stages last 1 msec. For more details, see armadino_setup.h

void stage0();
void stage1();
void stage2(); void stage2a();
void stage3();
void stage4();
void stage5(); void stage5a();
void stage6(); void stage6a(); void stage6b();
void stage7(); void stage7a(); void stage7b();
void stage8(); void stage8a(); void stage8b();
void stage9(); void stage9a(); void stage9b();
void stage10(); void stage10a();
void stage11();
void stage12();
void stage13();
void stage14();
void stage15();

static void (*nextStage)() = &stage0;
static uint8_t stageCount = 0;

#define makeTone()									 \
	if ( periodTimer != 0 ) { 				 \
		if ( --periodTimer == 1 ) 			 \
				PORTB |= B00000100; 				 \
		else if ( periodTimer == 0 ) {	 \
				PORTB &= B11111011; 				 \
				periodTimer = tonePeriod; 	 \
			} 														 \
	}

void Armadino::ledRefresh( )
{
	(*nextStage)();
}

void stage0( )
{
	makeTone();
	stageCount = 0;
	nextStage = &stage1;
	if (Armadino::hookPeriod!=0 && Armadino::hookPeriod==hookTimer &&
		Armadino::hook!=NULL) {
		hookTimer = 0;
		(*Armadino::hook)();
	} 		 
}

void stage1( )
{
	makeTone();
	if ( toneDuration == 0 ) {
		if ( melodyNotes )
			if ( pauseBetweenNotes != 0 ) pauseBetweenNotes--;
			else																		 // Continue with next note
				if ( melodyNotes[noteIndex] < 0 ) { 	 // Melody ends
					noteIndex = 0;
					if ( repeatMelody == ONCE ) melodyNotes = NULL;
				}
				else {
					nextStage = &stage2a; 		 // Play the next note in the melody
					return;
				}
	}
	else
		if ( --toneDuration == 0 ) {		// Stop making tone
			periodTimer = tonePeriod = 0;
			PORTB &= B11111011;
		}
	nextStage = &stage2;
}

void stage2a( )
{
	if ( toneEnabled && Armadino::soundLevel != 0 )
		periodTimer = tonePeriod = melodyNotes[noteIndex];
	else
		periodTimer = tonePeriod = 0;
	pauseBetweenNotes = toneDuration = melodyNotes[noteIndex+1];
	noteIndex+=2;
	nextStage = &stage3;							 // Skip debouncing in this cycle
}

void stage2( )
{
	makeTone();
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
	nextStage = &stage3;
}

void stage3( )
{
	makeTone();
	if ( delayTimer != 0 ) delayTimer--;
	if ( --blinkTimer == 0 ) {
		blinkTimer = BLINK_DELAY;
		blinkMask = ( blinkMask==BLINK? 0: BLINK );
	}
	nextStage = &stage4;
}

void stage4( )
{
	makeTone();
	sysUpTime++; hookTimer++;
	if ( reset == 1 )
		nextStage = &stage5a; 	// Branch to light the 7-seg LEDs (stages 5a-9a)
	else {
		if ( segLedEnabled )
			PORTC &= B11111110; 						 // Turn off 7-segment display
		if ( ++currentCol == MAX_COLUMN )  // Light the next matrix column
			currentCol = 0;
		nextStage = &stage5;	 // Branch to light the LED matrix (stages 5-15)
	}
}

void stage5a( )
{
	makeTone();
	if ( Armadino::current7Seg == NULL || !segLedEnabled ||
		 Armadino::current7Seg->pattern&blinkMask )
		bits = 0;
	else
		bits = Armadino::current7Seg->pattern & 0x7F;
	nextStage = &stage6a;
}

void stage6a( )
{
	makeTone();
	SPCR = (1 << SPE) | (1 << MSTR);	// Enable SPI, MSB first, Master
	SPDR = bits;
	nextStage = &stage7a;
}

void stage7a( )
{
	makeTone();
	if ( Armadino::current7Seg == NULL || !segLedEnabled ||
		 Armadino::current7Seg->pattern&blinkMask )
		bits = 0;
	else
		bits = (Armadino::current7Seg->pattern>>8) & 0xFF;
	nextStage = &stage8a;
}

void stage8a( )
{
	makeTone();
	SPDR = bits;
	reset = 0;
	nextStage = &stage9a;
}

void stage9a( )
{
	makeTone();
	SPCR = 0; 						 // Turn off SPI
	PORTB |= B00010000; 	 // Strobe LE on shift register
	PORTB &= B11101110; 	 // Deactivate Reset on 4017 and Col 0
	if ( segLedEnabled )
		PORTC |= B00000001;  // Turn on 7-seg LEDs
	nextStage = &stage10a;
}

void stage10a( )
{
	makeTone();
	if ( stageCount == 0 )
		stageCount = 10;
	else
		if ( ++stageCount == 15 ) nextStage = &stage0;
}

void stage5( )
{
	makeTone();

	// Refresh frequency is too low for brightness control
	if ( Armadino::currentMatrix != NULL	)
		if ( currentCol == 0 )
			currentColPtr = (byte *) Armadino::currentMatrix->pixArray;
		else
			currentColPtr += MAX_ROW;
	bits = 0;
	if ( currentColPtr != NULL && screenEnabled ) {
		ptr = currentColPtr + MAX_ROW - 1;	// Work from bottom row up
		nextStage = &stage6;	 // Fill the LED matrix column
	}
	else
		nextStage = &stage6b;  // Branch to blank the matrix column (stages 6b-9b)
}

void stage6b( )
{
	makeTone();
	SPCR = (1 << SPE) | (1 << MSTR);	// Enable SPI, MSB first, Master
	SPDR = 0;
	nextStage = &stage7b;
}

void stage7b( )
{
	makeTone();
	SPDR = 0;
	nextStage = &stage8b;
}

void stage8b( )
{
	makeTone();
	if ( stageCount == 0 )
		stageCount = 8;
	else
		if ( ++stageCount == 14 ) nextStage = &stage15;
}

void stage6( )
{
	makeTone();
	// Red LED
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 128;  // No brightness control
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 64;
	nextStage = &stage7;
}

void stage7( )
{
	makeTone();
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 32;
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 16;
	nextStage = &stage8;
}

void stage8( )
{
	makeTone();
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 8;
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 4;
	nextStage = &stage9;
}

void stage9( )
{
	makeTone();
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 2;
	p = *ptr;
	if ( !(p&blinkMask) && (p&0x07) ) bits |= 1;
	nextStage = &stage10;
}

void stage10( )
{
	makeTone();
	SPCR = (1 << SPE) | (1 << MSTR);	// Enable SPI, MSB first, Master
	SPDR = bits;
	bits = 0;
	if ( currentColPtr != NULL )
		ptr = currentColPtr + MAX_ROW - 1;	// Work from bottom row up
	nextStage = &stage11;
}

void stage11( )
{
	makeTone();
	// Green LED
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 128;  // No brightness control
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 64;
	nextStage = &stage12;
}

void stage12( )
{
	makeTone();
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 32;
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 16;
	nextStage = &stage13;
}

void stage13( )
{
	makeTone();
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 8;
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 4;
	nextStage = &stage14;
}

void stage14( )
{
	makeTone();
	p = *ptr--;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 2;
	p = *ptr;
	if ( !(p&blinkMask) && (p&0x70) ) bits |= 1;
	SPDR = bits;
	nextStage = &stage15;
}

void stage15( )
{
	makeTone();
	SPCR = 0; 						// Turn off SPI
	if ( currentCol == 0 ) {
		reset = 1;
		PORTB |= B00010001; // Strobe Latch Enable on shift register and reset 4017
	}
	else {
		PORTB &= B11111110;
		PORTB |= B00010000; // Strobe Latch Enable on shift register and 4017 clock
	}
	PORTB &= B11101111;
	nextStage = &stage0;
}
