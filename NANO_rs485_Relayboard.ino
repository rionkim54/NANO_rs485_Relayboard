#define READ_DEBUG_PRINT (1)

#include <SoftwareSerial.h>

// 시리얼 핀 4,5 (RX-RO/TX-DI)
SoftwareSerial rx485(6,5);
// 전송 ENABLE 핀 2 
int EN = 7; 

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  Serial.println("START");

  rx485.begin(9600);
  
  pinMode(EN, OUTPUT);
  digitalWrite(EN, 1); // Transmitter MODE ON
  
}

#define DATA_NUM 256
unsigned char BUFF[DATA_NUM] = { 0x00, };

#define PACKET_NUM 64
unsigned char packet[PACKET_NUM] = { 0x00, };

#define RESP_NUM 32
unsigned char resp[RESP_NUM] = { 0x00, };

#define POLYNORMIAL 0xA001;
unsigned short CRC16(unsigned char * puchMsg, int usDataLen)
{
  int i;
  unsigned short crc, flag;
  crc = 0xFFFF;
  while(usDataLen--)
  {
    crc ^= *puchMsg++;
    for(i = 0; i < 8; i++)
    {
      flag = crc & 0x0001;
      crc >>= 1;
      if(flag) crc ^= POLYNORMIAL;
    }
  }

  return crc;
}

typedef enum _STEP_
{
  STEP_READ_COIL_STATUS = 0,
  STEP_FORCE_SINGLE_COIL,
  STEP_FORCE_MULTIPLE_COIL,
} STEP;

STEP step = STEP_READ_COIL_STATUS;

// 8 bit = 8 relays
byte RELAY_OnOff[1] = { 0x00 };

void forceSingleCoil(int relay, int status)
{
  unsigned short crc;
  
  Serial.print("forceSingleCoil=");
  Serial.print(relay);Serial.print(',');
  Serial.println(status);
    int ri = 0;
    resp[ri++] = 0x01;
    resp[ri++] = 0x05;
    resp[ri++] = 0x02;
    resp[ri++] = 0x58 + relay;
    resp[ri++] = status ? 0xFF : 0x00;
    resp[ri++] = 0x00;
    crc = CRC16(resp, ri);
    //Serial.print("CRC=");
    //Serial.println(crc, HEX);
    resp[ri++] = 0xFF & (crc >> 0);
    resp[ri++] = 0xFF & (crc >> 8);
    digitalWrite(EN, 1);
    rx485.write(resp, ri);
    digitalWrite(EN, 0);  

    bitWrite(RELAY_OnOff[0], relay, status);
}

int timeout = 0;

void loop() {
  unsigned short crc;
  
  memset(BUFF, 0x00, DATA_NUM);
  int pos = 0;
  while(0 < rx485.available())
  {
    unsigned char val = rx485.read();
    BUFF[pos] = val; pos += 1;
    delay(1);
    if(DATA_NUM-20 < pos) break;
  }

  int pi = 0;
  int di = 0;

#if READ_DEBUG_PRINT
  if(0 < pos) {
      for(int i = 0; i < pos; i++) {
        Serial.print(BUFF[i], HEX); Serial.print(',');
      }
      Serial.println();
  }
#endif

  // DATA Parsing
  if(0 < pos) {
    crc = CRC16(BUFF, pos-2);

    if(step == STEP_READ_COIL_STATUS) {

      if(BUFF[0] == 0x01 &&
        BUFF[1] == 0x01 &&
        BUFF[2] == 0x01) {
          
          RELAY_OnOff[0] = BUFF[3];
          step = STEP_FORCE_SINGLE_COIL;
          Serial.println("step = STEP_FORCE_SINGLE_COIL;");
        }
    }
  }
  
  // put your main code here, to run repeatedly:
  // Serial.println("QUERY");
  if(step == STEP_READ_COIL_STATUS) {
    Serial.println("step = STEP_READ_COIL_STATUS;");
    int ri = 0;
    resp[ri++] = 0x01;
    resp[ri++] = 0x01;
    resp[ri++] = 0x02;
    resp[ri++] = 0x58;
    resp[ri++] = 0x00;
    resp[ri++] = 0x08;
    crc = CRC16(resp, ri);
    //Serial.print("CRC=");
    //Serial.println(crc, HEX);
    resp[ri++] = 0xFF & (crc >> 0);
    resp[ri++] = 0xFF & (crc >> 8);
    digitalWrite(EN, 1);
    rx485.write(resp, ri);
    digitalWrite(EN, 0);  
  }

  if(0 < Serial.available())
  {
    char data[4] = { 0x00, };
    
    data[0] = Serial.read();
    data[1] = Serial.read();    

    if('1' <= data[0] && data[0] <= '8') {
      Serial.print("CMD:");
      Serial.println(data);
      forceSingleCoil(data[0] - 0x31, data[1] - 0x30);
    }
    
  }

  timeout += 1;
  if(timeout % 20 == 0) // per 2sec
  {
    Serial.print("RELAY : ");
    for(int i = 7; i>=0;i--) {
      Serial.print((char)('0' + ((RELAY_OnOff[0]>>i)&1)));
    }
    Serial.println();
  }
  delay(100);
  
  

}
