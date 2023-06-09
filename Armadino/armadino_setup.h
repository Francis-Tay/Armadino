/*
	armadino_setup.h - Settings for the Armadino
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

#ifndef ARMADINO_SETUP_H
#define ARMADINO_SETUP_H

#define BLINK_DELAY 			 250 // 255 or less milliseconds
#define DEBOUNCE_TIME 			32 // Debounce time for buttons, in milliseconds

// #define SOFTWARE_TONE
/* Generate tone by software, instead of having Timer1 flips OC1B. Volumn
	 control is not possible, and tone range should be restricted to C3 to B6.
	 This feature is useful if you are calling Arduino PWM function analogWrite()
	 on D9/PB1; or if you intend to use Timer1 for another purpose.
	 This feature is active when TVOut is enabled. */

#define INCLUDE_TVOUT
/* Includes Myles Metzer's TVout library. To install the TVout library:
	 a) In the Arduino software, select Tools... Manage Lbraries...
	 b) Search for TVout and install it. By default, this will be in a folder
	    named '<Arduino folder for user programs>/libraries/TVout'.
	 c) There seems to be a bit of a mix-up in the TVout folder structure, so
	    - Remove the extra TVout folder that is found in TVout folder!
	    - Move the 'TVoutfonts' folder one level up to 'libraries'

!!NOTE!! Main program should first call Armadino::begin(), then TVout::begin(),
				 and set TVout::set_hbi_hook() to call Armadino::ledRefresh(). See
				 programs in the examples folder.

	In TVOut mode, LED matrix refresh and tone generation are not interrupt-driven
	by Timer1 and Timer2. There are thus side effects:
		a) LED matrix refresh rate is 90.9 Hz, i.e. 1000 Hz / (10 columns + 1 7-Seg)
		b) The pulse on A0 is thus also 90.9 Hz, instead of 909Hz
		c) No brightness control, so it's red, green or orange (red+green)
		d) #define SOFTWARE_TONE is active.
		e) Do not call TVout::tone() as it uses Timer2 to toggle PB3 (OC2). On the
			 Armadino, the buzzer is on PB2 (OC1B). Use Armadino::tone() instead.

	Modifications required in the TVout library
	a) In hardware_setup.h,
		 - comment out #define ENABLE_FAST_OUTPUT
		 - for defined(__AVR_ATmega8__) ... defined(__AVR_ATmega328__),
		   change the video pin from PORTB-0 to PORTC-1:
					#define	PORT_VID	PORTC
					#define	DDR_VID 	DDRC
					#define	VID_PIN 	1
		   Technically, the video pin may be any of the PORTC pins.
	b) In video_gen.cpp, modify vsync_line() such that
				 if (remainingToneVsyncs != 0)
				 {
					 if (--remainingToneVsyncs == 0)
					 {
						 TCCR2B = 0; // stop the tone
						 PORTB &= ~(_BV(SND_PIN));
					 }
				 }
		 to avoid resetting PB3 unwittingly. In any case, use Armadino::tone()
		 instead of TVout::tone() (see above).
	c) On some TVs, it may be helpful to change _NTSC_TIME_SCANLINE from 63.55
		 to 64 in video_properties.h

	Connecting the Armadino to the TV

								 1k ohm
		 PB1/D9 -----^^^^^^-----+ 					(RCA Plug)
		 (sync) 								|
														+-----------Video (centre)
								470 ohm 		|
		 PC1/A1 -----^^^^^^-----+ 		+-----GND
		 (video)											|
																	|
		 GND		----------------------+
*/
#endif
