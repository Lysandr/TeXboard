/* Teensy USB keyboard
 *  Padraig Lysandrou September 2018
 * Open issues: FXN+key will send the proper command but also sends the normal keystroke...
 * ex: FN+F11 sends volume up and fit to screen commands......
*/
#include <Keyboard.h>

#define GREEK 0xfe
#define MATH 0xff
#define KEY_NONE 0x00
#define KEY_FXN 0xfd
#define KEY_XTRA KEY_DELETE 

const byte COLS = 14;   //fourteen columns
const byte ROWS = 6;    //six rows
byte colPins[COLS] = {12,14,15,16,17,18,19,20,21,22,23,24,25,26};
byte rowPins[ROWS] = {2,3,4,5,6,11};
unsigned int keys_pressed[6] = {0};
unsigned int modifiers_pressed[2] = {0};
int cycle_time_ms = 1;
int FXN_flag = 0;


unsigned long last_debounce_time[ROWS][COLS] = {0};  // the last time the output pin was toggled
unsigned long time_since_last_TX[ROWS][COLS] = {0};
int last_button_state[ROWS][COLS] = {0};
unsigned long debounceDelay = 30;    // the debounce time; increase if the output flickers
unsigned long transmitDelay = 30;

unsigned int key_matrix[ROWS][COLS] = {
  {KEY_ESC,KEY_F1,KEY_F2,KEY_F3,KEY_F4,KEY_F5,KEY_F6,KEY_F7,KEY_F8,KEY_F9,KEY_F10,KEY_F11,KEY_F12,KEY_XTRA},
  {KEY_TILDE,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,KEY_0,KEY_MINUS,KEY_EQUAL,KEY_BACKSPACE},
  {KEY_TAB,KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T,KEY_Y,KEY_U,KEY_I,KEY_O,KEY_P,KEY_LEFT_BRACE,KEY_RIGHT_BRACE,KEY_BACKSLASH},
  {KEY_CAPS_LOCK,KEY_A,KEY_S,KEY_D,KEY_F,KEY_G,KEY_H,KEY_J,KEY_K,KEY_L,KEY_SEMICOLON,KEY_QUOTE,KEY_ENTER,KEY_NONE},
  {MODIFIERKEY_SHIFT,KEY_Z,KEY_X,KEY_C,KEY_V,KEY_B,KEY_N,KEY_M,KEY_COMMA,KEY_PERIOD,KEY_SLASH,MODIFIERKEY_RIGHT_SHIFT,KEY_NONE,KEY_NONE},
  {MODIFIERKEY_CTRL,MODIFIERKEY_GUI,MODIFIERKEY_ALT,KEY_NONE,KEY_NONE,KEY_NONE,KEY_SPACE,KEY_NONE,KEY_NONE,GREEK,KEY_FXN,MATH,MODIFIERKEY_RIGHT_CTRL,KEY_NONE}
};

/* this sets up all of our row/col pins 
 * and initializes keyboard
 */
void setup() {
  pinMode(13,OUTPUT);
  for(int row = 0; row < ROWS; row++){
    pinMode(rowPins[row], INPUT_PULLDOWN);
  }
  for(int col = 0; col < COLS; col++){
    pinMode(colPins[col], OUTPUT);
    digitalWrite(colPins[col], LOW);
  }
  Keyboard.begin();
  Serial.begin(9600);
}


// testing the keyboard now
bool there_exists(unsigned int key){
  for(int i=0; i<6; i++){
    if(keys_pressed[i] == key){
      return(true);
    }
  }
  return(false);
}


void fn_handler(int key_to_lookat){
  remove_key(key_to_lookat);
  switch(key_to_lookat){
    case KEY_F11:
      // Keyboard.print("KEY_F11");
      Keyboard.press(   KEY_MEDIA_VOLUME_INC);
      Keyboard.releaseAll();
      break;
    case KEY_F10:
      // Keyboard.print("KEY_F10");
      Keyboard.press(   KEY_MEDIA_VOLUME_DEC);
      Keyboard.releaseAll();
      break;
    case KEY_F9:
      // Keyboard.print("KEY_F9");
      Keyboard.press(   KEY_MEDIA_PLAY_PAUSE);
      Keyboard.releaseAll();
      break;
    case KEY_F8:
      // Keyboard.print("KEY_F8");
      Keyboard.press(   KEY_MEDIA_NEXT_TRACK);
      Keyboard.releaseAll();
      break;
    case KEY_F7:
      // Keyboard.print("KEY_F7");
      Keyboard.press(   KEY_MEDIA_PREV_TRACK);
      Keyboard.releaseAll();
      break;
    case KEY_ESC:
      // Keyboard.print("ESC");
      Keyboard.press(   KEY_MEDIA_MUTE);
      Keyboard.releaseAll();
      break;
    }
}
void math_handler(){}
void greek_handler(){}

/* Just to check if the key is a modifier
 */
bool check_for_modifier(unsigned int key_to_lookat){
  switch(key_to_lookat){
    case MODIFIERKEY_SHIFT:       return(true);
    case MODIFIERKEY_RIGHT_SHIFT: return(true);
    case MODIFIERKEY_CTRL:        return(true);
    case MODIFIERKEY_GUI:         return(true);
    case MODIFIERKEY_ALT:         return(true);
    case MODIFIERKEY_RIGHT_CTRL:  return(true);
  }
  return(false);
}


void remove_key(unsigned int key){
  if(key != 0){
    for(int i=0; i<6; i++){
      if(keys_pressed[i] == key){
        keys_pressed[i] = 0;
        digitalWrite(13,LOW);
        //Serial.println("key released");
        }
    }
  }
}

void add_key(unsigned int key){
  for(int i=0; i<6; i++){
    if(keys_pressed[i] == 0){
      keys_pressed[i] = key;
      digitalWrite(13,HIGH);
      //Serial.println("key clicked");
      break;
      }
  }
}

// if the mod is not already in the array, find a place that is zero and put it there.
void add_mod_list(unsigned int key){
  // if it is not already in there, find a place to put it
  if(modifiers_pressed[0] != key && modifiers_pressed[1] != key){
    if(modifiers_pressed[0] == 0x00){modifiers_pressed[0] = key;}
    else if(modifiers_pressed[1] == 0x00){modifiers_pressed[1] = key;}
  }
}

// remove the modifier
void remove_mod_list(unsigned int key){
  if(modifiers_pressed[0] == key){
    modifiers_pressed[0] = 0x00;
  }
  if(modifiers_pressed[1] == key){
    modifiers_pressed[1] = 0x00;
  }
}

void debounce_key_matrix(int row, int col){
  int state = digitalRead(rowPins[row]);
  int key_to_lookat = key_matrix[row][col];
  
  // If the state of the key has changed, log this time...
  if (state != last_button_state[row][col]) {
    last_debounce_time[row][col] = millis();
    if(key_to_lookat == KEY_FXN){
      FXN_flag = state;
    }
  }
  
  // Basically IF THE STATE OF A KEY HAS CHANGED:
  if ((millis() - last_debounce_time[row][col]) > debounceDelay) {
    // CHECK if the key is a classical modifier key (alt,ctrl,shft,gui)
    if (state == 1 && check_for_modifier(key_to_lookat)){
      add_mod_list(key_to_lookat);
      Keyboard.set_modifier(modifiers_pressed[0] | modifiers_pressed[1]);
      Keyboard.send_now();
    }
    else if (state !=1 && check_for_modifier(key_to_lookat)){
      remove_mod_list(key_to_lookat);
      Keyboard.set_modifier(modifiers_pressed[0] | modifiers_pressed[1]);
      Keyboard.send_now();
    }
    else if((key_to_lookat!=KEY_FXN)&& state==1 && (millis()-time_since_last_TX[row][col]) > transmitDelay){
      //Serial.println("key has been hit");
      add_key(key_to_lookat);
      if(FXN_flag && ((millis()-time_since_last_TX[row][col]) > 3*transmitDelay)){
        fn_handler(key_to_lookat);
      }
      time_since_last_TX[row][col] = millis();
    }
    else if(state !=1 && !check_for_modifier(key_to_lookat)){
      remove_key(key_to_lookat);
    }
  }
  last_button_state[row][col] = state;
}

void send_keys(){
  remove_key(KEY_FXN);
  remove_key(MATH);
  remove_key(GREEK);
  Keyboard.set_key1(keys_pressed[0]);
  Keyboard.set_key2(keys_pressed[1]);
  Keyboard.set_key3(keys_pressed[2]);
  Keyboard.set_key4(keys_pressed[3]);
  Keyboard.set_key5(keys_pressed[4]);
  Keyboard.set_key6(keys_pressed[5]);
  Keyboard.send_now();
}


void loop_key_matrix(){
  for(int col = 0; col < COLS; col++){
    digitalWrite(colPins[col], HIGH);
    for(int row = 0; row < ROWS; row++){
      debounce_key_matrix(row,col); 
    }
    digitalWrite(colPins[col], LOW);
  }
  send_keys();
}


void loop() {
  delay(cycle_time_ms);
  loop_key_matrix();
}

/* 
*  
*
 */
