#include "DHDCC.h"

DHDCC DccDecoder;

byte Speed;
byte Direction;

void setup() {
  
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// debug
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

  Serial.begin(9600);
  Serial.println("Starting DCC Decoder");

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Hardware setup
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

  pinMode(13, OUTPUT); // external led for TxRx signalling
  digitalWrite(13, LOW);   //LED off

  DccDecoder.DecoderSetup(2); // use pin 2 for dcc signal input from "DCC-to-logical-level" converter!!!!

  Speed=0;
  Direction=0;
  
};



void loop() {
  // put your main code here, to run repeatedly:

  Serial.print(" 0=");
  Serial.print(DccDecoder.GetOutputState(0));
  Serial.print(" 1=");
  Serial.print(DccDecoder.GetOutputState(1));
  Serial.print(" 2=");
  Serial.print(DccDecoder.GetOutputState(2));
  Serial.print(" ");
  Serial.print(DccDecoder.GetOutputState(3));
  Serial.print(" ");
  Serial.print(DccDecoder.GetOutputState(4));
  Serial.print(" ");
  Serial.print(DccDecoder.GetOutputState(5));
  Serial.print(" ");
  Serial.print(DccDecoder.GetOutputState(6));
  Serial.print(" ");
  Serial.print(DccDecoder.GetOutputState(7));
  Serial.print(" 8=");
  Serial.print(DccDecoder.GetOutputState(8));
  Serial.print(" 9=");
  Serial.print(DccDecoder.GetOutputState(9));
  Serial.print(" 10=");
  Serial.print(DccDecoder.GetOutputState(10));
  Serial.print(" 11=");
  Serial.print(DccDecoder.GetOutputState(11));
  Serial.print(" 12=");
  Serial.print(DccDecoder.GetOutputState(12));
  Serial.print(" 13=");
  Serial.print(DccDecoder.GetOutputState(13));
  Serial.print(" 14=");
  Serial.print(DccDecoder.GetOutputState(14));
  Serial.print(" 15=");
  Serial.println(DccDecoder.GetOutputState(15));
  

    /*if(Direction!=DccDecoder.GetDirection()){
      Direction=DccDecoder.GetDirection();
      Serial.print("Direction ");
      Serial.println(Direction);      
    };

    if(Speed!=DccDecoder.GetSpeed()){
      Speed=DccDecoder.GetSpeed();
      Serial.print("Speed ");
      Serial.println(Speed);      
    };

    for(byte i=0;i<29;i++){
    
      if(DccDecoder.GetFnState(i)){
        Serial.print("F");
        Serial.print(i);
        Serial.println(" is on");
      }
    }
*/

};





