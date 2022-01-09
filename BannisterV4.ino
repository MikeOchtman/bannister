#include "Arduino.h"
#include "FastLED.h"

/**
   @brief Control bannister lights

   A string of WS2812B LEDs run along a stair bannister inside an aluminium profile
   Microwave motion sensors at the top and bottom of the stairs detect motion
   A LDR reports current ambient light intensity

   During daytime, or when light levels are high, motion will trigger display of
   an animated rainbow down the length of the stairs for some time.

   During the night, motion will trigger a sequence to illuminate the stairs safely
   for a reasonable time.

*/

#define PROJECT "Bannister V4"

// #define DEBUG
#ifdef DEBUG
#define debugBegin      \
  Serial.begin(115200); \
  Serial.println(PROJECT);
#define debugWrite(x) Serial.print((x));
#define debugWriteln(x) Serial.println((x));
#else
#define debugBegin
#define debugWrite(x)
#define debugWriteln(x)
#endif

const uint8_t NUM_LEDS = 113; // leds in the addressible string
const uint8_t PIN_UPPER_SENSOR = 7;
const uint8_t PIN_LOWER_SENSOR = 6;
const uint8_t PIN_AMBIENT = A0;
const uint8_t PIN_LEDDATA = 2;
const uint16_t MAX_POWER = 10000; // 10W allowed at 5V = 2A for all the LEDs
const uint16_t THRESHOLD_NIGHT = 100;
const uint16_t THRESHOLD_MORNING = 300;
const uint16_t THRESHOLD_DAY = 500;
const uint16_t THRESHOLD_EVENING = 300;

CRGB leds[NUM_LEDS];
enum class LedMode
{
  NIGHT,
  MORNING,
  DAY,
  EVENING
};
LedMode mode;

/* Function Prototypes */
LedMode setLedMode(uint16_t lightLevel);
void doDayRoutine();
void doMorningRoutine();
void doNightRoutine();
void doEveningRoutine();
void doRainbow(uint8_t);
void lightsFromTop();
void lightsFromBottom();
void doPink(uint8_t);
/* start of program */
void setup()
{
  debugBegin;

  pinMode(PIN_UPPER_SENSOR, INPUT);
  pinMode(PIN_LOWER_SENSOR, INPUT);
  pinMode(PIN_AMBIENT, INPUT);
  pinMode(LED_BUILTIN, OUTPUT); // probably unnecessary

  uint16_t ambientLight = analogRead(PIN_AMBIENT);
  debugWrite("Light is ");
  debugWriteln(ambientLight);

  mode = LedMode::NIGHT;
  FastLED.addLeds<WS2812B, PIN_LEDDATA, GRB>(leds, NUM_LEDS);
  FastLED.setMaxPowerInMilliWatts(MAX_POWER);

  // clear the LEDs array and write to the LEDs to black out the strip
  FastLED.clear(true);
  debugWriteln("Booting up...");
  // doRainbow();
  debugWriteln("Ready...");
}

void loop()
{
  uint16_t ambientLight = analogRead(PIN_AMBIENT);
  mode = setLedMode(ambientLight);

  switch (mode)
  {
  case LedMode::MORNING:
  case LedMode::DAY:
    doDayRoutine();
    break;

  case LedMode::NIGHT:
    doNightRoutine();
    break;

  case LedMode::EVENING:
    doEveningRoutine();
    break;

  default:
    mode = LedMode::NIGHT;
  }
}
/****************************************************************************

 **************************************************************************/

/**
   @brief Set the Led Mode

   @param lightLevel 0..1024 value for ambient light level.
   @return LedMode
*/
LedMode setLedMode(uint16_t lightLevel)
{

  debugWrite("Mode ");
  debugWrite((int)mode);
  debugWrite("; Light Level ");
  debugWriteln(lightLevel);

  switch (mode)
  {
  case LedMode::NIGHT:
    // the only way out from night is morning
    if (lightLevel > THRESHOLD_MORNING)
    {
      debugWriteln("Switching to MORNING mode");
      mode = LedMode::MORNING;
    }
    break;

  case LedMode::MORNING:
    // if it becomes lighter, it must be morning.
    // it could be a very dark day, so it may become night again.
    if (lightLevel > THRESHOLD_DAY)
    {
      debugWriteln("Switching to DAY mode");
      mode = LedMode::DAY;
    }
    if (lightLevel < THRESHOLD_NIGHT)
    {
      debugWriteln("Switching back to NIGHT mode");
      mode = LedMode::NIGHT;
    }
    break;

  case LedMode::DAY:
    // the only way out from day is evening
    if (lightLevel < THRESHOLD_EVENING)
    {
      debugWriteln("Switching to EVENING mode");
      mode = LedMode::EVENING;
    }
    break;

  case LedMode::EVENING:
    // if it becomes darker, it must be night.
    // it may become lighter after a passing cloud, so it may be day again
    if (lightLevel < THRESHOLD_NIGHT)
    {
      debugWriteln("Switching to NIGHT mode");
      mode = LedMode::NIGHT;
    }
    if (lightLevel > THRESHOLD_DAY)
    {
      debugWriteln("Switching back to DAY mode");
      mode = LedMode::DAY;
    }
    break;
  default:
    mode = LedMode::NIGHT;
  }
  return mode;
}

/**
   @brief do what is necessary during daylight hours

   During daylight, there's no much need for light for vision
   so decorative effects are called for

*/
void doDayRoutine()
{

  uint8_t motionDetected = digitalRead(PIN_UPPER_SENSOR) || digitalRead(PIN_LOWER_SENSOR);

  if (motionDetected)
  {
    //doRainbow(250);
    doPink(80);
    delay(5000);
  }
}

/**
 * @brief perform morning activities for LEDs to herald a new day
 * 
 */
void doMorningRoutine()
{

  uint8_t motionDetected = digitalRead(PIN_UPPER_SENSOR) || digitalRead(PIN_LOWER_SENSOR);

  if (motionDetected)
  {
    doRainbow(250);
    //doPink(80);
    delay(5000);
  }
}

/**
   @brief perform night routine activities
   illuminate stairs
*/
void doNightRoutine()
{
  int motionUpperDetected = digitalRead(PIN_UPPER_SENSOR);
  int motionLowerDetected = digitalRead(PIN_LOWER_SENSOR);

  if (motionLowerDetected)
  {
    debugWriteln("Night routine: Bottom motion");
    lightsFromBottom();
    delay(2500);
  }

  if (motionUpperDetected)
  {
    debugWriteln("Night routine: Top motion");
    lightsFromTop();
    delay(2500);
  }
}

/**
   @brief do late afternoon ambient lighting

*/
void doEveningRoutine()
{
  //same as night routine
  doNightRoutine();
}

/**
   @brief displays an animated rainbow with colors advancing from top to bottom

   Each LED has a slightly different hue such that the entire gamut of hues
   are represented by each LED
   Over time, the hue ofeach LED cycles by one so that it appears as if the
   colors are moving down the stairs and the end color is cycling back todo the top of the stairs
*/
void doRainbow(uint8_t brightness)
{
  int offset = 0;                     // amount the hue is advanced for the animation effect
  uint8_t hueFactor = 255 / NUM_LEDS; // distance between each LED on the hue scale
  uint8_t color = 0;
  for (long counter = 0; counter < 2000; ++counter)
  {
    for (int led = 0; led < NUM_LEDS; ++led)
    {
      color = (led * hueFactor + offset) % 256;
      leds[led] = CHSV(color, 255, brightness);
    }
    FastLED.show();
    delay(50);
    if (++offset > 255)
      offset = 0;
  }
  // switch off the LEDs when done
  FastLED.clear(true);
}

/**
   @brief draws a sequence of lights brightening over time to illuminate steps for safety

   Starts from the top of the stairs and grows the illuminated LEDS
   from the top until all LEDs are equally lit with the same color

*/
void lightsFromTop()
{
  uint8_t color = 0;
  const uint8_t hueFactor = 255 / NUM_LEDS;

  // Fill the LEDs from the top of the stairs
  for (uint8_t length = 0; length < NUM_LEDS; ++length)
  {
    for (uint8_t led = 0; led < length; ++led)
    {
      color = led * hueFactor;
      leds[led] = CHSV(color, 255, 60);
    }
    FastLED.show();
    delay(10);
  }
  delay(40000);
  // then fade the LEDs out
  for (uint8_t brightness = 60; brightness > 0; --brightness)
  {
    for (uint8_t led = 0; led < NUM_LEDS; ++led)
    {
      color = led * hueFactor;
      leds[led] = CHSV(color, 255, brightness);
    }
    FastLED.show();
    delay(20);
  }

  FastLED.clear(true);
}

void lightsFromBottom()
{
  uint8_t color = 0;
  const uint8_t hueFactor = 255 / NUM_LEDS;

  for (uint8_t length = NUM_LEDS - 1; length > 0; --length)
  {
    for (uint8_t led = NUM_LEDS - 1; led > length; --led)
    {
      color = led * hueFactor;
      leds[led] = CHSV(color, 255, 60);
    }
    FastLED.show();
    delay(10);
  }
  delay(40000);

  // then fade the LEDs out
  for (uint8_t brightness = 60; brightness > 0; --brightness)
  {
    for (uint8_t led = 0; led < NUM_LEDS; ++led)
    {
      color = led * hueFactor;
      leds[led] = CHSV(color, 255, brightness);
    }
    FastLED.show();
    delay(20);
  }
  FastLED.clear(true);
}

/**
 * @brief set all the lights to fade in and out from pink
 * 
 * on motion detected, fade in from dark to a light pink/mauve
 * then stay on for a time, and fade back out to dark
 * 
 */
void doPink(uint8_t brightness)
{

  for (uint8_t level = 0; level < brightness; ++level)
  {
    for (uint8_t led = 0; led < NUM_LEDS; ++led)
    {
      leds[led] = CHSV(200, 255, level);
    }
    FastLED.show();
    delay(20);
  }
  delay(40000);
  for (uint8_t level = brightness - 1; level > 0; --level)
  {
    for (uint8_t led = 0; led < NUM_LEDS; ++led)
    {
      leds[led] = CHSV(200, 255, level);
    }
    FastLED.show();
    delay(20);
  }
  FastLED.clear(true);
}
