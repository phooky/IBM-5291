
const int STROBE = 14; // b5
const int D0 = 21; // f0
const int D1 = 20; // f1
const int D2 = 19;
const int D3 = 18; 
const int D4 = 17;
const int D5 = 16;
const int D6 = 15;
const int KP = 13;
const int DATA[7] = { D0, D1, D2, D3, D4, D5, D6 };


void setup() {
  //Serial.begin(9600);
  for (int i = 0; i < 7; i++) {
    pinMode(DATA[i], OUTPUT);
    digitalWrite(DATA[i],LOW);
  }
  pinMode(STROBE, OUTPUT);
  digitalWrite(STROBE, HIGH);
  pinMode(KP, INPUT);
}

bool readKey(int x) {
  
  for (int i = 0; i < 7; i++) {
    digitalWrite(DATA[i], ((x & (1<<i))!=0)?HIGH:LOW);
  }
  digitalWrite(STROBE,LOW);
  delayMicroseconds(5);
  bool rv = digitalRead(KP) == HIGH;
  digitalWrite(STROBE,HIGH);
  return rv;
}

void scanMatrix();

void loop() {
  scanMatrix();
  delay(10);
}

#define MOD(bit) ((1<<8) | (1 << bit))
const int MOD_LCTRL = MOD(0);
const int MOD_LSHIFT = MOD(1);
const int MOD_LALT = MOD(2);
const int MOD_LGUI = MOD(3);
const int MOD_RCTRL = MOD(4);
const int MOD_RSHIFT = MOD(5);
const int MOD_RALT = MOD(6);
const int MOD_RGUI = MOD(7);

#define IS_MOD(x) ((x & 1<<8) != 0)

const int keycount = 96;

const int keymap[96] = {
  KEY_Z,    KEY_S,    KEY_W,    KEY_3,    KEY_X,    KEY_D,    KEY_E,    KEY_4,
  KEY_C,    KEY_F,    KEY_R,    KEY_5,    KEY_V,    KEY_G,    KEY_T,    KEY_6,

  KEY_B,    KEY_H,    KEY_Y,    KEY_7,    KEY_N,    KEY_J,    KEY_U,    KEY_8,
  KEYPAD_2, KEYPAD_5, KEYPAD_8, KEY_NUM_LOCK, KEY_M, KEY_K,   KEY_I,    KEY_9,

  KEY_COMMA, KEY_L,   KEY_O,    KEY_0,    KEY_PERIOD, KEY_SEMICOLON, KEY_P, KEY_MINUS,
  KEY_SLASH, KEY_QUOTE, KEY_LEFT_BRACE, KEY_EQUAL, KEY_SPACE, MOD_RSHIFT, KEY_BACKSLASH, KEY_RIGHT_BRACE,

  KEY_CAPS_LOCK, KEYPAD_ASTERIX, KEY_ENTER, KEY_BACKSPACE, KEYPAD_0, KEYPAD_1, KEYPAD_4, KEYPAD_7,
  KEYPAD_PLUS, 0,     KEYPAD_MINUS, KEY_SCROLL_LOCK, KEYPAD_PERIOD, KEYPAD_3, KEYPAD_6, KEYPAD_9,

  0 /*???*/, KEY_A,   KEY_Q,    KEY_2,    MOD_LALT, 0,        0,        KEY_1,
  KEY_F7,   KEY_F5,   KEY_F3,   KEY_F1,   KEY_F8,   KEY_F6,   KEY_F4,   KEY_F2,

  KEY_F10,  0,        0,        0,        KEY_F9,   0,        0,        0,
  MOD_LSHIFT,MOD_LCTRL,KEY_TAB, KEY_ESC,  0,        0,        0,        0,
};

void scanMatrix() {
  int k[6] = { 0, 0, 0, 0, 0, 0 };
  int kidx = 0;
  int mods = 0;

  for (int i = 0; i < keycount; i++) {
    int v = keymap[i];
    if (v == 0) continue;
    if (readKey(i)) {
      if (IS_MOD(v)) {
        mods |= v;
      } else {
        if (kidx < 6) {
          k[kidx++] = v;
        }
      }
    }
  }
  Keyboard.set_modifier(mods & 0xff);
  Keyboard.set_key1(k[0]);
  Keyboard.set_key2(k[1]);
  Keyboard.set_key3(k[2]);
  Keyboard.set_key4(k[3]);
  Keyboard.set_key5(k[4]);
  Keyboard.set_key6(k[5]);
  Keyboard.send_now();
}

