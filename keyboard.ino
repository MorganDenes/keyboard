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

void printBits(uint32_t b)
{
  Serial.print("0x");
  for(int i = 0; i < 32; i++)
  {
    Serial.print(b & 0x1 << i ? '1' : '0');
  }
  Serial.println();
}

#include "PCF8575.h"
#define LAYER_TWO 0xAAAA

class Keys
{
private:
    uint32_t mask[2];

public:
    void Clear()
    {
        mask[0] = 0x0;
        mask[1] = 0x0;
    }

    bool Get(int index)
    {
        return mask[index < 32] & 0x1 << index % 32;
    }

    void Set(int index)
    {
        mask[index < 32] |= 0x1 << index % 32;
    }

    void Unset(int index)
    {
        mask[index < 32] &= 0xFFFFFFFF ^ (0x1 << index % 32);
    }

    void Check()
    {
      printBits(mask[0]);
      printBits(mask[1]);
    }
};

class SendKeys
{
private:
    int _mod;
    int _count;
    int _keys[6];
    void (*_setkeys[6])(uint16_t) = {
        [](uint16_t k){ Keyboard.set_key1(k); },
        [](uint16_t k){ Keyboard.set_key2(k); },
        [](uint16_t k){ Keyboard.set_key3(k); },
        [](uint16_t k){ Keyboard.set_key4(k); },
        [](uint16_t k){ Keyboard.set_key5(k); },
        [](uint16_t k){ Keyboard.set_key6(k); }
    };

public:
    void Clear()
    {
        _mod = 0;
        _count = 0;
    }

    void Add(int k)
    {
        if ((k & 0xFF00) == 0xE000)
        {
            _mod |= k;
        }
        else if ((k & 0xFF00) == 0xF000 && _count != 6)
        {
            _keys[_count++] = k;
        }
    }

    void Send()
    {
      for (auto i = 0; i < 6; i++)
      {
        _setkeys[i](i < _count ? _keys[i] : 0);
      }
      Keyboard.set_modifier(_mod);
      Keyboard.send_now();
    }
};

class HPKey
{
private:
    const int _key;
    const int _hold;
    const unsigned long _delay = 90;

    bool _set;
    bool _held;
    unsigned long _time;

public:
    HPKey(int k, int h) : _key(k), _hold(h) { }

    void Check(Keys& keyMask, SendKeys& sendKeys)
    {
        bool pressed = keyMask.Get(_key);
        if(!_set && pressed)
        {
            _set = true;
            _held = false;
            _time = millis();
        }

        if (_set)
        {
            if(pressed)
            {
                keyMask.Unset(_key);
                if(millis() - _time > _delay)
                {
                    // set held
                    sendKeys.Add(_hold);
                    _held = true;
                }
            }
            else
            {
                _set = false;
                if (_held)
                {
                    keyMask.Unset(_key);
                    _held = false;
                }
                else
                {
                  keyMask.Set(_key);
                }
            }
        }
    }
};

PCF8575 PCF(0x20, &Wire1);

const uint8_t rowCount = 6;
const uint8_t colCount = 5;
const uint8_t keyCount = rowCount * colCount * 2;

uint8_t leftColPins[colCount] { 14, 15, 20, 21, 22 };
uint8_t leftRowPins[rowCount] { 6, 7, 8, 9, 10, 11 };
uint8_t rightColPins[colCount] { 11, 12, 13, 14, 15 };
uint8_t rightRowPins[rowCount] { 0, 1, 2, 3, 4, 5 };

uint16_t layerKey;
Keys keyMask;
SendKeys sendKeys;
HPKey hpKeys[] = {
    HPKey(22, MODIFIERKEY_SHIFT)
    HPKey(21, MODIFIERKEY_CTRL)
};

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

int LayerOne[keyCount] {
// LEFT
//  |                 |              |          |                |      |          |          |
    KEY_TAB,          KEY_Q,         KEY_W,     KEY_E,           KEY_R, KEY_T,
    KEY_ESC,          KEY_A,         KEY_S,     KEY_D,           KEY_F, KEY_G,
    MODIFIERKEY_CTRL, KEY_BACKSLASH, KEY_Z,     KEY_X,           KEY_C, KEY_V,                MODIFIERKEY_SHIFT,
                                     KEY_TILDE, MODIFIERKEY_ALT,        KEY_MINUS, KEY_SPACE, LAYER_TWO,
// RIGHT
// |                           |                  |      |               |                |              |
                               KEY_Y,             KEY_U, KEY_I,          KEY_O,           KEY_P,         KEY_BACKSPACE,
                               KEY_H,             KEY_J, KEY_K,          KEY_L,           KEY_SEMICOLON, KEY_QUOTE,
   MODIFIERKEY_GUI,            KEY_B,             KEY_N, KEY_M,          KEY_COMMA,       KEY_PERIOD,    KEY_SLASH,     
   KEY_EQUAL,       KEY_ENTER, MODIFIERKEY_SHIFT,        KEY_LEFT_BRACE, KEY_RIGHT_BRACE
};

int LayerTwo[keyCount] {
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

  // Find layer key
  for(int i = 0; i < keyCount; i++)
  {
    if (LayerOne[i] == LAYER_TWO)
    {
        layerKey = i;
        break;
    }
  }
}

void loop()
{
  keyMask.Clear();
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
          keyMask.Set(conversionsLeft[c][r]);
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
          keyMask.Set(conversionsRight[c][r] + 24);
      }
    }
    PCF.write(colPin, HIGH);
  }

  // SEND
  sendKeys.Clear();
  for (auto& k : hpKeys)
  {
    k.Check(keyMask, sendKeys);
  }
  
  for(int i = 0; i < keyCount; i++)
  {
    if (keyMask.Get(i))
    {
      sendKeys.Add(!keyMask.Get(layerKey) ? LayerOne[i] : LayerTwo[i]);
    }
  }
  sendKeys.Send();
  delay(5);
}
