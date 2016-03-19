#ifndef H_DHDCC_03032016
#define H_DHDCC_03032016
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Simple DCC decoder
// Vasily Vasilchikov 
// doublehead@mail.ru
// http://scaletrainsclub.com/board
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
//#include "C:\Program Files (x86)\Arduino\hardware\arduino\avr\libraries\EEPROM\EEPROM.h"

#include <Arduino.h>
#include <EEPROM.h>

#define DHDCC_EEPROM_LENGTH 1024

#define DHDCC_MaxPacketLength 5 //  Maximum DCC Packet Length 

// Multi Function Digital Decoder
#define DHDCC_MFDD

// Accessory Digital Decoder
#define DHDCC_ADD

/*

#define MF7BIT        // DHDCC_ADDresses 00000001-01111111 (1-127)(inclusive): Multi-Function decoders with 7 bit DHDCC_ADDresses
#define BA9BIT        // DHDCC_ADDresses 10000000-10111111 (128-191)(inclusive): Basic Accessory Decoders with 9 bit DHDCC_ADDresses and Extended Accessory Decoders with 11-bit DHDCC_ADDresses
#define MF14BIT       // DHDCC_ADDresses 11000000-11100111 (192-231)(inclusive): Multi Function Decoders with 14 bit DHDCC_ADDresses

*/

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// class for holding DCC packet
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

class DCCPacket{

  public:
  
    DCCPacket(void);
  
    void SetDHDCC_ADDress(volatile byte * InDHDCC_ADDress);
    bool PushData(volatile byte * InData);
    
    byte GetDHDCC_ADDress(void);
    byte GetDataLength(void);
    byte GetData(byte Position);
    
  private:
    byte DataPositionCounter;
    byte DHDCC_ADDress;
    byte Data[DHDCC_MaxPacketLength-1];
    

};

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// class for holding DCC packets stack
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

class DCCPacketStack{

  public:
    DCCPacketStack(void);
    bool PushPacket(DCCPacket * InPacket);
    bool PopPacket(DCCPacket * OutPacket);
    void Clear(void);
    
  private:
    byte StoredPackets;
    byte LastStoredPacketPosition;
    DCCPacket Packets[10];
    
};

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main library class 
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

class DHDCC{

  public:
    //DHDCC(void);
    void DecoderSetup(byte InPin);    
    //void ProcessPackets(void);
    bool GetFnState(byte InFN);
    byte GetSpeed();
    byte GetDirection();
    
  private:
    //bool GlobalFN(byte InFN);
    //bool GlobalFNSet(byte InFN, byte InState);
/*#ifdef MF7BIT
bool ProcessMF7BIT(DCCPacket * InPacket);
bool ProcessMF7BIT(DCCPacket * InPacket){
};
#endif

#ifdef BA9BIT
bool ProcessBA9BIT(DCCPacket * InPacket);
bool ProcessBA9BIT(DCCPacket * InPacket){
};
#endif

#ifdef MF14BIT
bool ProcessMF14BIT(DCCPacket * InPacket);
bool ProcessMF14BIT(DCCPacket * InPacket){
};
#endif
*/
    

// Speed, direction, Fx states

/*byte GlobalSpeed;        // speed 256 steps? but we use only 128 steps for now
byte GlobalDirection;    // direction 0/1 (0 - reverse,1 - forward)
unsigned int GlobalFn1;   // function states FL(FL0),F1,F2,F3 - F15
unsigned int GlobalFn2;   // funstion states F16-F28*/
// need to implement analog function output at least for two pro mini pins

// Configuration Variable Access Instruction - Long Form
/*unsigned int CVAI_LAST_DHDCC_ADDRESS;
byte CVAI_LAST_DATA;
bool CVAI_UPDATE;*/

/*#ifdef MF7BIT



#endif
*/

};



bool DHDCC_GlobalFN(byte InFN);
bool DHDCC_GlobalFNSet(byte InFN, byte InState);

void DHDCC_ProcessPackets(void);
void DCC_ISR();

#endif
