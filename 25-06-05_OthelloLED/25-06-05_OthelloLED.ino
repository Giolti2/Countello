#include <Keypad.h>
#include <Adafruit_NeoPixel.h>
#include "DFRobotDFPlayerMini.h"

DFRobotDFPlayerMini myDFPlayer;

struct Note {
  int key;
  long timestamp;
};

bool jamMode = false;
bool printDebug = false;
bool winDebug = false;
String playbackTrack = "";
String playbackNext = "";
Note nextNote;

bool playback = false;
long playbackTimer = 0;

#define ROW_NUM 8
#define COL_NUM 8

#define LEDPIN 7
#define LEDNUM 64

char keys[ROW_NUM][COL_NUM] = {
  {' ', '!', '"', '#', '$', '%', '&', '\''},
  {'(', ')', '*', '+', ',', '-', '.', '/'},
  {'0', '1', '2', '3', '4', '5', '6', '7'},
  {'8', '9', ':', ';', '<', '=', '>', '?'},
  {'@', 'A', 'B', 'C', 'D', 'E', 'F', 'G'},
  {'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O'},
  {'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W'},
  {'X', 'Y', 'Z', '[', '\\', ']', '^', '_'}
};

byte pin_rows[ROW_NUM] = {31, 33, 35, 37, 39, 41, 43, 45}; //connect to the row pinouts of the keypad
byte pin_column[COL_NUM] = {30, 32, 34, 36, 38, 40, 42, 44}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COL_NUM );

Adafruit_NeoPixel strip(LEDNUM, LEDPIN, NEO_GRB + NEO_KHZ800);

uint32_t colorList[3] ={
  strip.Color(0,0,0),
  strip.Color(50, 0, 0), // less 1 led
  strip.Color(0, 0, 50)
};

int board[ROW_NUM][COL_NUM];
int score[2] = {0, 0};
bool validDirs[8];
bool playing = true;
bool recording = false;
long recordTimer = 0;
bool winBlink = false;

long winBlinkTimer = 0;
#define WINBLINKINT 300
#define WINBLINKDUR 5000
int winBlinkStatus = 0;

int activePlayer = 1;

int directions[8][2] = {
  {-1, -1},
  {-1, 0},
  {-1, 1},
  {0, -1},
  {0, 1},
  {1, -1},
  {1, 0},
  {1, 1}
};

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  if (!myDFPlayer.begin(Serial1)) {
    Serial.println("DFPlayer Mini not detected!");
    while (!myDFPlayer.begin(Serial1)){};
  }
    
  Serial.println("DFPlayer Mini ready!");
  myDFPlayer.volume(25);  // Set volume (0 to 30)

  strip.begin();
  strip.show();

  for (int i; i<ROW_NUM; i++){
    for (int j; j<COL_NUM; j++){
      board[i][j] = 0;
    }
  }

  for (int i = 0; i < sizeof(validDirs)/sizeof(validDirs[0]); i++){
    validDirs[i] = false;
  }

  board[3][3] = 1;
  board[4][4] = 1;
  board[3][4] = 2;
  board[4][3] = 2;

  printBoard();
  ledBoard();
}

void loop() {
  // put your main code here, to run repeatedly:
  char key = keypad.getKey();
  int intKey;

  int keyRow;
  int keyCol;

  if (!playback && key){
    intKey = (int) key;
    intKey -= 32;

    keyRow = intKey / COL_NUM;
    keyCol = intKey % COL_NUM;

    Serial.print("Pressed: ");
    Serial.print(keyRow);
    Serial.print(" x ");
    Serial.println(keyCol);
    if(!jamMode && playing){
      bool valid = validMove(keyRow, keyCol, activePlayer);

      if(valid){
        myDFPlayer.playFolder(0, intKey+1);

        othelloMove(keyRow, keyCol, activePlayer);
        Serial.print("SCORE1||");
        Serial.println(score[0]);
        Serial.print("SCORE2||");
        Serial.println(score[1]);
        if(checkVictory() || winDebug){
          winDebug = false;

          Serial.print("WIN||");
          playing = false;
          if(score[0] > score[1]){ //p1 victory
            winBlinking(1);
            Serial.println( "1");
          }
          else if(score[0] < score [1]){ //p2 victory
            winBlinking(2);
            Serial.println("2");
          }
          else{ //draw
            winBlinking(0);
            Serial.println("0");
          }
        }
        else{
          activePlayer = 3 - activePlayer;
        }

        printBoard();
        ledBoard();
      }
      else{
        Serial.println("Invalid move");
      }

      if(winBlink){
        winBlinkingRoutine();
      }
    }
    else if(jamMode){
      myDFPlayer.playFolder(1, intKey+1);
      if(recording){
        long timestamp = millis() - recordTimer;
        String msg = String(timestamp);
        msg += "/";
        msg += String(intKey);
        msg += "x";
        Serial.print("KEY||");
        Serial.println(msg);
      }
    }
  }
  else{ //playback
    if((playbackNext == "") && (playbackTrack != "")){ 
      int index = playbackTrack.indexOf("x");
      playbackNext = playbackTrack.substring(0, index);
      playbackTrack = playbackTrack.substring(index+1);
    }

    if(playbackNext != ""){
      int index = playbackNext.indexOf("/");
      long timestamp = playbackNext.substring(0, index).toInt();
      int key = playbackNext.substring(index+1).toInt();
    }
  }

  if(Serial.available()>0){
    String receivedString = Serial.readString();
    int index = receivedString.indexOf('||');
    String msg = receivedString.substring(0, index);
    String value = receivedString.substring(index+2);

    if(msg == "MODE"){
      if(value == "JAM")
        jamMode = 1;
      else if(value == "COUNT")
        jamMode = 0;
    }
    else if(msg == "RECORD"){
      if(value == "ON"){
        recording = true;
        recordTimer = millis();
      }
      else if(value == "OFF"){
        recording = false; 
      } 
    }
    else if(msg == "PLAYBACK"){
      playbackTrack = value;

      playback = true;
      playbackTimer = millis();
    }
    else if(msg == "START"){
      Serial.println("ACK");
    }

    receivedString.trim();
    if(receivedString == "jam"){
      jamMode = 1;
    }
    else if(receivedString == "win"){
      winDebug = true;
    }
  }
}

void printBoard(){
  if(printDebug){
    for(int i = 0; i < ROW_NUM; i++){
      for(int j = 0; j < COL_NUM; j++){
        Serial.print(board[i][j]);
        Serial.print(" ");
      }
      Serial.println("");
    }
  }
  
}

void ledBoard(){
  for(int i = 0; i < ROW_NUM; i++){
    for(int j = 0; j < COL_NUM; j++){
      if((i%2) == 0){
        strip.setPixelColor(i*COL_NUM + j, colorList[board[i][j]]);
      }
      else{
        strip.setPixelColor((i+1)*COL_NUM - j - 1, colorList[board[i][j]]);
      }
    }
  }
  strip.show();
}

void setCell(int row, int col, uint32_t color){
  int cell = row * COL_NUM;

  if((row % 2) == 0){ //if it's odd = regular direction
    cell += col;
  }
  else{ //if it's even = reverse direction
    cell += (COL_NUM - col) - 1;
  }

  strip.setPixelColor(cell, color);
}

bool validMove(int row, int col, int player){
  if(board[row][col] != 0){
    return false;
  }

  int activeRow;
  int activeCol;

  for (int i = 0; i < sizeof(validDirs)/sizeof(validDirs[0]); i++){
    validDirs[i] = false;
  }

  for (int k = 0; k < sizeof(directions)/sizeof(directions)[0]; k++){
    activeRow = row + directions[k][0];
    activeCol = col + directions[k][1];

    if(withinBounds(activeRow, activeCol)){
      if(board[activeRow][activeCol] == (3 - player)){
        bool search = true;
        bool validDir = true;

        while(search){
          activeRow = activeRow + directions[k][0];
          activeCol = activeCol + directions[k][1];

          if(withinBounds(activeRow, activeCol)){
            if(board[activeRow][activeCol] == 0){
              search = false;
              validDir = false;
            }
            else if(board[activeRow][activeCol] == player){
              search = false;
            }
          }
        }

        validDirs[k] = validDir;
      }
    }
  }

  for (int i = 0; i < sizeof(validDirs)/sizeof(validDirs[0]); i++){
    if(validDirs[i]){
      return true;
    }
  }

  return false;
}

void othelloMove(int row, int col, int player){

  int activeRow;
  int activeCol;

  board[row][col] = player;

  for(int dir = 0; dir < sizeof(validDirs)/sizeof(validDirs[0]); dir++){

    if(validDirs[dir]){
      
      activeRow = row + directions[dir][0];
      activeCol = col + directions[dir][1];

      do{
        board[activeRow][activeCol] = player;

        activeRow = activeRow + directions[dir][0];
        activeCol = activeCol + directions[dir][1];
        Serial.print("Checking cell ");
        Serial.print(activeRow);
        Serial.print(" x ");
        Serial.println(activeCol);
      }
      while(board[activeRow][activeCol] == (3 - player));
    }

  }

  updateScore();
}

void updateScore(){
  for (int i = 0; i < ROW_NUM; i++){
    for (int j = 0; j < COL_NUM; j++){
      if(board[i][j] == 1){
        score[0]++;
      }
      else if(board[i][j] == 2){
        score[1]++;
      }
    }
  }
}

void winBlinking(int player){
  winBlinkStatus = player;

  winBlink = true;
  winBlinkTimer = millis();
}

void winBlinkingRoutine(){

}

bool checkVictory(){
  int player = 3 - activePlayer;
  for(int i = 0; i < ROW_NUM; i++){
    for(int j = 0; j < COL_NUM; j++){
      if(board[i][j] == 0){
        if(validMove(i, j, player)){
          return false;
        }
      }
    }
  }

  return true;
}

bool withinBounds(int row, int col){
  return ((row >= 0) && (row < ROW_NUM) && (col >= 0) && (col < COL_NUM));
}