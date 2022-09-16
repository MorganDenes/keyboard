/*
       LEFT
      LAYOUT TO PINOUT
      06 07 08 09 10 11
     +-----------------+
  14 |17 18       21 20|       PHYSICAL LAYOUT
  15 |11 12 16 15 14 13|      01 02 03 04 05 06
  20 |05 06 10 09 08 07|      07 08 09 10 11 12
  21 |      04 03 02 01|      13 14 15 16 17 18    19
  22 |24 19 23 22      |            20 21    22 23 24
     +-----------------+

       RIGHT
      LAYOUT TO PINOUT
      00 01 02 03 04 05
     +-----------------+
  11 |      03 04 05 06|             PHYSICAL LAYOUT
  12 |01 02 09 10 11 12|            01 02 03 04 05 06
  13 |14 15       23 24|            07 08 09 10 11 12
  14 |07 08 16 17 18 19|      13    14 15 16 17 18 19
  15 |13 20 21 22      |      20 21 22    23 24
     +-----------------+
*/

#include "PCF8575.h"

PCF8575 PCF(0x20, &Wire1);

const uint8_t rowCount = 6;
const uint8_t colCount = 5;

uint8_t leftColPins[colCount] { 14, 15, 20, 21, 22 };
uint8_t leftRowPins[rowCount] { 6, 7, 8, 9, 10, 11 };
uint8_t rightColPins[colCount] { 11, 12, 13, 14, 15 };
uint8_t rightRowPins[rowCount] { 0, 1, 2, 3, 4, 5 };

uint8_t conversionsLeft[colCount][rowCount] {
  { 16, 17, 99, 99, 20, 19 },
  { 10, 11, 15, 14, 13, 12 },
  {  4,  5,  9,  8,  7,  6 },
  { 99, 99,  3,  2,  1,  0 },
  { 23, 18, 22, 21, 99, 99 }
};

uint8_t conversionsRight[colCount][rowCount] {
  { 99, 99,  2,  3,  4,  5 },
  {  0,  1,  8, 9, 10, 11 },
  { 13, 14, 99, 99, 22, 23 },
  {  6,  7, 15, 16, 17, 18 },
  { 12, 19, 20, 21, 99, 99 }
};

int LayerOne[colCount * rowCount * 2] {
// LEFT
//  |                 |              |                |          |      |          |          |
    KEY_TAB,          KEY_Q,         KEY_W,           KEY_E,     KEY_R, KEY_T,
    KEY_ESC,          KEY_A,         KEY_S,           KEY_D,     KEY_F, KEY_G,
    MODIFIERKEY_CTRL, KEY_BACKSLASH, KEY_Z,           KEY_X,     KEY_C, KEY_V,                MODIFIERKEY_SHIFT,
                                     MODIFIERKEY_ALT, KEY_TILDE,        KEY_MINUS, KEY_SPACE, /*LAYER TWO*/0,
// RIGHT
// |                           |                  |      |               |                |              |
                               KEY_Y,             KEY_U, KEY_I,          KEY_O,           KEY_P,         KEY_BACKSPACE,
                               KEY_H,             KEY_J, KEY_K,          KEY_L,           KEY_SEMICOLON, KEY_QUOTE,
   MODIFIERKEY_GUI,            KEY_B,             KEY_N, KEY_M,          KEY_PERIOD,      KEY_COMMA,     KEY_SLASH,
   KEY_EQUAL,       KEY_ENTER, MODIFIERKEY_SHIFT,        KEY_LEFT_BRACE, KEY_RIGHT_BRACE
};

int LayerTwo[colCount * rowCount * 2] {
// LEFT
//  |                 |  |                |         |          |           |  |
    0,                0, 0,               0,        KEY_COMMA, KEY_PERIOD,
    0,                0, KEY_LEFT,        KEY_UP,   KEY_RIGHT, 0,
    MODIFIERKEY_CTRL, 0, 0,               KEY_DOWN, 0,         0,             0,
                         MODIFIERKEY_ALT, 0,                   0,          0, /*NOOP*/0,
// RIGHT
// |                |  |                  |      |      |      |      |
                       0,                 KEY_7, KEY_8, KEY_9, KEY_0, 0,
                       0,                 KEY_4, KEY_5, KEY_6, 0,     0,
   MODIFIERKEY_GUI,    0,                 KEY_1, KEY_2, KEY_3, 0,     0,
   0,               0, MODIFIERKEY_SHIFT,        KEY_0, 0
};

void setup()
{
  // Left
  for (uint8_t r : leftRowPins)
  {
      pinMode(r, INPUT);
  }
  for (uint8_t c : leftColPins)
  {
      pinMode(c, INPUT);
  }
  // Right
  uint16_t writeMask = 0x00;
  for (uint8_t r : rightRowPins)
  {
      writeMask |= 0x01 << r;
  }
  PCF.begin();
  PCF.write16(writeMask);
}

short count;
int sendKeys[6];
int mod;
int layerKey = 23;
int layer;

int getKey(int index)
{
  if (layer == 0)
    return LayerOne[index];
  else
    return LayerTwo[index];
}

void loop()
{
  count = 0;
  mod = 0x0;
  layer = 0;

  // Left
  for (uint8_t c = 0; c< colCount; c++) {
    // col: set to output to low
    uint8_t colPin = leftColPins[c];
    pinMode(colPin, OUTPUT);
    digitalWrite(colPin, LOW);

    // row: interate through the rows
    for (uint8_t r = 0; r< rowCount; r++) {
      uint8_t rowPin = leftRowPins[r];
      pinMode(rowPin, INPUT_PULLUP);
      delay(1);
      if (digitalRead(rowPin) == 0)
      {
        uint8_t key = conversionsLeft[c][r];
        if (key == layerKey)
        {
          layer = 1;
        }
        else if (key == 19 || key == 12 || key == 18)
        {
          mod |= LayerOne[key];
        }
        else if (count == 5)
          return;
        else
          sendKeys[count++] = key;
      }
      // disable the row
      pinMode(rowPin, INPUT);
    }
    // disable the column
    pinMode(colPin, INPUT);
  }

  // Right
  if (PCF.isConnected())
  for(uint8_t c = 0; c < colCount; c++)
  {
    uint8_t colPin = rightColPins[c];
    PCF.write(colPin, LOW);
    uint8_t readme = PCF.read16();
    for (uint8_t r = 0; r < rowCount; r++)
    {
      if (!((readme >> r) & 0x1))
      {
        uint8_t key = conversionsRight[c][r];
        Serial.println(key);
        if (key == 12 || key == 21)
        {
          mod |= LayerOne[key + 24];
        }
        else if (count == 5)
          return;
        else
          sendKeys[count++] = key + 24;
      }
    }
    PCF.write(colPin, HIGH);
  }

  // Write
  Keyboard.set_key1(0);
  Keyboard.set_key2(0);
  Keyboard.set_key3(0);
  Keyboard.set_key4(0);
  Keyboard.set_key5(0);
  Keyboard.set_key6(0);
  Keyboard.set_modifier(mod);
  switch (count)
  {
  case 6:
    Keyboard.set_key6(getKey(sendKeys[5]));
  case 5:
    Keyboard.set_key5(getKey(sendKeys[4]));
  case 4:
    Keyboard.set_key4(getKey(sendKeys[3]));
  case 3:
    Keyboard.set_key3(getKey(sendKeys[2]));
  case 2:
    Keyboard.set_key2(getKey(sendKeys[1]));
  case 1:
    Keyboard.set_key1(getKey(sendKeys[0]));
  case 0:
    break;
  }
  Keyboard.send_now();
  delay(10);
}
