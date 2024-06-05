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
#define LAYER_ONE 0xAA00
#define LAYER_TWO 0xAA01
#define LAYER_THREE 0xAA02
#define LAYER_FOUR 0xAA04

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

    void Map(Keys& target, bool set = true)
    {
      for (int i = 0; i < 60; i++)
      {
          if (this->Get(i))
          {
              if (set)
              {
                target.Set(i);
              }
              else
              {
                target.Unset(i);
              }
          }
      }
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
        else if ((k & 0xFF00) != 0xAA00 && k != 0 && _count != 6)
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

class DelayKey
{
private:
    const int _key;
    const int _hold;
    const int _layer;
    const unsigned long _delay;

    bool _set;
    bool _held;
    unsigned long _time;

public:
    DelayKey(int k, int h, int d = 100, int l = 0x0) : _key(k), _hold(h), _layer(l), _delay(d) { }

    void Check(Keys& keyMask, SendKeys& sendKeys , int& active, int& layer, bool& wait)
    {
        bool pressed = keyMask.Get(_key);
        if(!_set && pressed)
        {
            _set = true;
            active++;
            _held = false;
            _time = millis();
        }

        if (_set)
        {
            if(pressed)
            {
                keyMask.Unset(_key);
                if(_held || millis() - _time > _delay)
                {
                    if ((_layer & 0xFF00) == 0xAA00 )
                    {
                        layer = _layer;
                    }
                    else if ((_hold & 0xFF00) == 0xAA00)
                    {
                        layer = _hold;
                    }

                    if ((_hold & 0xFF00) != 0xAA00)
                    {
                        sendKeys.Add(_hold);
                    }

                    if (!_held)
                    {
                        active--;
                        wait = true;
                    }
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
                  active--;
                  wait = true;
                }
            }
        }
    }
};

PCF8575 PCF(0x20, &Wire1);

bool delayWait = false; // state to know when delay key superposition is at play
int delayActive = 0; // running total of outstanding superposition delay keys
Keys delayIgnoreMask; // keys presssed before a delay key should be ignored
Keys delayMask; // Gather these until all delay keys resolve then send them

bool layerActive = false;
Keys layerMask; // Prevent keypresses to outlive the layer key

Keys keyMask;// Activly pressed keys, sort of. Delay keys will eat these
SendKeys sendKeys;

const uint8_t rowCount = 6;
const uint8_t colCount = 5;
const uint8_t keyCount = rowCount * colCount * 2;

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
    {  0,  1,  8,  9, 10, 11 },
    { 13, 14, 99, 99, 22, 23 },
    {  6,  7, 15, 16, 17, 18 },
    { 12, 19, 20, 21, 99, 99 }
};

// Add 1 to the base numbers to get the correct key
DelayKey hpKeys[] = {
    DelayKey(12, LAYER_FOUR),
    DelayKey(21, MODIFIERKEY_SHIFT),
    DelayKey(22, MODIFIERKEY_CTRL),
    DelayKey(23, MODIFIERKEY_SHIFT, 110, LAYER_THREE),
    DelayKey(11 + 24, LAYER_TWO),
    DelayKey(20 + 24, MODIFIERKEY_SHIFT)
    // DelayKey(21 + 24, MODIFIERKEY_CTRL)
};

// LAYER ONE
// <Alphas>
// ;'/\.,-=[]`+*
// <Enter> <Space> <Backspace> <Esc> <Meta>
int LayerOne[keyCount] {
// LEFT
//  |               |      |                |               |      |              |          |
    KEY_LEFT_BRACE, KEY_B, KEY_Y,           KEY_O,          KEY_U, KEY_MINUS,
    KEY_TILDE,      KEY_C, KEY_I,           KEY_E,          KEY_A, KEY_SEMICOLON,
    KEY_SLASH,      KEY_G, KEY_X,           KEY_J,          KEY_K, KEY_COMMA,                MODIFIERKEY_GUI,
                           MODIFIERKEY_ALT, KEYPAD_ASTERIX,        KEY_ESC,       KEY_SPACE, KEY_TAB,
// RIGHT
// |                |          |           |      |      |      |      |
                               KEY_EQUAL,  KEY_L, KEY_D, KEY_W, KEY_V, KEY_RIGHT_BRACE,
                               KEY_Q,      KEY_H, KEY_T, KEY_S, KEY_N, KEY_QUOTE,
   MODIFIERKEY_GUI,            KEY_PERIOD, KEY_R, KEY_M, KEY_F, KEY_P, KEY_BACKSLASH,
   KEY_BACKSPACE,   KEY_ENTER, KEY_DELETE,        KEY_Z, KEYPAD_PLUS
};

// LAYER TWO
// *-=+./;'\,[]
// <Number line>
// <Enter> <Space> <Backspace>
int LayerTwo[keyCount] {
// LEFT
//  |          |              |      |               |                |              |           |
    0,         KEY_B,         KEY_1, KEY_9,          KEY_0,           0,
    KEY_TILDE, KEY_4,         KEY_5, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, KEY_SEMICOLON,
    0,         KEY_BACKSLASH, KEY_X, KEY_7,          KEY_6,           KEY_COMMA,                 0,
                              0,     KEY_F,                            0,             KEY_SPACE, KEY_TAB,
// RIGHT
// |              |          |             |      |      |      |               |
                             KEY_EQUAL,    KEY_7, KEY_8, KEY_9, KEYPAD_MINUS,   0,
                             KEYPAD_SLASH, KEY_4, KEY_5, KEY_6, KEYPAD_PLUS,    /*NOOP*/0,
   0,                        KEY_PERIOD,   KEY_1, KEY_2, KEY_3, KEYPAD_ASTERIX, 0,
   KEY_BACKSPACE, KEY_ENTER, KEY_0,               KEY_X, KEY_F
};

// LAYER THREE
// ~!@#$%^&*()?<>_+-"|{}
// <Enter> <Space> <Backspace> <Esc> <Meta>
int LayerThree[keyCount] {
// LEFT
//  |          |              |      |               |                |              |  |
    0,         0,             KEY_1, KEY_9,          KEY_0,           KEY_MINUS,
    KEY_TILDE, KEY_4,         KEY_5, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, KEY_SEMICOLON,
    0,         KEY_BACKSLASH, 0,     KEY_7,          KEY_6,           KEY_COMMA,        0,
                              0,     0,                               0,             0, /*NOOP*/0,
// RIGHT
// |              |          |             |          |       |          |               |
                             KEY_EQUAL,    KEY_QUOTE, 0,      0,         KEYPAD_MINUS,   0,
                             KEY_LEFT,     KEY_DOWN,  KEY_UP, KEY_RIGHT, KEYPAD_PLUS,    0,
   0,                        KEY_PERIOD,   KEY_SLASH, KEY_2,  KEY_3,     KEYPAD_ASTERIX, 0,
   KEY_BACKSPACE, KEY_ENTER, KEY_DELETE,              0,      0
};

// LAYER FOUR
// <Function keys>
// <Enter> <Space> <Backspace> <Esc> <Meta>
// <Arrow keys>
// <Arrow keys>
// CXPZY for default copy paste and stuff
int LayerFour[keyCount] {
// LEFT
//  |          |       |       |                |                 |            |          |
    KEY_F1,    KEY_F2, KEY_F3, KEY_F4,          KEY_F5,           KEY_F6,
    KEY_HOME,  KEY_Z,  KEY_X,  KEY_C,           KEY_V,            KEY_Y,
    /*NOOP*/0, 0,      0,      MODIFIERKEY_CTRL, MODIFIERKEY_ALT, KEY_PAGE_UP,            KEY_PRINTSCREEN,
                       0,      KEY_BACKSPACE,                     KEY_ENTER,   KEY_SPACE, KEY_TAB,
// RIGHT
// |                |          |              |         |       |          |        |
                               KEY_F7,        KEY_F8,   KEY_F9, KEY_F10,   KEY_F11, KEY_F12,
                               KEY_LEFT,      KEY_DOWN, KEY_UP, KEY_RIGHT, 0,       KEY_END,
   MODIFIERKEY_GUI,            KEY_PAGE_DOWN, 0,        0,      0,         0,       0,
   KEY_BACKSPACE,   KEY_ENTER, KEY_DELETE,              0,      0
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

void SetSendKeys(int layer[])
{
  for(int i = 0; i < keyCount; i++)
  {
    if (keyMask.Get(i))
    {
      sendKeys.Add(layer[i]);
    }
  }
}

int* GetLayer(int layer)
{
    switch (layer)
    {
    case LAYER_ONE:
        return LayerOne;
    case LAYER_TWO:
        return LayerTwo;
    case LAYER_THREE:
        return LayerThree;
    case LAYER_FOUR:
        return LayerFour;
    default:
      break;
    }
    return LayerOne;
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
  int layer = LAYER_ONE;
  bool wait = false;
  for (auto& k : hpKeys)
  {
    k.Check(keyMask, sendKeys, delayActive, layer, wait);
  }

  if (delayActive != 0)
  {
      if (!delayWait)
      {
          delayMask.Clear();
          delayIgnoreMask.Clear();
          keyMask.Map(delayIgnoreMask);
          delayWait = true;
      }
      else
      {
          for (int i = 0; i < keyCount; i++)
          {
              if (keyMask.Get(i) && !delayIgnoreMask.Get(i))
              {
                  keyMask.Unset(i);
                  delayMask.Set(i);
              }
          }
      }
  }
  else
  {
      if (delayWait && !wait)
      {
          delayMask.Map(keyMask);
          delayWait = false;
      }
  }

  if (layer != LAYER_ONE)
  {
      keyMask.Map(layerMask);
  }
  else
  {
      for (int i = 0; i < keyCount; i++)
      {
          if (layerMask.Get(i))
          {
              if (keyMask.Get(i))
              {
                  keyMask.Unset(i);
              }
              else
              {
                  layerMask.Unset(i);
              }
          }
      }
  }


  SetSendKeys(GetLayer(layer));
  sendKeys.Send();
  delay(4);
}
