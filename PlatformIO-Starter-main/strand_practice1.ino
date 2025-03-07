#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>  // Required for 16 MHz Adafruit Trinket
#endif

// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1:
#define LED_PIN 8

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 60

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

struct RGBA {
  uint8_t r, g, b, a;
  void init(uint8_t rd, uint8_t gn, uint8_t bu, uint8_t alpha) {
    r = rd;
    g = gn;
    b = bu;
    a = alpha;
  }
  RGBA(uint8_t rd, uint8_t gn, uint8_t bu, uint8_t alpha)
    : r(rd), g(gn), b(bu), a(alpha) {}
  RGBA()
    : r(0), g(0), b(0), a(123) {}
};

class Light {
public:
  // "cast" an array of N RGBA to an array of N type Light
  // rColor0 and rLight0 are references to 1st element in the corresponding
  // statically allocated arrays in a size ratio of 1 RGBA per Light
  // The Light array is assumed to have adequate capacity
  static void Cast(const RGBA &rColor0, unsigned int numColors, Light &rLight0) {
    const RGBA *pColor = &rColor0;
    Light *pLt = &rLight0;

    for (unsigned int n = 0; n < numColors; ++n)
      *(pLt + n) = *(pColor + n);
  }

  uint32_t Light::pack() const {
    uint32_t temp = a;
    temp = temp << 8;
    temp += b;
    temp = temp << 8;
    temp += g;
    temp = temp << 8;
    temp += r;
    return temp;
  }

  uint8_t r = 1, g = 2, b = 3, a = 255;  // can test for default constructed instance

  void setState(uint8_t rd, uint8_t gn, uint8_t bu, uint8_t alpha) {
    r = rd;
    g = gn;
    b = bu;
    a = alpha;
  }
  Light(uint8_t rd, uint8_t gn, uint8_t bu, uint8_t alpha)
    : r(rd), g(gn), b(bu), a(alpha) {}

  // construct or assign from type RGBA
  Light(const RGBA &C) {
    r = C.r;
    g = C.g;
    b = C.b;
    a = C.a;
  }
  void operator=(const RGBA &Color) {
    r = Color.r;
    g = Color.g;
    b = Color.b;
    a = Color.a;
  }

  Light::Light(uint32_t C32) {
    // parse to r, g, b, a
    r = (C32)&0xFF;
    g = (C32 >> 8) & 0xFF;
    b = (C32 >> 16) & 0xFF;
    a = (C32 >> 24) & 0xFF;
  }

  Light() {}
  ~Light() {}

protected:

private:
};

RGBA red{ 255, 0, 0, 255 };
RGBA green{ 0, 255, 0, 255 };
RGBA blue{ 0, 0, 255, 255 };


void setup() {
  // put your setup code here, to run once:
  strip.begin();            // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();             // Turn OFF all pixels ASAP
  strip.setBrightness(50);  // Set BRIGHTNESS to about 1/5 (max = 255)
}

void loop() {
  // put your main code here, to run repeatedly:
  // redPulse(200);
  // pulseBySpeed(100, 25);
  pulseSlowingDown(5, 25, red);
  pulseSlowingDown(5, 25, green);
  pulseSlowingDown(5, 25, blue);
}

double get_sine_0_1(double input) {
  double out = sin(input);
  out += 1.0;
  out /= 2.0;
  return out;
}

void pulseSlowingDown(int minDelay, int maxDelay, RGBA color) {
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
}

// sinusodal speed
void pulseBySpeed(int wait, int speed) {

  for (int i = 0; i < strip.numPixels(); ++i) {
    strip.clear();
    // int lightIndex = (int)floor(get_sine_0_1(double(i / 2.0)) * strip.numPixels());
    // int nextDelay = (int)floor(get_sine_0_1(double(i / 2.0)) * speed);
    int nextDelay = 50 + get_sine_0_1(double(i / 60.0)) * 100;
    strip.setPixelColor(i, 255, 0, 0);
    strip.show();
    delay(nextDelay);
  }

  delay(wait);
}

// Make a red pulse
uint16_t current_pulse_index = 0;
void redPulse(int wait) {

  for (int i = 0; i < strip.numPixels(); ++i) {
    strip.clear();
    int lightIndex = (int)floor(get_sine_0_1(double(i / 2.0)) * strip.numPixels());
    strip.setPixelColor(lightIndex, 255, 0, 0);
    strip.show();
    delay(wait);
  }

  delay(wait);
}
