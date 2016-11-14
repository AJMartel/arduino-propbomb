#include <TM1638.h>
#define MAGNET 2
#define KEYHOLE 3
#define ROW1 A2
#define ROW2 A3
#define ROW3 A4
#define ROW4 A5
#define COL1 9
#define COL2 10
#define COL3 11
#define BUZZER 12
#define LED 13

// define a module on data pin 6, clock pin 7 and strobe pin 5
TM1638 module(6, 7, 5); //(data, clock, strobe)

int timeSet = 0; //wordt daarna overschreven in setBomb
unsigned char keycode; //0 tot 11 of 0xFF if no key is pressed};
unsigned long Starttijd; //via millis bijhouden om met verschil te kunnen werken
unsigned char passCode[] ={'0', '0', '0', '0'};
unsigned char rightCode[] ={'*', '*', '*', '*'};
boolean running;
byte rightWires(0b00001111);
int fouten=0;
int juisten=0;

void setup() 
{
  pinMode(ROW1, INPUT_PULLUP);
  pinMode(ROW2, INPUT_PULLUP);
  pinMode(ROW3, INPUT_PULLUP);
  pinMode(ROW4, INPUT_PULLUP);
  
  pinMode(COL1, INPUT_PULLUP);
  pinMode(COL2, INPUT_PULLUP);
  pinMode(COL3, INPUT_PULLUP);
  
  digitalWrite(BUZZER, LOW);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(LED, LOW);
  pinMode(LED, OUTPUT);
  pinMode(KEYHOLE, INPUT_PULLUP);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(KEYHOLE),keyChange,CHANGE);
  //attachInterrupt(digitalPinToInterrupt(MAGNET),keyChange,CHANGE);
  setBomb();
}

void(* resetFunc) (void) = 0;//declare reset function at address 0
//http://www.instructables.com/id/two-ways-to-reset-arduino-in-software/step2/using-just-software/

void loop() 
{
  if (running==1)
  {
    keycode = getKeyCode();
    if (keycode!=0xFF) {newdigit(keycode);}
    checkWires();
    int time = (millis()-Starttijd)/100;
    int timeLeft = timeSet-time;
    if (timeLeft<1) {explode();} //ontplof als timer op 0 komt
    updateDisplay(timeLeft);
  }
}

void setBomb(void)
{
  Serial.println("running setBomb");
  setWires(); //instellen welke draden doorgeknipt mogen worden
  module.clearDisplay();
  module.setDisplayToString("set min", 0, 0);
  timeSet=0;
  delay(500);
  setTimer(); //instellen hoeveel minuten de bom aftelt
  timeSet=timeSet*600; //omzetten naar tienden van seconden
  setPassKey(); //code voor disarm instellen
}

void setTimer(void)
{
  Serial.println("running setTimer");
  do
    {
      keycode=getKeyCode();
      if (keycode!=0xFF && keycode !='*' && keycode !='#' ) {
        timeSet=timeSet*10;
        timeSet=timeSet+(keycode-48);
        module.clearDisplay();
        module.setDisplayToDecNumber(timeSet,0, false);
        Serial.print("Bomb set to ");
        Serial.print(timeSet);
        Serial.println(" minutes...");
        delay(500);
      }
    } while (keycode!='#');
}

void setPassKey()
{
  Serial.println("running setPassKey");
  module.clearDisplay();
  module.setDisplayToString("****", 240, 0);
  delay(300);
  for (int i=0; i<4;i++)
  {
    do
    {
      keycode=getKeyCode();
      if (keycode!=0xFF) {
        rightCode[i]=keycode;
        delay(500);
      }
    } while (keycode==0xFF);
    Serial.print("setting digit ");
    Serial.print(i);
    Serial.print(" to ");
    Serial.println(rightCode[i]-48);
    module.setDisplayDigit(rightCode[i]-48, i, true);
  }
  module.setDisplayToString("go?", 240, 5);
  do {
    keycode=getKeyCode();
    delay(500);
    if (keycode=='*') {setPassKey();}
    if (keycode=='#') 
    {
      Serial.println("Bomb armed");
      armBomb();
      return;
    }
  }
  while (keycode!='*' || keycode !='#');
}

void setWires(void)
/*
6 LSB staan voor draden. Bij HIGH is zijn dit degene die je moet doorknippen om de bom onscadelijk te maken
Bit 7 is voor de code. Is deze LOW, dan maakt de code de bom niet onschadelijk
*/
{
  Serial.println("running setWires");
  module.clearDisplay();
  module.setDisplayToString("set wires", 0);
  byte mem = 63; //beginwaarde van de LEDs
  do
  {
    keycode=getKeyCode();
    byte keys = module.getButtons();
    mem = mem ^ keys;
    module.setLEDs(mem);
    delay(100);
  } while (keycode!='#');
  rightWires = mem;
  Serial.print("Setting wirecode to ");
  Serial.println(rightWires);
  delay(200);
}

void armBomb(void)
{
  module.setLEDs (0); //verstop welke draad mag doorgeknipt worden
  module.clearDisplay();
  module.setDisplayToString("armed", 0);
  keycode=getKeyCode();
  boolean run=checkPanic();
  if (run==true)
  {
    running=1;
    Starttijd = millis();
    Serial.println("start running");
    return;
  }
  else {armBomb();}
}

void newdigit(unsigned char digit)
{
  Serial.print(keycode);
  passCode[0]=passCode[1];
  passCode[1]=passCode[2];
  passCode[2]=passCode[3];
  passCode[3]=digit;
  delay(200); //Tegen de Concatdender
  int i=0;
  int stop=0;  
  do 
  {
    if (passCode[i]==rightCode[i]) 
    {
      i++;
      if (i>3) {
        Serial.println("goede code");
        disarm();
      }
    }
    else {stop=1;}
  }
  while (stop==0);  
}

unsigned char getKeyCode(void)
{
  keycode = 0xFF;
  pinMode(COL1, OUTPUT);
  digitalWrite(COL1, LOW);
  if (digitalRead(ROW1) == 0){keycode = '1';}
  if (digitalRead(ROW2) == 0){keycode = '4';}
  if (digitalRead(ROW3) == 0){keycode = '7';}
  if (digitalRead(ROW4) == 0){keycode = '*';}
  
  digitalWrite(COL1, HIGH);
  pinMode(COL1, INPUT_PULLUP);
  
  pinMode(COL2, OUTPUT);
  digitalWrite(COL2, LOW);
  if (digitalRead(ROW1) == 0){keycode = '2';}
  if (digitalRead(ROW2) == 0){keycode = '5';}
  if (digitalRead(ROW3) == 0){keycode = '8';}
  if (digitalRead(ROW4) == 0){keycode = '0';}
  
  digitalWrite(COL2, HIGH);
  pinMode(COL2, INPUT_PULLUP);
  
  pinMode(COL3, OUTPUT);
  digitalWrite(COL3, LOW);
  if (digitalRead(ROW1) == 0){keycode = '3';}
  if (digitalRead(ROW2) == 0){keycode = '6';}
  if (digitalRead(ROW3) == 0){keycode = '9';}
  if (digitalRead(ROW4) == 0){keycode = '#';}
  
  digitalWrite(COL3, HIGH);
  pinMode(COL3, INPUT_PULLUP);
  return (keycode);
}

void checkWires()
{
  Serial.print("Running checkWires  ");
  int allWires=0;
  for (int i=0; i<2; i++)
  {
    int wires = 0;
    int val = analogRead(i);
    Serial.print(val);
    Serial.print(" gemeten op A");
    Serial.print(i);
    Serial.print("    wires= ");
    if (val>750) {wires=0;}
    if (val>600 && val<750) {wires=1;}
    if (val>230 && val<300) {wires=2;}
    if (val>208 && val<230) {wires=3;}
    if (val>450 && val<600) {wires=4;}
    if (val>300 && val<450) {wires=5;}
    if (val>188 && val<208) {wires=6;}
    if (val<188) {wires=7;}
    if (i==1){wires=wires*8;}
    allWires=allWires+wires;
  }
  Serial.println(allWires);
  if (allWires!=63) {
    if(allWires==rightWires)
    {
      juisten++;
      Serial.println("juiste kabel?");
    }
    else
    {
      fouten++;
      Serial.println("Foute kabel?");
    }
  }
  if(fouten>6){explode();}
  if(juisten>4){disarm();}
}

//check panic
boolean checkPanic(void)
{
  boolean alarm = 0;
  pinMode(COL1, INPUT_PULLUP);
  pinMode(COL2, INPUT_PULLUP);
  pinMode(COL3, INPUT_PULLUP);
  if(digitalRead(ROW4) == 0) alarm = 1;
  return (alarm);
}

void explode(void)
{
  //noInterrupts();
  Serial.println("Bomb exploded!!!");
  running=0;
  module.clearDisplay();
  module.setDisplayToString("exploded");
  for (int i = 0; i<12; i++)
  {
    for (int tonex = 440; tonex < 750; tonex++)
    {
      digitalWrite(LED,HIGH);
      tone(BUZZER, tonex);
      delay(5);
      digitalWrite(LED,LOW);
      noTone(BUZZER);
    }
  }
  //wacht op sleutel
}

void disarm(void)
{
  int pauze=millis();
  running=0;
  module.clearDisplay();
  module.setDisplayToString("disarmed");
  do
  {
    keycode=getKeyCode();
    if (boolean run=checkPanic()==1)
    {
      int stoppauze = millis();
      Starttijd = Starttijd + (stoppauze-pauze);
      Serial.println("start running");
      running=1;
      return;
    }
    if (keycode=='*'){resetFunc();}
  }
  while (running==0);
}

void updateDisplay(int timeLeft) {
  int tienden = timeLeft%600;
  int minuten = (timeLeft/600);
  if((tienden/10)%2==0)
  {
    digitalWrite(LED,HIGH);
    fouten=0;
    juisten=0;
  }
  else {digitalWrite(LED,LOW);}
  int timerView = minuten*1000 + tienden;
  module.clearDisplay();
  module.setDisplayToDecNumber(timerView, 10, false);
}

void keyChange(void)
{
  Serial.println("key changed");
  disarm();
}

void magnetChange(void)
{
  Serial.println("box opened");
  explode();
}
