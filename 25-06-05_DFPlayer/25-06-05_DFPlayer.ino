// DFPlayer Mini with Arduino by ArduinoYard
#include "DFRobotDFPlayerMini.h"

DFRobotDFPlayerMini myDFPlayer;

#include <Keypad.h>

#define ROW_NUM 3
#define COL_NUM 4

int mode = 0;

char keys[ROW_NUM][COL_NUM] = {
  {' ', '!', '"', '#'},
  {'$', '%', '&', '\''},
  {'(', ')', '*', '+'}
};

byte pin_rows[ROW_NUM] = {50, 48, 46}; //connect to the row pinouts of the keypad
byte pin_column[COL_NUM] = {51, 49, 47, 45}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COL_NUM );

void setup() {
    Serial.begin(9600);
    Serial1.begin(9600);
    
    if (!myDFPlayer.begin(Serial1)) {
        Serial.println("DFPlayer Mini not detected!");
        while (true);
    }
    
    Serial.println("DFPlayer Mini ready!");
    myDFPlayer.volume(25);  // Set volume (0 to 30)
}

void loop() {

  char key = keypad.getKey();
  int intKey;

  if (key){
    intKey = (int) key;
    intKey -= 32;
    Serial.print("Pressed: ");
    Serial.println(intKey);

    myDFPlayer.playFolder(mode, intKey+1);
  }

  if(Serial.available()>0){
    String receivedString = Serial.readString();
    int index = receivedString.indexOf('||');
    String msg = receivedString.substring(0, index);
    String value = receivedString.substring(index+2);

    if(msg == "mode"){
      if(value == "jam")
        mode = 1;
      else
        mode = 0;
    }
  }

}