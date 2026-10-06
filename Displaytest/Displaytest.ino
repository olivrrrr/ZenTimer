#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Wire.h>

// ============================================================
// ZenTimer LCD + Touch Test
//
// Waveshare 1.69" Touch LCD
// ST7789V2 240x280
// CST816S Touch
//
// XIAO nRF52840 Sense
// ============================================================

// LCD
constexpr uint8_t LCD_DIN = 10;  // D10
constexpr uint8_t LCD_CLK = 8;   // D8
constexpr uint8_t LCD_CS  = 9;   // D9
constexpr uint8_t LCD_DC  = 7;   // D7
constexpr uint8_t LCD_RST = 3;   // D3
constexpr uint8_t LCD_BL  = 6;   // D6

// TOUCH
constexpr uint8_t TP_IRQ = 0;    // D0
constexpr uint8_t TP_RST = 1;    // D1

// D4 = SDA
// D5 = SCL

constexpr uint8_t CST816_ADDR = 0x15;

// Display
constexpr int LCD_WIDTH  = 240;
constexpr int LCD_HEIGHT = 280;
constexpr int Y_OFFSET   = 20;

// RGB565
constexpr uint16_t BLACK  = 0x0000;
constexpr uint16_t WHITE  = 0xFFFF;
constexpr uint16_t RED    = 0xF800;
constexpr uint16_t GREEN  = 0x07E0;
constexpr uint16_t ORANGE = 0xFC60;
constexpr uint16_t GREY   = 0x3186;

volatile bool touchIRQ = false;


// ============================================================
// SOFTWARE SPI
// ============================================================

void spiWrite(uint8_t value)
{
  for (int bit = 7; bit >= 0; bit--) {

    digitalWrite(LCD_CLK, LOW);

    digitalWrite(
      LCD_DIN,
      (value & (1 << bit)) ? HIGH : LOW
    );

    delayMicroseconds(1);

    digitalWrite(LCD_CLK, HIGH);

    delayMicroseconds(1);
  }

  digitalWrite(LCD_CLK, LOW);
}


// ============================================================
// LCD COMMAND
// ============================================================

void lcdCommand(uint8_t command)
{
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, LOW);

  spiWrite(command);

  digitalWrite(LCD_CS, HIGH);
}


// ============================================================
// LCD DATA
// ============================================================

void lcdData(uint8_t data)
{
  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, HIGH);

  spiWrite(data);

  digitalWrite(LCD_CS, HIGH);
}


// ============================================================
// LCD WINDOW
// ============================================================

void lcdSetWindow(
  uint16_t x0,
  uint16_t y0,
  uint16_t x1,
  uint16_t y1
)
{
  y0 += Y_OFFSET;
  y1 += Y_OFFSET;

  // Column address
  lcdCommand(0x2A);

  lcdData(x0 >> 8);
  lcdData(x0 & 0xFF);

  lcdData(x1 >> 8);
  lcdData(x1 & 0xFF);

  // Row address
  lcdCommand(0x2B);

  lcdData(y0 >> 8);
  lcdData(y0 & 0xFF);

  lcdData(y1 >> 8);
  lcdData(y1 & 0xFF);

  // RAM write
  lcdCommand(0x2C);
}


// ============================================================
// RECTANGLE
// ============================================================

void fillRect(
  int x,
  int y,
  int w,
  int h,
  uint16_t color
)
{
  if (x < 0) {
    w += x;
    x = 0;
  }

  if (y < 0) {
    h += y;
    y = 0;
  }

  if (x + w > LCD_WIDTH)
    w = LCD_WIDTH - x;

  if (y + h > LCD_HEIGHT)
    h = LCD_HEIGHT - y;

  if (w <= 0 || h <= 0)
    return;

  lcdSetWindow(
    x,
    y,
    x + w - 1,
    y + h - 1
  );

  uint8_t hi = color >> 8;
  uint8_t lo = color & 0xFF;

  uint32_t pixels =
    (uint32_t)w * (uint32_t)h;

  digitalWrite(LCD_CS, LOW);
  digitalWrite(LCD_DC, HIGH);

  while (pixels--) {
    spiWrite(hi);
    spiWrite(lo);
  }

  digitalWrite(LCD_CS, HIGH);
}


void fillScreen(uint16_t color)
{
  fillRect(
    0,
    0,
    LCD_WIDTH,
    LCD_HEIGHT,
    color
  );
}


// ============================================================
// LCD INIT
//
// Absichtlich dieselbe minimale Initialisierung,
// mit der dein Farbtest funktioniert hat.
// ============================================================

void lcdInit()
{
  digitalWrite(LCD_RST, HIGH);
  delay(50);

  digitalWrite(LCD_RST, LOW);
  delay(100);

  digitalWrite(LCD_RST, HIGH);
  delay(150);

  // Software reset
  lcdCommand(0x01);
  delay(150);

  // Sleep out
  lcdCommand(0x11);
  delay(150);

  // RGB565
  lcdCommand(0x3A);
  lcdData(0x55);

  // Orientation
  lcdCommand(0x36);
  lcdData(0x00);

  // Normal mode
  lcdCommand(0x13);
  delay(10);

  // Inversion ON
  lcdCommand(0x21);
  delay(10);

  // Display ON
  lcdCommand(0x29);
  delay(100);
}


// ============================================================
// TOUCH
// ============================================================

void touchISR()
{
  touchIRQ = true;
}


void resetTouch()
{
  digitalWrite(TP_RST, HIGH);
  delay(10);

  digitalWrite(TP_RST, LOW);
  delay(20);

  digitalWrite(TP_RST, HIGH);
  delay(100);
}


bool touchPresent()
{
  Wire.beginTransmission(CST816_ADDR);

  return Wire.endTransmission() == 0;
}


// ============================================================
// CST816S TOUCH READ
// ============================================================

bool readTouch(
  uint16_t &x,
  uint16_t &y,
  uint8_t &points
)
{
  Wire.beginTransmission(CST816_ADDR);

  // Finger number starts at 0x02
  Wire.write(0x02);

  if (Wire.endTransmission(false) != 0)
    return false;

  // points, XH, XL, YH, YL
  if (Wire.requestFrom(
        CST816_ADDR,
        (uint8_t)5
      ) != 5) {

    return false;
  }

  uint8_t data[5];

  for (int i = 0; i < 5; i++)
    data[i] = Wire.read();

  points = data[0] & 0x0F;

  x =
    ((uint16_t)(data[1] & 0x0F) << 8)
    |
    data[2];

  y =
    ((uint16_t)(data[3] & 0x0F) << 8)
    |
    data[4];

  return true;
}


// ============================================================
// MARKER
// ============================================================

int lastX = -1;
int lastY = -1;


void eraseMarker()
{
  if (lastX < 0 || lastY < 0)
    return;

  // horizontal
  fillRect(
    lastX - 10,
    lastY - 2,
    21,
    5,
    BLACK
  );

  // vertical
  fillRect(
    lastX - 2,
    lastY - 10,
    5,
    21,
    BLACK
  );
}


void drawMarker(int x, int y)
{
  // Horizontal
  fillRect(
    x - 10,
    y - 2,
    21,
    5,
    ORANGE
  );

  // Vertical
  fillRect(
    x - 2,
    y - 10,
    5,
    21,
    ORANGE
  );

  // Mittelpunkt
  fillRect(
    x - 3,
    y - 3,
    7,
    7,
    WHITE
  );

  lastX = x;
  lastY = y;
}


// ============================================================
// Optional coordinate transformation
//
// Im ersten Versuch 1:1.
// Falls Touch gespiegelt/gedreht ist,
// ändern wir NUR diese Funktion.
// ============================================================

void transformTouch(
  uint16_t rawX,
  uint16_t rawY,
  int &screenX,
  int &screenY
)
{
  screenX = rawX;
  screenY = rawY;

  screenX = constrain(
    screenX,
    0,
    LCD_WIDTH - 1
  );

  screenY = constrain(
    screenY,
    0,
    LCD_HEIGHT - 1
  );
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  // ----------------------------------------------------------
  // LCD Pins
  // ----------------------------------------------------------

  pinMode(LCD_DIN, OUTPUT);
  pinMode(LCD_CLK, OUTPUT);
  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_RST, OUTPUT);
  pinMode(LCD_BL, OUTPUT);

  digitalWrite(LCD_CLK, LOW);
  digitalWrite(LCD_DIN, LOW);
  digitalWrite(LCD_CS, HIGH);

  // Backlight zunächst aus
  digitalWrite(LCD_BL, LOW);

  // ----------------------------------------------------------
  // Touch Pins
  // ----------------------------------------------------------

  pinMode(TP_RST, OUTPUT);
  pinMode(TP_IRQ, INPUT_PULLUP);

  digitalWrite(TP_RST, HIGH);

  // ----------------------------------------------------------
  // Display
  // ----------------------------------------------------------

  lcdInit();

  fillScreen(BLACK);

  // etwas Rahmen als Orientierung
  fillRect(0, 0, 240, 2, GREY);
  fillRect(0, 278, 240, 2, GREY);
  fillRect(0, 0, 2, 280, GREY);
  fillRect(238, 0, 2, 280, GREY);

  digitalWrite(LCD_BL, HIGH);

  // ----------------------------------------------------------
  // I2C / Touch
  // ----------------------------------------------------------

  Wire.begin();
  Wire.setClock(400000);

  resetTouch();

  delay(100);

  if (touchPresent()) {

    // Grün = Touchcontroller gefunden
    fillRect(
      10,
      10,
      220,
      8,
      GREEN
    );

  } else {

    // Rot = Touchcontroller NICHT gefunden
    fillRect(
      10,
      10,
      220,
      8,
      RED
    );

    // hier stehen bleiben
    while (true) {
      delay(1000);
    }
  }

  // ----------------------------------------------------------
  // IRQ
  // ----------------------------------------------------------

  attachInterrupt(
    digitalPinToInterrupt(TP_IRQ),
    touchISR,
    FALLING
  );
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // IRQ oder aktuell LOW
  if (!touchIRQ &&
      digitalRead(TP_IRQ) == HIGH) {

    delay(5);
    return;
  }

  touchIRQ = false;

  uint16_t rawX = 0;
  uint16_t rawY = 0;

  uint8_t points = 0;

  if (!readTouch(
        rawX,
        rawY,
        points
      )) {

    return;
  }

  if (points == 0)
    return;

  int x;
  int y;

  transformTouch(
    rawX,
    rawY,
    x,
    y
  );

  eraseMarker();

  drawMarker(
    x,
    y
  );

  delay(15);
}