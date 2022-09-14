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

const int leftRowCount = 6;
const int leftColCount = 5;
byte leftCols[leftColCount] { 14, 15, 20, 21, 22 };
byte leftRows[leftRowCount] { 6, 7, 8, 9, 10, 11 };

int conversionsLeft[leftColCount][leftRowCount] {
  { 17, 18, -1, -1, 21, 20 },
  { 11, 12, 16, 15, 14, 13 },
  {  5,  6, 10,  9,  8,  7 },
  { -1, -1,  4,  3,  2,  1 },
  { 24, 19, 23, 22, -1, -1 }
};

const int rightRowCount = 6;
const int rightColCount = 5;
byte rightCols[rightColCount] { 11, 12, 13, 14, 15 };
byte rightRows[rightRowCount] { 0, 1, 2, 3, 4, 5 };

int conversionsRight[rightColCount][rightRowCount] {
  { -1, -1,  3,  4,  5,  6 },
  {  1,  2,  9, 10, 11, 12 },
  { 14, 15, -1, -1, 23, 24 },
  {  7,  8, 16, 17, 18, 19 },
  { 13, 20, 21, 22, -1, -1 }
};

int LayerOne[leftColCount * leftRowCount * 2] {
// LEFT
//  |          |      |                 |                |      |           |          |
    KEY_TAB,   KEY_Q, KEY_W,            KEY_E,           KEY_R, KEY_T,
    KEY_ESC,   KEY_A, KEY_S,            KEY_D,           KEY_F, KEY_G,
    KEY_TILDE, KEY_Z, KEY_X,            KEY_C,           KEY_V, KEY_B,                 KEY_BACKSLASH,
                      MODIFIERKEY_CTRL, MODIFIERKEY_ALT,        KEY_PERIOD, KEY_SPACE, /*LAYER TWO*/0,
// RIGHT
// |                           |                  |      |               |                |              |
                               KEY_Y,             KEY_U, KEY_I,          KEY_O,           KEY_P,         KEY_BACKSPACE,
                               KEY_H,             KEY_J, KEY_K,          KEY_L,           KEY_SEMICOLON, KEY_QUOTE,
   MODIFIERKEY_GUI,            KEY_B,             KEY_N, KEY_M,          KEY_MINUS,       KEY_EQUAL,     KEY_SLASH,
   KEY_COMMA,       KEY_ENTER, MODIFIERKEY_SHIFT,        KEY_LEFT_BRACE, KEY_RIGHT_BRACE
};

int LayerTwo[leftColCount * leftRowCount * 2] {
// LEFT
//  |
    0, 0, 0,                       0,               KEY_COMMA, KEY_PERIOD,
    0, 0, KEY_LEFT,                KEY_UP,          KEY_RIGHT, 0,
    0, 0, 0,                       KEY_DOWN,        0,         0,             0,
          MODIFIERKEY_CTRL,        MODIFIERKEY_ALT,            0,          0, /*NOOP*/0,
// RIGHT
// |
                       0,                 KEY_7, KEY_8, KEY_9, KEY_0, 0,
                       0,                 KEY_4, KEY_5, KEY_6, 0,     0,
   MODIFIERKEY_GUI,    0,                 KEY_1, KEY_2, KEY_3, 0,     0,
   0,               0, MODIFIERKEY_SHIFT,        KEY_0, 0
};




void setup()
{
  // Left
  for (int r = 14; r <= 18; r++)
      pinMode(r == 18 ? 22 : r, INPUT);
  for (int c = 6; c <= 11; c++)
  {
      pinMode(c, INPUT);
  }

  // Right
  PCF.begin();
  PCF.write16(0x00);
  for (int r : rightRows)
    PCF.write(r, HIGH);
}

short count;
int sendKeys[6];
int mod;
int layerKey = 24;
int layer;

int getKey(int indexish)
{
  if (layer == 0)
    return LayerOne[indexish -1];
  else
    return LayerTwo[indexish -1];
}

void loop()
{
  count = 0;
  mod = 0x0;
  layer = 0;

  // Left
  for (int colIndex=0; colIndex < leftColCount; colIndex++) {
    // col: set to output to low
    byte curCol = leftCols[colIndex];
    pinMode(curCol, OUTPUT);
    digitalWrite(curCol, LOW);

    // row: interate through the rows
    for (int rowIndex=0; rowIndex < leftRowCount; rowIndex++) {
      byte rowCol = leftRows[rowIndex];
      pinMode(rowCol, INPUT_PULLUP);
      delay(1);
      if (digitalRead(rowCol) == 0)
      {
        int key = conversionsLeft[colIndex][rowIndex];
//        Serial.println(key);
        if (key == layerKey)
        {
          layer = 1;
        }
        else if (key == 20 || key == 21)
        {
//          Serial.println("Left mod key pressed");
          mod |= LayerOne[key - 1];
        }
        else if (count == 5)
          return;
        else
          sendKeys[count++] = key;
      }
      pinMode(rowCol, INPUT);
    }
    // disable the column
    pinMode(curCol, INPUT);
  }

  // Right
  if (PCF.isConnected())
  for(int i = 0; i < rightColCount; i++)
  {
    int c = rightCols[i];
    PCF.write(c, LOW);
    auto readme = PCF.read16();
    // printPCF(readme);
    for (int j = 0; j < rightRowCount; j++)
    {
      if (!((readme >> j) & 0x1))
      {
        Serial.print(i);
        Serial.print(", ");
        Serial.println(j);
        int key = conversionsRight[i][j];
        Serial.println(key);
        if (key == 13 || key == 22)
        {
          mod |= LayerOne[key  + 23];
        }
        else if (count == 5)
          return;
        else
          sendKeys[count++] = key + 24;
      }
    }
    PCF.write(c, HIGH);
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
