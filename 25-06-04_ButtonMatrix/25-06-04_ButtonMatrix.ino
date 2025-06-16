#include <Keypad.h>

#define ROW_NUM 2
#define COLUMN_NUM 1

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1'},
  {'2'},
};

byte pin_rows[ROW_NUM] = {2, 3}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {53}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

void setup(){
  Serial.begin(9600);
}

void loop(){
  char key = keypad.getKey();

  if (key){
    Serial.println("KEY||"+String(key));
  }
}
