#include <Adafruit_NeoPixel.h>

#define LEDPIN 2
#define LEDNUM 64

#define ROW_NUM 8
#define COL_NUM 8

Adafruit_NeoPixel strip(LEDNUM, LEDPIN, NEO_GRB + NEO_KHZ800);

uint32_t colorList[3] ={
  strip.Color(0,0,0),
  strip.Color(50, 0, 0), // less 1 led
  strip.Color(0, 0, 50)
};

int board[ROW_NUM][COL_NUM] = {
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0},
  {0,1,2,1,2,1,2,0}
};

void setup() {
  // put your setup code here, to run once:
  strip.begin();
  strip.setBrightness(50);

  ledBoard();

}

void loop() {
  // put your main code here, to run repeatedly:

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
