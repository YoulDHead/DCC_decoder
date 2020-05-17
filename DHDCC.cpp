#include "DHDCC.h"

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// CVs config 
// According to NMRA S-9.2.2 we must have only three mandatory CVs. DHDCC_CV1 DHDCC_ADDress, DHDCC_CV7 Mfr version, DHDCC_CV8 Mfr Id.
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

volatile byte DHDCC_CV1;
volatile byte DHDCC_CV3;
volatile byte DHDCC_CV4;
volatile byte DHDCC_CV5;
volatile byte DHDCC_CV6;
volatile byte DHDCC_CV7;
volatile byte DHDCC_CV8;
volatile byte DHDCC_CV9;
volatile byte DHDCC_CV17;
volatile byte DHDCC_CV18;
volatile byte DHDCC_CV29;

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Service mode operations
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
volatile bool DHDCC_ServiceMode;

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// packet catcher
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

volatile int DHDCC_StartTime;
volatile int DHDCC_Interval;

// parser stage
volatile byte DHDCC_ParserStage;

#define DHDCC_WaitPreamble        1
#define DHDCC_WaitPacketStartBit  2
#define DHDCC_ReadDHDCC_ADDress         3
#define DHDCC_DHDCC_WaitDataStartBit    4
#define DHDCC_ReDHDCC_ADData            5
#define DHDCC_WaitPacketEndBit    6 

// preamble processing
volatile byte DHDCC_PreambleBitsCounter;

// packet processing
volatile bool DHDCC_WaitData;
// Packet format DHDCC_TempPacket[0] - DHDCC_ADDress byte, DHDCC_TempPacket[1..DHDCC_MaxPacketLength] - data bytes  
volatile byte DHDCC_TempPacket[DHDCC_MaxPacketLength];
volatile byte DHDCC_TempPacketLength=0;
volatile byte DHDCC_DataBitsCounter;
volatile byte DHDCC_DatabytesCounter;

// DHDCC_Reset packet counter
volatile byte DHDCC_DHDCC_ResetPacketsCounter;

// DHDCC_Reset DHDCC_Idle DHDCC_Stop flags
volatile bool DHDCC_LastPacketReset;
volatile bool DHDCC_Reset;
volatile bool DHDCC_Stop;
volatile bool DHDCC_Idle;

// Speed, direction, Fx states

byte GlobalSpeed;        // speed 256 steps? but we use only 128 steps for now
byte GlobalDirection;    // direction 0/1 (0 - reverse,1 - forward)
unsigned int GlobalFn1;   // function states FL(FL0),F1,F2,F3 - F15
unsigned int GlobalFn2;   // funstion states F16-F28

// Output state
unsigned int GlobalOutput1;

// Configuration Variable Access Instruction - Long Form
unsigned int CVAI_LAST_DHDCC_ADDRESS;
byte CVAI_LAST_DATA;
bool CVAI_UPDATE;

DCCPacket PacketStack;

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// class for holding DCC packet
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

DCCPacket::DCCPacket(void){
  DHDCC_ADDress=0;
  DataPositionCounter=0;
}

void DCCPacket::SetDHDCC_ADDress(volatile byte * InDHDCC_ADDress){
   DHDCC_ADDress=*InDHDCC_ADDress;
};

bool DCCPacket::PushData(volatile byte * InData){
    
  if(DataPositionCounter<(DHDCC_MaxPacketLength-1)){
    Data[DataPositionCounter]=*InData;
    DataPositionCounter++;
    return true;
  }
    return false;
}
byte DCCPacket::GetDHDCC_ADDress(void){
  return DHDCC_ADDress;
};

byte DCCPacket::GetDataLength(void){
  return DataPositionCounter;
};

byte DCCPacket::GetData(byte Position){
  return Data[Position];
};

void DCCPacket::Clear(){
  for(byte i=0;i<DHDCC_MaxPacketLength;i++){
      Data[i]=0;
  }
}
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main library class 
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------


bool DHDCC_GlobalFN(byte InFN){

    if(InFN<16){
        if(bitRead(GlobalFn1,InFN)==1){
          return true;
        }
        return false;
    }else if(InFN<29){
        if(bitRead(GlobalFn2,InFN-16)==1){
          return true;
        }
        return false;
    }
   return false;  
};

// decorator for 
bool DHDCC::GetFnState(byte InFN){

    return DHDCC_GlobalFN(InFN);

};

bool DHDCC_GlobalFNSet(byte InFN, byte InState){

    if(InFN<16){
        if(InState){
          bitSet(GlobalFn1,InFN);
        }else{
          bitClear(GlobalFn1,InFN);
        }
        return true;
    }else if(InFN<29){
        if(InState){
          bitSet(GlobalFn2,InFN-16);
        }else{
          bitClear(GlobalFn2,InFN-16);
        }
        return true;
    }
   return false;  
};

byte DHDCC::GetSpeed(){
    return GlobalSpeed;
};

byte DHDCC::GetDirection(){
    return GlobalDirection;
};

bool DHDCC_GlobalOutput(byte InOutput){

    if(InOutput<16){
        if(bitRead(GlobalOutput1,InOutput)==1){
          return true;
        }
        return false;
    }
   return false;  
};

// decorator for 
bool DHDCC::GetOutputState(byte InOutput){

    return DHDCC_GlobalOutput(InOutput);

};

bool DHDCC_GlobalOutputSet(byte InOutput, byte InState){

    if(InOutput<16){
        if(InState){
          bitSet(GlobalOutput1,InOutput);
        }else{
          bitClear(GlobalOutput1,InOutput);
        }
        return true;
    }
   return false;  
};


void DHDCC::DecoderSetup(byte InPin){

   pinMode(InPin, INPUT_PULLUP);  // DCC signal input pin 2

   attachInterrupt(digitalPinToInterrupt(InPin),DCC_ISR,CHANGE); // attach interrupt ot dcc digital input
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// CV Setup
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
  
  DHDCC_CV1=EEPROM.read(1); // read stored values from eeprom
  DHDCC_CV3=EEPROM.read(3);
  DHDCC_CV4=EEPROM.read(4);
  DHDCC_CV5=EEPROM.read(5);
  DHDCC_CV6=EEPROM.read(6);
  DHDCC_CV7=EEPROM.read(7); // 
  DHDCC_CV8=EEPROM.read(8); // 
  DHDCC_CV9=0;//EEPROM.read(9); // 
  DHDCC_CV17=EEPROM.read(17); // 
  DHDCC_CV18=EEPROM.read(18); // 
  DHDCC_CV29=128;//EEPROM.read(29); // 

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Service mode setup
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

  DHDCC_ServiceMode=false; // initially we're in operation mode
  
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Catcher setup
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

  DHDCC_PreambleBitsCounter=0; // DHDCC_Reset preamble bits counter

  for(byte i=0;i<DHDCC_MaxPacketLength;i++){
    DHDCC_TempPacket[i]=0; // DHDCC_Reset temp packet
  }
  DHDCC_TempPacketLength=0;
  DHDCC_DataBitsCounter=0; // DHDCC_Reset bits counter
  DHDCC_DatabytesCounter=0; // DHDCC_Reset bytes counter

  DHDCC_ParserStage=DHDCC_WaitPreamble; // set stage to DHDCC_WaitPreamble
  
  DHDCC_DHDCC_ResetPacketsCounter=0;

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// DHDCC_Reset DHDCC_Stop DHDCC_Idle flags setup
// 
  DHDCC_LastPacketReset=false;
  DHDCC_Reset=false;
  DHDCC_Stop=false;
  DHDCC_Idle=false;

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// DHDCC_Reset DHDCC_Stop DHDCC_Idle flags setup
// 
  GlobalSpeed=0;        // speed 256 steps? but we use only 128 steps for now
  GlobalDirection=0;    // direction 0/1
  GlobalFn1=0;   // function states FL,F1,F2,F3 - F15
  GlobalFn2=0;

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// DHDCC_Reset CVAI
// 
CVAI_LAST_DHDCC_ADDRESS=0;
CVAI_LAST_DATA=0;
CVAI_UPDATE=false;

};

void DHDCC_ProcessPackets(void){
  
  DCCPacket TPacket=PacketStack;
  if(0==0){
    
    // filter adress partition
    // DHDCC_ADDress 
    // 00000000 (0): Broadcast DHDCC_ADDress
    // DHDCC_ADDresses 00000001-01111111 (1-127)(inclusive): Multi-Function decoders with 7 bit DHDCC_ADDresses
    // DHDCC_ADDresses 10000000-10111111 (128-191)(inclusive): Basic Accessory Decoders with 9 bit DHDCC_ADDresses and Extended Accessory Decoders with 11-bit DHDCC_ADDresses
    // DHDCC_ADDresses 11000000-11100111 (192-231)(inclusive): Multi Function Decoders with 14 bit DHDCC_ADDresses
    // DHDCC_ADDresses 11101000-11111110 (232-254)(inclusive): Reserved for Future Use
    // DHDCC_ADDress 11111111 (255): DHDCC_Idle Packet

    if(TPacket.GetDHDCC_ADDress()==0 || TPacket.GetDHDCC_ADDress()==255){
      // broadcast or idle packet
      switch (TPacket.GetDHDCC_ADDress()){
        case 0:
          // test for reset packet;
          if((TPacket.GetData(0)+TPacket.GetData(1)+TPacket.GetData(2))==0){
            // process Digital Decoder Reset Packet For All Decoders
              GlobalSpeed=0;
              GlobalDirection=0;
              GlobalFn1=0;
              GlobalFn2=0;
              CVAI_LAST_DHDCC_ADDRESS=0;
              CVAI_LAST_DATA=0;
              CVAI_UPDATE=false;           
          }else
            // test for Digital Decoder Broadcast Stop Packets For All Decoders
            if(TPacket.GetData(0)==0 && TPacket.GetData(1)==TPacket.GetData(2)){
              //process Digital Decoder Broadcast Stop Packets For All Decoders
              if((TPacket.GetData(1) & 1)==0){
                GlobalSpeed=0; // normal stop
              }
              if((TPacket.GetData(1) & 1)==1){
                GlobalSpeed=1; // emergency stop
              }
              // bits four and five ignored for now.
            }          
        break;
        case 255:
          // test for Digital Decoder Idle Packet For All Decoders
          if(TPacket.GetData(0)==255 && TPacket.GetData(1)==0 && TPacket.GetData(2)==255){
            //process Digital Decoder Idle Packet For All Decoders
            // e.g. do nothing :)
          }
        break;       
      }


      
    }

    

/*    if(TPacket.GetDHDCC_ADDress()!=255 && TPacket.GetDHDCC_ADDress()!=0){
      Serial.print(TPacket.GetDHDCC_ADDress(),BIN);
      Serial.print(" ");
      Serial.print(TPacket.GetData(0),BIN);
      Serial.print(" ");
      Serial.print(TPacket.GetData(1),BIN);
      Serial.print(" ");
      Serial.print(TPacket.GetData(2),BIN);
      Serial.print(" ");
      Serial.println(TPacket.GetData(3),BIN);
  
      
    }
    */    
    
//
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Multi Function Digital Decoders
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef DHDCC_MFDD

//Check short/long address
byte DataShift=0;
unsigned int Address=0;

if(  (  (TPacket.GetDHDCC_ADDress()>>5)==3 ) && (  (TPacket.GetDHDCC_ADDress()&63)!=63 )  ){
  // long adress detected - address contained in 0&1 bytes
  DataShift=1;
  Address=((TPacket.GetDHDCC_ADDress()&63)<<8)+TPacket.GetData(1);
}else{
  Address=TPacket.GetDHDCC_ADDress();  
}

// Check for appropriate address
 bool NeedToProcess=false;
 if(DataShift==0 && (bitRead(DHDCC_CV29,5)==0) && (Address>=1) && (Address<=127) && (Address==DHDCC_CV1)){
    NeedToProcess=true;  
 }else  
  if((DataShift==1) && (bitRead(DHDCC_CV29,5)==1) && (Address==((DHDCC_CV17<<8)+DHDCC_CV18))){
  //Serial.println("2");
    NeedToProcess=true;
 }

 if(NeedToProcess){  
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Advanced Operations Instruction (001)
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

    bool RestrictedSpeed=false;

    if((TPacket.GetData(DataShift+1)>>5)==1){
      if((TPacket.GetData(DataShift+1)&31)==31){
        // 11111 : 128 speed step instruction (126+DHDCC_Stop+emergency DHDCC_Stop)
        // bit 7 direction(1-forward,0-reverse), bits 0-6 speed

        GlobalDirection=bitRead(TPacket.GetData(DataShift+2),7);
        //Serial.println(TPacket.GetData(DataShift+2),BIN);
        if((TPacket.GetData(DataShift+2)&127)==0 ||(TPacket.GetData(DataShift+2)&127)==1){
          GlobalSpeed=(TPacket.GetData(DataShift+2)&127);  
        }else{
          //GlobalSpeed=(TPacket.GetData(DataShift+2)&127)-1;
        GlobalSpeed=(TPacket.GetData(DataShift+2)&127);
        }
      }else
      if((TPacket.GetData(DataShift+1)&31)==30){
        // 11110 : Restricted Speed Step Instruction        
        if(bitRead(DHDCC_CV29,1)){
          // 28 steps speed control
          if((TPacket.GetData(DataShift+1)&31)==0 || (TPacket.GetData(DataShift+1)&31)==16){
            GlobalSpeed=0;
          }else
          if((TPacket.GetData(DataShift+1)&31)==1 || (TPacket.GetData(DataShift+1)&31)==17){
            GlobalSpeed=1;
          }else
          if((TPacket.GetData(DataShift+1)&31)>15){
            GlobalSpeed=((TPacket.GetData(DataShift+1)&31)-16)*2-1;
          }else{
            GlobalSpeed=((TPacket.GetData(DataShift+1)&15)*2)-2;
          }
        }else{
          // 14 steps speed control
          //Serial.println(TPacket.GetData(DataShift+1),BIN);
          if((TPacket.GetData(DataShift+1)&15)==0){
            GlobalSpeed=0; 
          }else{
            GlobalSpeed=(TPacket.GetData(DataShift+1)&15);//-1;            
          }
  //        Serial.println("set1");
          DHDCC_GlobalFNSet(0,bitRead(TPacket.GetData(DataShift+1),4));         
        }    
      }else
      if((TPacket.GetData(DataShift+1)&31)==29){
        // 11101 : Analog Function Group
        // TPacket.GetData(DataShift+2) Analog Function Output (0-255)
        // TPacket.GetData(DataShift+3) Analog Function Data (0-255)
      }
    }else
    
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Speed and Direction Instructions (010 and 011)
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
    if(((TPacket.GetData(DataShift+1)>>5)==2) || ((TPacket.GetData(DataShift+1)>>5)==3)){
    // 010 - reverse
      // 011 - forward      
      if((TPacket.GetData(DataShift+1)>>5)==2){
        GlobalDirection=0;
      }else{
        GlobalDirection=1;
      }
      // DHDCC_CV29 bit 1 = 1 speed size 5 bits, else bit#4 controls headlight (FL)
      if(bitRead(DHDCC_CV29,1)){
        // 28 steps speed control
        if((TPacket.GetData(DataShift+1)&31)==0 || (TPacket.GetData(DataShift+1)&31)==16){
          GlobalSpeed=0;
        }else
        if((TPacket.GetData(DataShift+1)&31)==1 || (TPacket.GetData(DataShift+1)&31)==17){
          GlobalSpeed=1;
        }else
        if((TPacket.GetData(DataShift+1)&31)>15){
          GlobalSpeed=((TPacket.GetData(DataShift+1)&31)-16)*2-1;
        }else{
          GlobalSpeed=((TPacket.GetData(DataShift+1)&15)*2)-2;
        }
      }else{
         // 14 steps speed control
         //Serial.println(TPacket.GetData(DataShift+1),BIN);
         if((TPacket.GetData(DataShift+1)&15)==0){
            GlobalSpeed=0; 
         }else{
            GlobalSpeed=(TPacket.GetData(DataShift+1)&15);//-1;            
         }
    //      Serial.println("set2");
         DHDCC_GlobalFNSet(0,bitRead(TPacket.GetData(DataShift+1),4));
      }    
    }else
    
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Group One Instruction (100) controls FL if DHDCC_CV29 bit 1 is set, F1-F4
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
    if((TPacket.GetData(DataShift+1)>>5)==4 ){

      //Serial.println("--3");
     
      DHDCC_GlobalFNSet(1,bitRead(TPacket.GetData(DataShift+1),0)); //  F1 on/off
      DHDCC_GlobalFNSet(2,bitRead(TPacket.GetData(DataShift+1),1)); //  F2 on/off
      DHDCC_GlobalFNSet(3,bitRead(TPacket.GetData(DataShift+1),2)); //  F3 on/off
      DHDCC_GlobalFNSet(4,bitRead(TPacket.GetData(DataShift+1),3)); //  F4 on/off
      if(0==bitRead(DHDCC_CV29,1)){
          //Serial.println("set3");
        DHDCC_GlobalFNSet(0,bitRead(TPacket.GetData(DataShift+1),4)); //  FL on/off
      }
    }else
    
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Group Two Instruction (101) controls F5-F12
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
    if((TPacket.GetData(DataShift+1)>>5)==5){
      //Serial.println("--4");
      if(bitRead(TPacket.GetData(DataShift+1),4)){
        // F5-F8  
        DHDCC_GlobalFNSet(5,bitRead(TPacket.GetData(DataShift+1),0)); //  F5 on/off
        DHDCC_GlobalFNSet(6,bitRead(TPacket.GetData(DataShift+1),1)); //  F6 on/off
        DHDCC_GlobalFNSet(7,bitRead(TPacket.GetData(DataShift+1),2)); //  F7 on/off
        DHDCC_GlobalFNSet(8,bitRead(TPacket.GetData(DataShift+1),3)); //  F8 on/off        
      }else{
        // F9-F12
        DHDCC_GlobalFNSet(9,bitRead(TPacket.GetData(DataShift+1),0)); //  F9 on/off
        DHDCC_GlobalFNSet(10,bitRead(TPacket.GetData(DataShift+1),1)); //  F10 on/off
        DHDCC_GlobalFNSet(11,bitRead(TPacket.GetData(DataShift+1),2)); //  F11 on/off
        DHDCC_GlobalFNSet(12,bitRead(TPacket.GetData(DataShift+1),3)); //  F12 on/off        
      }     
    }else
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Feature Expansion Instruction (110) 
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
     if((TPacket.GetData(DataShift+1)>>5)==6){
           //Serial.println("--5");

       // don't know how to use it with Pro Mini, but....
       // CCCCC = 00000 - Binary State Control Instruction long form
       /*
         if((TPacket.GetData(DataShift+1)<<3)>>3==0){         
         bool TempBinarySate=bitRead(7,TPacket.GetData(DataShift+2));
         unsigned int TempDHDCC_ADDress=(TPacket.GetData(DataShift+3)<<8;
         TempDHDCC_ADDress=TempDHDCC_ADDress+((TPacket.GetData(DataShift+2)<<1)>>1);

         if(TempDHDCC_ADDress==0){
           // broadcast! we must set or clear all 32767 states to TemBinaryState
         }                  
       }else
       // CCCCC = 11101 - Binary State Control Instruction short form
       if((TPacket.GetData(DataShift+1)<<3)>>3==29){
          bool TempBinarySate=bitRead(7,TPacket.GetData(DataShift+2));
          unsigned int TempDHDCC_ADDress=((TPacket.GetData(DataShift+2)<<1)>>1);
          if(TempDHDCC_ADDress==0){
           // broadcast! we must set or clear all 32767 states to TemBinaryState
          }       
       }else
       */
       // CCCCC = 11110 - F13-F20 Function Control
       if((TPacket.GetData(DataShift+1)&31)==30){
         DHDCC_GlobalFNSet(13,bitRead(TPacket.GetData(DataShift+2),0)); //  F13 on/off
         DHDCC_GlobalFNSet(14,bitRead(TPacket.GetData(DataShift+2),1)); //  F14 on/off
         DHDCC_GlobalFNSet(15,bitRead(TPacket.GetData(DataShift+2),2)); //  F15 on/off
         DHDCC_GlobalFNSet(16,bitRead(TPacket.GetData(DataShift+2),3)); //  F16 on/off
         DHDCC_GlobalFNSet(17,bitRead(TPacket.GetData(DataShift+2),4)); //  F17 on/off
         DHDCC_GlobalFNSet(18,bitRead(TPacket.GetData(DataShift+2),5)); //  F18 on/off
         DHDCC_GlobalFNSet(19,bitRead(TPacket.GetData(DataShift+2),6)); //  F19 on/off
         DHDCC_GlobalFNSet(20,bitRead(TPacket.GetData(DataShift+2),7)); //  F20 on/off         
       }else
       // CCCCC = 11111 - F21-F28 Function Control
       if((TPacket.GetData(DataShift+1)&31)==31){
         DHDCC_GlobalFNSet(21,bitRead(TPacket.GetData(DataShift+2),0)); //  F21 on/off         
         DHDCC_GlobalFNSet(22,bitRead(TPacket.GetData(DataShift+2),1)); //  F22 on/off         
         DHDCC_GlobalFNSet(23,bitRead(TPacket.GetData(DataShift+2),2)); //  F23 on/off         
         DHDCC_GlobalFNSet(24,bitRead(TPacket.GetData(DataShift+2),3)); //  F24 on/off         
         DHDCC_GlobalFNSet(25,bitRead(TPacket.GetData(DataShift+2),4)); //  F25 on/off         
         DHDCC_GlobalFNSet(26,bitRead(TPacket.GetData(DataShift+2),5)); //  F26 on/off         
         DHDCC_GlobalFNSet(27,bitRead(TPacket.GetData(DataShift+2),6)); //  F27 on/off         
         DHDCC_GlobalFNSet(28,bitRead(TPacket.GetData(DataShift+2),7)); //  F28 on/off         
       }
    }else

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Configuration Variable Access Instruction (111)
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
    if((TPacket.GetData(DataShift+1)>>5)==7){
            //Serial.println("--6");

      
      // Configuration Variable Access Instruction - Short Form
      if(((TPacket.GetData(DataShift+1)>>4)&15)==15){
        if((TPacket.GetData(DataShift+1)&15)==2){
          // cccc = 0010 set CV23 
          EEPROM.write(23,TPacket.GetData(DataShift+2));         
        }else
        if((TPacket.GetData(DataShift+1)&15)==3){
          // cccc = 0011 set CV24 
          EEPROM.write(24,TPacket.GetData(DataShift+2));
        }
        /*
        if((TPacket.GetData(DataShift+1)&15)==9){
          // cccc = 1001 // LOCK DECODER INSTRUCTION (NMRA S-9.2.3, Appendix B), I think we don't need to implement it
        }*/
      }
      
      // Configuration Variable Access Instruction - Long Form
      if((TPacket.GetData(DataShift+1)>>4)&14==14){
        // 11110CCVV
        // CC = 00 - Reserved
        // CC = 01 - Verify byte
        if((TPacket.GetData(DataShift+1)&12)>>2==1){
          // verify byte
        }else
        if((TPacket.GetData(DataShift+1)&12)>>2==3){
          // write byte with 10 bit adress - need to implement 2-nd packet catcher for this function!
          unsigned int TempCVDHDCC_ADDress=TPacket.GetData(DataShift+1)&3;
          TempCVDHDCC_ADDress=(TempCVDHDCC_ADDress<<8)+TPacket.GetData(DataShift+2);
          
          if(TempCVDHDCC_ADDress<DHDCC_EEPROM_LENGTH){
            if(CVAI_LAST_DHDCC_ADDRESS==TempCVDHDCC_ADDress && CVAI_LAST_DATA==TPacket.GetData(DataShift+3)){
              EEPROM.write(CVAI_LAST_DHDCC_ADDRESS,CVAI_LAST_DATA);              
              CVAI_LAST_DHDCC_ADDRESS=0;
              CVAI_LAST_DATA=0;
              CVAI_UPDATE=false;
            }else{
              CVAI_LAST_DHDCC_ADDRESS=TempCVDHDCC_ADDress;
              CVAI_LAST_DATA=TPacket.GetData(DataShift+3);
              CVAI_UPDATE=true;              
            }              
          }    
        }else
        if((TPacket.GetData(DataShift+1)&12)>>2==2){
          // bit manipulation 
          unsigned int TempCVDHDCC_ADDress=TPacket.GetData(DataShift+1)&3;
          TempCVDHDCC_ADDress=(TempCVDHDCC_ADDress<<8)+TPacket.GetData(DataShift+2);
          
          if((TPacket.GetData(DataShift+3)>>5)==7){
            byte TempBitOperation=(TPacket.GetData(DataShift+3)&16);
            byte TempBitPosition=(TPacket.GetData(DataShift+3)&7);
            byte TempBitContent=(TPacket.GetData(DataShift+3)&8);
            if(TempBitOperation==0){
              // verify bit
              // not implemented yet
            }else{
              // write bit
              if(TempCVDHDCC_ADDress<DHDCC_EEPROM_LENGTH){
                byte TempData=EEPROM.read(TempCVDHDCC_ADDress);
                bitWrite(TempData,TempBitPosition,TempBitContent);
              }
            }  
          }
        }
      }
    }
    
    if(!CVAI_UPDATE){
      CVAI_LAST_DHDCC_ADDRESS=0;
      CVAI_LAST_DATA=0;    
    }
  }

#endif    


// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Accessory Digital Decoder Packet Formats    
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
#ifdef DHDCC_ADD
   
    // Basic Accessory Decoder Packet Format
    if(bitRead(DHDCC_CV29,5)==0){
  
    
      // decoder address
      // detect address depend on adressing mode selected by bit 6 in CV29
      if(bitRead(DHDCC_CV29,6)==0){
        //Decoder Address method  
        //LSB CV1 / CV513   6 bits
        //MSB CV9 / CV521   3 bits
       
        unsigned int ReceivedAddress=(TPacket.GetData(0)&63)
                        | ((~TPacket.GetData(1) & 112) << 2);

        unsigned int MyBaseAddress=(DHDCC_CV9 << 6) | (DHDCC_CV1);

        bool ProcessFlag=false;
        byte Offset=0;
        for(byte i=0;i<DHDCC_ADD_ADRESSES;i++){
           if(MyBaseAddress+i==ReceivedAddress){
              ProcessFlag=true;
              Offset=i;
              break;
           }
        }        

        

        if(ProcessFlag){
 //         Serial.println("basic");  
  //  Serial.print(TPacket.GetData(0),BIN);
  //  Serial.print(" ");
  //  Serial.println(TPacket.GetData(1),BIN);
    //Serial.print(" ");
    //Serial.println(TPacket.GetData(2),BIN);
    //Serial.print(" ");
    //Serial.println(TPacket.GetData(3),BIN);

          
          //process packet  
          //get output from pair (0 - first output, 1 - second);
          byte OutputNum=(TPacket.GetData(1)&1);

    //      Serial.print("OutputNum=");
      //    Serial.println(OutputNum);
          
          // get pair num (0-3)
          byte PairNum=((TPacket.GetData(1)&6)>>1);
     //     Serial.print("PairNum=");
      //    Serial.println(PairNum);
          bool ActivateDeactivate=((TPacket.GetData(1)&8)>>3);
        //  Serial.print("AD=");
         // Serial.println(ActivateDeactivate);

          //set ouptut state. Output number is calculated as PairNum*2+OutputNum
          //Serial.print(Offset);
          //Serial.print(" ");
          //Serial.println(8*Offset+(PairNum*2+OutputNum));
          DHDCC_GlobalOutputSet(8*Offset+(PairNum*2+OutputNum),ActivateDeactivate);
         
        }
      }else{
        //Output Address method       
        // which of pair
        
        
      }
  
  
    }else
    
    // Extended Accessory Decoder Control Packet Format
    if(bitRead(DHDCC_CV29,5)==1){
/*    Serial.println("extended");  
    Serial.print(TPacket.GetData(0),BIN);
    Serial.print(" ");
    Serial.print(TPacket.GetData(1),BIN);
    Serial.print(" ");
    Serial.print(TPacket.GetData(2),BIN);
    Serial.print(" ");
    Serial.println(TPacket.GetData(3),BIN);
*/
    }
#endif


  }



// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// process DHDCC_ServiceMode - need to rewrite?
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

if(DHDCC_ServiceMode && DHDCC_LastPacketReset){
    // Direct mode (112)
    if((TPacket.GetDHDCC_ADDress()&112)==112 && TPacket.GetDHDCC_ADDress()!=255){
      // test for write CV
      if((TPacket.GetDHDCC_ADDress()&12)==12){
        // compose 10 bits DHDCC_ADDress (DHDCC_CV1 DHDCC_ADDress is 0, CV2 - 1 etc)
        unsigned int TDHDCC_ADDress=(TPacket.GetDHDCC_ADDress()&3)<<8;
        TDHDCC_ADDress=TDHDCC_ADDress+TPacket.GetData(1);        
        // next filter our CV
        byte TempCV=0;
        TempCV=EEPROM.read(TDHDCC_ADDress);
        Serial.print("TempCV=");
        Serial.println(TempCV,DEC);
        switch (TDHDCC_ADDress+1){
          case 1:
            DHDCC_CV1=TPacket.GetData(2);
            if(TempCV!=DHDCC_CV1){
              
              EEPROM.write(TDHDCC_ADDress,DHDCC_CV1);
              Serial.print("CV");
              Serial.print(TDHDCC_ADDress+1,DEC);
              Serial.print("=");
              Serial.println(DHDCC_CV1,DEC);
            }
            delay(300);
            PacketStack.Clear();
            DHDCC_ServiceMode=false;
          break;        
          case 9:
            DHDCC_CV9=TPacket.GetData(2);
            if(TempCV!=DHDCC_CV9){              
              EEPROM.write(TDHDCC_ADDress,DHDCC_CV9);
              Serial.print("CV");
              Serial.print(TDHDCC_ADDress+1,DEC);
              Serial.print("=");
              Serial.println(DHDCC_CV9,DEC);
            }
            delay(300);
            PacketStack.Clear();
            DHDCC_ServiceMode=false;
          break;        
          case 17:
            DHDCC_CV17=TPacket.GetData(2);
            if(TempCV!=DHDCC_CV17){
              EEPROM.write(TDHDCC_ADDress,DHDCC_CV17);
              Serial.print("CV");
              Serial.print(TDHDCC_ADDress+1,DEC);
              Serial.print("=");
              Serial.println(DHDCC_CV17,DEC);
            }
            delay(300);              
            PacketStack.Clear();
            DHDCC_ServiceMode=false;
          break;        
          case 18:
            DHDCC_CV18=TPacket.GetData(2);
            if(TempCV!=DHDCC_CV18){
              EEPROM.write(TDHDCC_ADDress,DHDCC_CV18);
              Serial.print("CV");
              Serial.print(TDHDCC_ADDress+1,DEC);
              Serial.print("=");
              Serial.println(DHDCC_CV18,DEC);
            }
            delay(300);
            PacketStack.Clear();
            DHDCC_ServiceMode=false;
          break;        
          case 29:
            DHDCC_CV29=TPacket.GetData(2);
            if(TempCV!=DHDCC_CV29){
              EEPROM.write(TDHDCC_ADDress,DHDCC_CV29);
            }
            Serial.print("CV");
            Serial.print(TDHDCC_ADDress+1,DEC);
            Serial.print("=");
            Serial.println(DHDCC_CV29,DEC);
            delay(500);
            PacketStack.Clear();
            DHDCC_ServiceMode=false;
          break;        
          default:
            DHDCC_ServiceMode=false;
          break;
        }        
      }
    }else{
      DHDCC_ServiceMode=false;  
    }
  }else
  
  // process DHDCC_Reset
  if(DHDCC_Reset){
    DHDCC_DHDCC_ResetPacketsCounter++;
    if(DHDCC_DHDCC_ResetPacketsCounter>=3){
      // Enter to service mode
      DHDCC_ServiceMode=true;
      PacketStack.Clear();
      // call for decoder reset!!!!!
      DHDCC_LastPacketReset=true;      
    }
  }else   
  // process DHDCC_Stop
  if(DHDCC_Stop){
  }else
  // process DHDCC_Idle
  if(DHDCC_Idle){
  }else{
    // process normal packet
    // 
    DHDCC_DHDCC_ResetPacketsCounter=0;  
  };
}

void DCC_ISR(){
  //  DHDCC_Intervals for ProMini  ~ 64 & 120
  noInterrupts();
  if(digitalRead(2)==HIGH){
    DHDCC_StartTime=micros();
  }else{
    DHDCC_Interval=micros()-DHDCC_StartTime;
    byte rBit=0;
    // get next bit
    // if DHDCC_Interval <116 (<58ms*2) bit="1", if more (>>95ms*2) bit="0"
    if (DHDCC_Interval<=116){
      rBit=1;
    }else{
      rBit=0;
    }
    switch (DHDCC_ParserStage){
      // first stage - wait preamble (at least 10 "1" bits) 
      case DHDCC_WaitPreamble:
      {
        if(rBit==1){
          DHDCC_PreambleBitsCounter++;
          if(DHDCC_PreambleBitsCounter==10){
            DHDCC_ParserStage=DHDCC_WaitPacketStartBit;
            DHDCC_PreambleBitsCounter=0;
          }
        }else{
          DHDCC_PreambleBitsCounter=0;  
        }          
      }
      break;  
      // wait for PacketStartBit (0)
      case DHDCC_WaitPacketStartBit:
      {
        if(rBit==0){
          DHDCC_ParserStage=DHDCC_ReadDHDCC_ADDress;
          DHDCC_DataBitsCounter=0;
          DHDCC_DatabytesCounter=0;
        }
      }
      break;
      // read DHDCC_ADDress byte into DHDCC_TempPacket[0]
      case DHDCC_ReadDHDCC_ADDress:
      {
        //set bits
        if(rBit==1){
          bitSet(DHDCC_TempPacket[DHDCC_DatabytesCounter],7-DHDCC_DataBitsCounter);
        } else {
          bitClear(DHDCC_TempPacket[DHDCC_DatabytesCounter],7-DHDCC_DataBitsCounter);
        }          
        DHDCC_DataBitsCounter++;
        // it was last DHDCC_ADDress bit, wait for DataStartBit
        if(DHDCC_DataBitsCounter==8){
          DHDCC_ParserStage=DHDCC_DHDCC_WaitDataStartBit;
          DHDCC_DataBitsCounter=0;
          DHDCC_DatabytesCounter++;
        }
      }
      break;
      // Wait for data start bit
      case DHDCC_DHDCC_WaitDataStartBit:
      {
        if(rBit==0){
          DHDCC_ParserStage=DHDCC_ReDHDCC_ADData;
        }else{
          if(DHDCC_TempPacket[0]==0 && DHDCC_DatabytesCounter==3){
            // possble DHDCC_Reset or DHDCC_Stop packet, check it
            if(DHDCC_TempPacket[1]==0 && DHDCC_TempPacket[2]==0){
                // DHDCC_Reset packet received
                DHDCC_Reset=true;                
            }
            if(((DHDCC_TempPacket[1]&65)==65) && ((DHDCC_TempPacket[0]^DHDCC_TempPacket[1])==DHDCC_TempPacket[2])){
                // DHDCC_Stop packet received
                DHDCC_Stop=true;
            }
          }else if(DHDCC_TempPacket[0]==1 && DHDCC_DatabytesCounter==3){
            // possible DHDCC_Idle packet
            if(DHDCC_TempPacket[1]==0 && DHDCC_TempPacket[2]==1){
                // DHDCC_Idle packet received!!!
                DHDCC_Idle=true;
            }
          }else{
          //if(DHDCC_TempPacket[0]==DHDCC_CV1 || DHDCC_ServiceMode){
          // normal packet        
            // check for consistency              
            byte Check=DHDCC_TempPacket[0];
            for(byte a=1;a<DHDCC_DatabytesCounter-1;a++){
              Check=Check^DHDCC_TempPacket[a];
            }
            if(Check==DHDCC_TempPacket[DHDCC_DatabytesCounter-1]){
              //good packet, do something
              DCCPacket TPacket;
              TPacket.SetDHDCC_ADDress(&DHDCC_TempPacket[0]);
              for (byte a=0;a<DHDCC_DatabytesCounter;a++){
                TPacket.PushData(&DHDCC_TempPacket[a]);
              }
              PacketStack=TPacket;              
              DHDCC_ProcessPackets();
            }
          }          
          DHDCC_ParserStage=DHDCC_WaitPreamble; 
        }
      }
      break;
      case DHDCC_ReDHDCC_ADData:
      {
        if(rBit==1){
          bitSet(DHDCC_TempPacket[DHDCC_DatabytesCounter],7-DHDCC_DataBitsCounter);
        } else {
          bitClear(DHDCC_TempPacket[DHDCC_DatabytesCounter],7-DHDCC_DataBitsCounter);
        }          
        DHDCC_DataBitsCounter++;
        if(DHDCC_DataBitsCounter==8){
          DHDCC_ParserStage=DHDCC_DHDCC_WaitDataStartBit;
          DHDCC_DataBitsCounter=0;
          DHDCC_DatabytesCounter++;
        }
      }
      break;
    }
  }
  interrupts();
};



/*
void print_binary(int v, int num_places)
{
    int mask=0, n;
    for (n=1; n<=num_places; n++)
    {
        mask = (mask << 1) | 0x0001;
    }
    v = v & mask;  // truncate v to specified number of places
    while(num_places)
    {
        if (v & (0x0001 << num_places-1))
        {
             Serial.print("1");
        }
        else
        {
             Serial.print("0");
        }
       --num_places;
        if(((num_places%4) == 0) && (num_places != 0))
        {
            //Serial.print("_");
        }
    }
}*/
