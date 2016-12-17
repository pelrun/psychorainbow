#include "Arduino.h"

#include "FastLED.h"
#include "EEPROM.h"

#define BUTTON1_PIN 10
#define BUTTON2_PIN 11

#define BACK_PIN 2
#define ARMS_PIN 3
#define LEGS_PIN 4
#define SPINE_PIN 5
#define TAIL_PIN 6
#define CHEST_PIN 7
#define EYES_PIN 8

#define NUM_LEDS_PER_STRIP 23

#define DEBOUNCE_LIMIT 5

#define FRAMERATE 50

CRGB leds[NUM_LEDS_PER_STRIP];
CRGB eye_leds[2];

uint8_t mode;
uint32_t frame;

uint16_t debounce[2];

typedef enum
{
	NONE,
	A_ONLY,
	B_ONLY,
	A_THEN_B,
	B_THEN_A
} Button_e;

//The setup function is called once at startup of the sketch
void setup()
{
	pinMode(BUTTON1_PIN, INPUT);
	digitalWrite(BUTTON1_PIN, HIGH);
	pinMode(BUTTON2_PIN, INPUT);
	digitalWrite(BUTTON2_PIN, HIGH);

	FastLED.addLeds<NEOPIXEL, EYES_PIN>(eye_leds, 2);
	FastLED.addLeds<NEOPIXEL, CHEST_PIN>(leds, NUM_LEDS_PER_STRIP);
	FastLED.addLeds<NEOPIXEL, ARMS_PIN>(leds, NUM_LEDS_PER_STRIP);
	FastLED.addLeds<NEOPIXEL, LEGS_PIN>(leds, NUM_LEDS_PER_STRIP);
	FastLED.addLeds<NEOPIXEL, SPINE_PIN>(leds, NUM_LEDS_PER_STRIP);
	FastLED.addLeds<NEOPIXEL, BACK_PIN>(leds, NUM_LEDS_PER_STRIP);
	FastLED.addLeds<NEOPIXEL, TAIL_PIN>(leds, NUM_LEDS_PER_STRIP);

	FastLED.setMaxPowerInVoltsAndMilliamps(5,2000);

	mode = EEPROM.read(0);
	frame = 0;
}

bool debounce_read(uint8_t pin, uint16_t &counter)
{
	if (!digitalRead(pin))
	{
		if (counter < DEBOUNCE_LIMIT)
		{
			counter++;
			return false;
		}
		if (counter >= DEBOUNCE_LIMIT)
		{
			return true;
		}
	}
	else
	{
		counter = 0;
	}
	return false;
}

void update_state()
{
	static Button_e state = NONE;

	bool buttonA = debounce_read(BUTTON1_PIN, debounce[0]);
	bool buttonB = debounce_read(BUTTON2_PIN, debounce[1]);

	if (!buttonA && !buttonB && state != NONE)
	{
		// use state to select next pattern
		mode = state-1;
		EEPROM.write(0,mode);
		state = NONE;
		return;
	}

	switch (state)
	{
	case NONE:
		if (buttonA)
		{
			state = A_ONLY;
		}
		else if (buttonB)
		{
			state = B_ONLY;
		}
		break;
	case A_ONLY:
		if (buttonB)
		{
			state = A_THEN_B;
		}
		break;
	case B_ONLY:
		if (buttonA)
		{
			state = B_THEN_A;
		}
		break;
	default:
		// A_THEN_B/B_THEN_A don't do anything except wait for buttons to be fully released
		break;
	}
}

//1. Psycho rainbow, flickering random bright rainbow colours Changing rapidly everywhere. Red eyes
void psychorainbow()
{
	// psycho rainbow 2
	random16_set_seed(0xB00B);
	for (uint8_t light=0; light<NUM_LEDS_PER_STRIP; light++)
	{
		leds[light] = CHSV(random8()+light*37-frame*(5.2+random8()/32),255,255);
	}
	eye_leds[0] = eye_leds[1] = CRGB::Red;
}

// Hues:
// 0:Red 32:Orange 64:Yellow 96:Green 128:Aqua 160:Blue 192:Purple 224:Pink

//2. Slow pulse, gentle fade in and out of pale colour range violet/pink (160-224) black everywhere... aqua eyes
void breathe()
{
	CRGB colour = CHSV(200,96,255);
	colour %= (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;

	fill_solid(leds, NUM_LEDS_PER_STRIP, colour);
	eye_leds[0] = eye_leds[1] = CRGB::Aqua;
}

void off()
{
	fill_solid(leds, NUM_LEDS_PER_STRIP, CRGB::Black);
	eye_leds[0] = eye_leds[1] = CRGB::Black;
}

void solidgreen()
{
	fill_solid(leds, NUM_LEDS_PER_STRIP, CRGB::YellowGreen);
	eye_leds[0] = eye_leds[1] = CRGB::MediumVioletRed;
}

//4. Pulse with aqua to violet (128-192) section of  colour  wheel .  Pale Yellow eyes
// frame increments 50 each second
// Fast brightness cycle (5s?) 255 count -> frame&0xFF
// Slow colour cycle (30s?) 1500 count -> (frame/6)&0xFF
void aquapulse()
{
	CRGB colour = blend(CRGB::Aqua, CRGB::Violet, quadwave8(frame/6)); // sine wave between two colours
	colour %= quadwave8(frame); // sine wave brightness changes

	fill_solid(leds,NUM_LEDS_PER_STRIP, colour);
	eye_leds[0] = eye_leds[1] = CRGB::LightGoldenrodYellow;
}

// put all the pattern functions in order here:
void (*patterns[])() = {psychorainbow, aquapulse, solidgreen, off};

// The loop function is called in an endless loop
void loop()
{
	update_state();

	frame++;

	if (mode < sizeof(patterns)/sizeof(patterns[0]))
	{
		patterns[mode]();
	}
	else
	{
		mode = 0;
	}

	FastLED.show();
	FastLED.delay(1000/FRAMERATE);
}
