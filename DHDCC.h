#ifndef H_DHDCC_03032016
#define H_DHDCC_03032016
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Simple DCC decoder
// Vasily Vasilchikov 
// doublehead@mail.ru
// http://scaletrainsclub.com/board
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

#include <Arduino.h>
#include <EEPROM.h>

#define DHDCC_EEPROM_LENGTH 1024

#define DHDCC_MaxPacketLength 5 //  Maximum DCC Packet Length 

// Multi Function Digital Decoder
// #define DHDCC_MFDD

// Accessory Digital Decoder
#define DHDCC_ADD

// Number of adresses served by accessory decoder (base adress + slave adresses) e.g. for adresses 10,11,12 - DHDCC_ADD_ADRESSES=3
#define DHDCC_ADD_ADRESSES  2

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
    void Clear();
    
  private:
    byte DataPositionCounter;
    byte DHDCC_ADDress;
    byte Data[DHDCC_MaxPacketLength-1];
    

};

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// class for holding DCC packets stack
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

/*class DCCPacketStack{

  public:
    DCCPacketStack(void);
    bool PushPacket(DCCPacket * InPacket);
    bool PopPacket(DCCPacket * OutPacket);
    void Clear(void);
    
  private:
    byte StoredPackets;
    byte LastStoredPacketPosition;
    DCCPacket Packets[10];
    
};*/

// ---------------------------------------------------------------------------------------------------------------------------------------------------------------
// Main library class 
// ---------------------------------------------------------------------------------------------------------------------------------------------------------------

class DHDCC{

  public:
    void DecoderSetup(byte InPin);    
    bool GetFnState(byte InFN);
    bool GetOutputState(byte InOutput);
    byte GetSpeed();
    byte GetDirection();
};

bool DHDCC_GlobalFN(byte InFN);
bool DHDCC_GlobalFNSet(byte InFN, byte InState);

bool DHDCC_GlobalOutput(byte InOutput);
bool DHDCC_GlobalOutputSet(byte InOutput, byte InState);

void DHDCC_ProcessPackets(void);
void DCC_ISR();
#endif //H_DHDCC_03032016
