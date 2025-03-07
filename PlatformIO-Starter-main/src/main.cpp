#include <FastLED.h>
#include <stdint.h>

#define LED_PIN     8
#define NUM_LEDS    30
#define BRIGHTNESS  50
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#include <Adafruit_NeoPixel.h>

void wait_for_serial_connection()
{
  uint32_t timeout_end = millis() + 2000;
  Serial.begin(115200);
  while (!Serial && timeout_end > millis()) {}  //wait until the connection to the PC is established
}

/*
  for (int x = 0; x < 1; x++) {
    strip.clear();
    for (int i = 0; i < strip.numPixels(); ++i) {
      // strip.clear();
      // int lightIndex = (int)floor(get_sine_0_1(double(i / 2.0)) * strip.numPixels());
      // int nextDelay = (int)floor(get_sine_0_1(double(i / 2.0)) * speed);
      int nextDelay = minDelay + int((maxDelay - minDelay) * ((double)(i) / strip.numPixels()));
      strip.setPixelColor(i, color.r, color.g, color.b);
      strip.show();
      if (i < strip.numPixels() - 1) {
        delay(nextDelay);
      } else {
        delay(1);
      }
    }
  }
  */

constexpr uint8_t COLOR_MAX = uint8_t(255);

CRGB colors[] = {
  CRGB(COLOR_MAX, 0,          0), // red
  CRGB(COLOR_MAX, COLOR_MAX,  0),
  CRGB(0, COLOR_MAX, 0), // green
    // CRGB(COLOR_MAX, 0,          0), // red
  CRGB(0, COLOR_MAX, COLOR_MAX),
  CRGB(0, 0, COLOR_MAX), // blue,
  CRGB(COLOR_MAX, 0, COLOR_MAX)
  // CRGB(COLOR_MAX, COLOR_MAX,  COLOR_MAX)
};

// CRGB colors[] = {
//   CRGB(COLOR_MAX, 0,          0), // red
//   CRGB(COLOR_MAX, COLOR_MAX,  0),
//   CRGB(0, COLOR_MAX, 0), // green
//   CRGB(0, COLOR_MAX, COLOR_MAX),
//   CRGB(0, 0, COLOR_MAX), // blue,
//   CRGB(COLOR_MAX, COLOR_MAX,  COLOR_MAX),
//   // CRGB(0,         COLOR_MAX,  COLOR_MAX),
//   // CRGB(0,         0,          COLOR_MAX),
//   // CRGB(0,         0,          0)
// };
// const int numColors = sizeof(colors) / sizeof(colors[0]);

// int blendSteps = 10;
// int currentBlendStep = 0;
// int currentColorIndex = 0;

// fract8 currBlend = 1;
// CRGB currentFrameColor = colors[0];

CRGB currentBlendedColor = colors[0];
constexpr auto s = sizeof(CRGB);
int targetColorIndex = 1;
int currStep = 0;
fract8 currLerpFrac = 255;
int color_dir = 1;

void targetNextColor() {
  targetColorIndex = (targetColorIndex + 1) % 6;
}

CRGB getColorForStep()
{
  CRGB targetColor = colors[targetColorIndex];
  CRGB nextColor = CRGB::blend(currentBlendedColor, targetColor, currLerpFrac);
  currLerpFrac += color_dir;

  if (color_dir == 1 && currLerpFrac == 255)
  {
    color_dir = -1;
    targetNextColor();
  }
  else if (color_dir == -1 && currLerpFrac == 0)
  {
    color_dir = 1;
    targetNextColor();
  }

  // if (currLerpFrac == 0) {
  //   currLerpFrac = 255;
  //   targetNextColor();
  // }


  // // Get the current color and the next color
  // CRGB targetColor = colors[targetColorIndex];
  // CRGB diff = targetColor - currentBlendedColor;
  
  // if (currentBlendedColor.r != targetColor.r)
  // {
  //   currentBlendedColor.r += max(uint8_t(5), diff.r);

  // }
  // if (currentBlendedColor.g != targetColor.g)
  // {
  //   currentBlendedColor.g += max(uint8_t(5), diff.g);

  // }
  // if (currentBlendedColor.b != targetColor.b)
  // {
  //   currentBlendedColor.b += max(uint8_t(5), diff.b);

  // }
  
  // if (currentBlendedColor == targetColor) {

  //   currentBlendedColor = targetColor;
  //   targetColorIndex = (targetColorIndex + 1) % 6;
  // }
  
  return nextColor;
}

void pulseSegment(int startIndex, int endIndex) {
  CRGB thisColor = getColorForStep();
  // for (int curr = 0; curr < index; ++curr)
  // {
  for (int i = startIndex; i < endIndex && i < NUM_LEDS; ++i)
  {
    leds[i] = thisColor;
  }
  for (int i = 0; i < startIndex; ++i) {
    leds[i] = CRGB::Black;
  }
  for (int i = endIndex; i < NUM_LEDS; ++i)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  delay(5);
}

class Pulser {
  enum _state : uint16_t {
    // First bit is for direction
    direction_forward = 0x01,
    direction_reverse = 0x00,
  };

  bool m_isForward = true;
  uint16_t m_currentIndex = 0;
  uint16_t m_minIndex = 0;
  uint16_t m_maxIndex = NUM_LEDS;
  CRGB m_currentColor;
public:

  // Update own internal state
  void Update() {
    m_currentIndex += m_isForward ? 1 : -1;
    if (m_isForward && m_currentIndex >= m_maxIndex - 1) {
      m_isForward = false;
    } else if (!m_isForward && m_currentIndex <= m_minIndex) {
      m_isForward = true;
    }
    m_currentColor = getColorForStep();
  }

  // Write state to LEDs
  void Show() {
    for (uint16_t i = m_minIndex; i < m_currentIndex && i < m_maxIndex; ++i) {
      leds[i] = m_currentColor;
    }
    for (uint16_t i = m_currentIndex; i < m_maxIndex; ++i) {
      leds[i] = CRGB::Black;
    }
  }

  void Start() {
    m_isForward = true;
    m_currentIndex = m_minIndex;
  }

  void Init(uint16_t minIdx, uint16_t maxIdx) {
    m_maxIndex = maxIdx;
    m_minIndex = minIdx;
  }
};

void pulseFillUpTo(int index)
{
  CRGB thisColor = getColorForStep();
  // for (int curr = 0; curr < index; ++curr)
  // {
  for (int i = 0; i < index && i < NUM_LEDS; ++i)
  {
    leds[i] = thisColor;
  }
  for (int i = index; i < NUM_LEDS; ++i)
  {
    leds[i] = CRGB::Black;
  }
  FastLED.show();
  delay(5);
  // }
  // currentBlendStep++;
  // if (currentBlendStep == 20) {
  //   currentBlendStep = 0;
  //   currentColorIndex++;
  // }
  // currentColorIndex = currentColorIndex % 3;
}

Pulser myPulser;

void setup()
{
  wait_for_serial_connection(); // Optional, but seems to help Teensy out a lot.
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  myPulser.Init(15, 30);
  myPulser.Start();
}

int currentFillUpTo = 1;
int dir = 1;

void strobeColors() {
  CRGB color = getColorForStep();
  FastLED.showColor(color);
  delay(5);
}


/*
  for (int x = 0; x < 1; x++) {
    strip.clear();
    for (int i = 0; i < strip.numPixels(); ++i) {
      // strip.clear();
      // int lightIndex = (int)floor(get_sine_0_1(double(i / 2.0)) * strip.numPixels());
      // int nextDelay = (int)floor(get_sine_0_1(double(i / 2.0)) * speed);
      int nextDelay = minDelay + int((maxDelay - minDelay) * ((double)(i) / strip.numPixels()));
      strip.setPixelColor(i, color.r, color.g, color.b);
      strip.show();
      if (i < strip.numPixels() - 1) {
        delay(nextDelay);
      } else {
        delay(1);
      }
    }
  }
  */

int maxDelay = 25;
int minDelay = 5;
 int getNextDelay(int i) {
   int nextDelay = minDelay + int((maxDelay - minDelay) * ((double) (i) / NUM_LEDS));
   return nextDelay * 10;
 }

void loop()
{
  FastLED.clear();
  // leds[0] = CRGB::Red;
  // FastLED.show();
  // delay(1000);
  // leds[0] = CRGB::Black;
  // FastLED.show();
  // delay(1000);
  // strobeColors();
  // pulseFillUpTo(currentFillUpTo);
  // currentFillUpTo += dir;

  // if (dir == 1 && currentFillUpTo >= NUM_LEDS - 1)
  // {
  //   dir = -1;
  // }
  // else if (dir == -1 && currentFillUpTo <= 1)
  // {
  //   dir = 1;
  // }
  FastLED.clear();
  myPulser.Update();
  myPulser.Show();
  FastLED.show();
  int nextDelay = getNextDelay(currentFillUpTo);
  delay(nextDelay);
}
