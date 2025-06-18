#include <Keypad.h>
#include <Adafruit_NeoPixel.h>
#include <DFRobotDFPlayerMini.h>

//debug flags
bool printDebug = false;

//keypad setup
#define ROW_NUM 8
#define COL_NUM 8

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

byte pin_rows[ROW_NUM] = {31, 33, 35, 37, 39, 41, 43, 45}; 
byte pin_column[COL_NUM] = {30, 32, 34, 36, 38, 40, 42, 44}; 

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COL_NUM );

//board setup

#define LEDPIN 7
#define LEDNUM 64

Adafruit_NeoPixel strip(LEDNUM, LEDPIN, NEO_GRB + NEO_KHZ800);

uint32_t colorList[3] ={
  strip.Color(0,0,0),
  strip.Color(30, 0, 0), // less 1 led
  strip.Color(0, 0, 30)
};

//game setup

int board[ROW_NUM][COL_NUM];
int score[2] = {0, 0};

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
bool validDirs[8];

//speaker setup

DFRobotDFPlayerMini myDFPlayer;

//state variables

bool jamMode = false;
bool playback = false;
bool recording = false;
bool gameEnd = false;

//victory blinking

long winBlinkTimer = 0;
#define WINBLINKINT 300
#define WINBLINKDUR 5000
int winBlinkStatus = 0;

//recording

struct Note {
  int key;
  long timestamp;
  bool ready = false;
};

long recordTimer = 0;
long playbackTimer = 0;
String playbackTrack = "";
String playbackNext = "";
Note nextNote;

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);

  if (!myDFPlayer.begin(Serial1)) {
    Serial.println("DFPlayer Mini not detected!");
  }
    
  Serial.println("DFPlayer Mini ready!");
  myDFPlayer.volume(25);  // Set volume (0 to 30)

  strip.begin();
  strip.show();

  for (int i = 0; i < sizeof(validDirs)/sizeof(validDirs[0]); i++){ //initalize valid directions as all false
    validDirs[i] = false;
  }

  resetBoard();
}

void loop() {
  // put your main code here, to run repeatedly:
  char key = keypad.getKey();
  int intKey;

  int keyRow;
  int keyCol;

  if (key && !playback && !gameEnd){
    intKey = (int) key;
    intKey -= 32;

    keyRow = intKey / COL_NUM;  //get row and column of pressed button
    keyCol = intKey % COL_NUM;  

    Serial.print("Pressed: ");
    Serial.print(keyRow);
    Serial.print(" x ");
    Serial.print(keyCol);
    Serial.print(" intKey: ");
    Serial.println(intKey);

    if(!jamMode){
      bool valid = validMove(keyRow, keyCol, activePlayer);

      if(valid){
        myDFPlayer.playFolder(0, intKey+1);

        othelloMove(keyRow, keyCol, activePlayer);

        Serial.print("SCORE1||");  //update score on protopie
        Serial.println(score[0]);
        Serial.print("SCORE2||");
        Serial.println(score[1]);

        if(checkVictory()){
          victory();
        }
        else{
          activePlayer = 3 - activePlayer;  //switch player
        }

        printBoard();
        ledBoard();
      }
      else{
        Serial.println("Invalid move");
      }
    }
    else{
      myDFPlayer.playFolder(1, intKey+1);

      if(recording){
        long timestamp = millis() - recordTimer;

        String msg = String(timestamp); //compose string, format TIMExKEY/TIMExKEY/
        msg += "x";
        msg += String(intKey);
        msg += "/";

        Serial.print("KEY||");  //send key and timestamp to protopie
        Serial.println(msg);
      }
    }
  }
  else if (playback){ //playback
    if((!nextNote.ready) && (playbackTrack != "") && (playbackTrack != "\n")){ //check if note is empty but buffer is not
      int index = playbackTrack.indexOf("/");
      playbackNext = playbackTrack.substring(0, index);
      playbackTrack = playbackTrack.substring(index+1);  //take one note off buffer

      index = playbackNext.indexOf("x");
      nextNote.timestamp = playbackNext.substring(0, index).toInt();
      nextNote.key = playbackNext.substring(index+1).toInt();  
      nextNote.ready = true;                              //assign the note to the struct

      playbackNext = "";
    }

    if(nextNote.ready && ((millis() - playbackTimer) > nextNote.timestamp)){ //if note is ready wait for its timestamp

      myDFPlayer.playFolder(1, nextNote.key+1);
      nextNote.ready = false;

      if ((playbackTrack == "") || (playbackTrack == "\n")){
        playback = false;
        Serial.println("PLAY_END"); //tell protopie the playback ended
      }
    }
  }
  else if(gameEnd){
    winBlinkingRoutine();
  }

  if(Serial.available()>0){
    String receivedString = Serial.readString();
    receivedString.trim();

    String value = "";
    String msg = "";
    int index = receivedString.indexOf('||');

    if(index != -1){
      msg = receivedString.substring(0, index);
      value = receivedString.substring(index+2);
    }
    else
      msg = receivedString;
    

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
      playbackTrack.trim();

      playback = true;
      playbackTimer = millis();
    }

    else if(msg == "START"){
      Serial.println("ACK");
    }

    if(receivedString == "jam"){
      jamMode = true;
    }
    else if(receivedString == "win"){
      fakeWin();
    }
    else if(receivedString == "reset"){
      resetBoard();
    }
  }
}

void printBoard(){ //print board to serial for debug
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

void ledBoard(){ //update led board state
  for(int i = 0; i < ROW_NUM; i++){
    for(int j = 0; j < COL_NUM; j++){
      if((i%2) == 0){
        strip.setPixelColor(i*COL_NUM + j, colorList[board[i][j]]);
      }
      else{
        strip.setPixelColor((i+1)*COL_NUM - j - 1, colorList[board[i][j]]); //accounting for right-to-left rows
      }
    }
  }
  strip.show();
}

bool validMove(int row, int col, int player){ //check validity of move
  if(board[row][col] != 0){ //check that cell is empty
    return false;
  }

  int activeRow;
  int activeCol;

  for (int i = 0; i < sizeof(validDirs)/sizeof(validDirs[0]); i++){ //reinitialize directions tracker
    validDirs[i] = false;
  }

  for (int k = 0; k < sizeof(directions)/sizeof(directions)[0]; k++){ //check in every direction
    activeRow = row + directions[k][0];
    activeCol = col + directions[k][1]; 

    if(withinBounds(activeRow, activeCol)){  //don't move outside the board
      if(board[activeRow][activeCol] == (3 - player)){ //check that starting cell is adjacent to opponent cell in                           
        bool search = true;                            //currently checked direction
        bool validDir = false;

        while(search){                                 //search for another player cell in this direction
          activeRow = activeRow + directions[k][0];
          activeCol = activeCol + directions[k][1];

          if(withinBounds(activeRow, activeCol)){
            if(board[activeRow][activeCol] == 0){
              search = false;
              validDir = false;
            }
            else if(board[activeRow][activeCol] == player){
              search = false;
              validDir = true;
            }
          }
          else{                                       //out of bounds
            search = false;
            validDir = false;
          }
        }

        validDirs[k] = validDir;                      //direction is valid if player cell was found after opponent cells going in this direction
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

void othelloMove(int row, int col, int player){ //assumes move is already valid (checked before in loop)

  int activeRow;
  int activeCol;

  board[row][col] = player;

  for(int dir = 0; dir < sizeof(validDirs)/sizeof(validDirs[0]); dir++){  

    if(validDirs[dir]){                                       //goes in every valid direction
      
      activeRow = row + directions[dir][0];
      activeCol = col + directions[dir][1];

      do{                                                     //turn opponent cells into player's
        board[activeRow][activeCol] = player;

        activeRow = activeRow + directions[dir][0];
        activeCol = activeCol + directions[dir][1];
      }
      while(board[activeRow][activeCol] == (3 - player));     //ends when it can't find more opponent cells in chosen direction
    }

  }

  updateScore();
}

void updateScore(){
  score[0] = 0;
  score[1] = 0;
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

  winBlinkTimer = millis();
}

void winBlinkingRoutine(){
  if((millis() - winBlinkTimer) < WINBLINKDUR){
    if((millis() - winBlinkTimer)%(WINBLINKINT * 2) < WINBLINKINT){
      strip.clear();
      strip.show();
    }
    else{
      ledBoard();
    }
  }
  else{
    gameEnd = false;
    resetBoard();
  }
}

void resetBoard(){
  for(int i = 0; i < ROW_NUM; i++){
    for (int j = 0; j < COL_NUM; j++){
      board[i][j] = 0;
    }
  }

  board[3][3] = 1;
  board[4][4] = 1;
  board[3][4] = 2;
  board[4][3] = 2;

  ledBoard();

  recording = false;
  playback = false;
  jamMode = false;
  activePlayer = 1;
}

bool checkVictory(){
  int player = 3 - activePlayer;
  for(int i = 0; i < ROW_NUM; i++){
    for(int j = 0; j < COL_NUM; j++){
      if(board[i][j] == 0){
        if(validMove(i, j, player)){  //checks that every possible move is invalid
          return false;               //execution ends when finding the first valid move
        }
      }
    }
  }

  return true;
}

void fakeWin(){  //debug win state
  for (int i = 0; i < ROW_NUM; i++){
    for (int j = 0; j < COL_NUM; j++){
      if((i%3) == 2)
        board[i][j] = 1;
      else
        board[i][j] = 2;
    }
  }

  updateScore();
  ledBoard();
  victory();
}

void victory(){

  Serial.print("WIN||");
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

bool withinBounds(int row, int col){
  return ((row >= 0) && (row < ROW_NUM) && (col >= 0) && (col < COL_NUM));
}