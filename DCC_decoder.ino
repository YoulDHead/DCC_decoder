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

    //DccDecoder.ProcessPackets();   

    if(Direction!=DccDecoder.GetDirection()){
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


};





