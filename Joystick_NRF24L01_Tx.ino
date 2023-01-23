#include <SPI.h>
#include <TimerOne.h>

//---------------------------------------------------------------
//                    Define Timer and Counter
//---------------------------------------------------------------
unsigned int Tick_01ms = 0, Tick_1ms = 0, Tick_10ms = 0, Tick_100ms = 0, Tick_1000ms = 0;
unsigned char Event_1ms = 0, Event_10ms = 0, Event_100ms = 0, Event_1000ms = 0;
unsigned int Button_Status = 0;
//---------------------------------------------------------------
//             Define Joystick Pin Mapping to Arduino
//---------------------------------------------------------------
// Store the Arduino pin associated with each input
// These pins are mapping to the Arduino UNO board
// PIN Define
const byte PIN_BUTTON_UP = 2;        // D2 = UP Button(Shield Button A)
const byte PIN_BUTTON_RIGHT = 3;     // D3 = RIGHT Button(Shield Button B)
const byte PIN_BUTTON_DOWN = 4;      // D4 = DOWN Button(Shield Button C)
const byte PIN_BUTTON_LEFT = 5;      // D5 = UP Button(Shield Button D)

const byte PIN_BUTTON_START = 6;     // D6 = STRAT Button(Shield Button E)
const byte PIN_BUTTON_SELECT = 7;    // D7 = SELECT Button (Shield Button F)
const byte PIN_BUTTON_STICK = 8;     // STICK Button(Shield Button K)

const byte PIN_ANALOG_X = 0;         // X Axis(Shield Stick X)
const byte PIN_ANALOG_Y = 1;         // Y Axis(Shield Stick Y)

// JoyStick Status
unsigned int Status_Button = 0;      // For Buttons
unsigned int Status_ANALOG_X = 0;    // X Axis(Shield Stick X)
unsigned int Status_ANALOG_Y = 1;    // Y Axis(Shield Stick Y)

//--------------------------------------------------------
//             Define NRF24L01 Command
//--------------------------------------------------------
#define  READ_REG          0x00       // Read Register Command
#define  WRITE_REG         0x20       // Write Register Command
#define  RD_RX_PLOAD       0x61       // Read RX Payload
#define  WR_TX_PLOAD       0xA0       // Write TX Payload
#define  FLUSH_TX          0xE1       // Transmit FIFO Command
#define  FLUSH_RX          0xE2       // Receive FIFO Command
#define  REUSE_TX_PLOAD    0xE3       // Re-load TX Payload
#define  NOP               0xFF       // Reserved

//--------------------------------------------------------
#define  RX_DR            0x40        
#define  TX_DS            0x20
#define  MAX_RT           0x10

//-----------------------------------------------------------------------
//            Define NRF24L01 Register Address(Refer to spec.)
//-----------------------------------------------------------------------
#define  CONFIG           0x00       // Configuration Register
#define  EN_AA            0x01       // Enable Auto Acknowledgment Function
#define  EN_RXADDR        0x02       // Enable RX Addresses
#define  SETUP_AW         0x03       // Setup of Address Widths
#define  SETUP_RETR       0x04       // Setup of Automatic Retransmission
#define  RF_CH            0x05       // RF Channel
#define  RF_SETUP         0x06       // RF Setup Register
#define  STATUS           0x07       // Status Register
#define  OBSERVE_TX       0x08       // Transmit Observer Register
#define  CD               0x09       // CD
#define  RX_ADDR_P0       0x0A       // Receive Address Data Pipe 0
#define  RX_ADDR_P1       0x0B       // Receive Address Data Pipe 1
#define  RX_ADDR_P2       0x0C       // Receive Address Data Pipe 2
#define  RX_ADDR_P3       0x0D       // Receive Address Data Pipe 3
#define  RX_ADDR_P4       0x0E       // Receive Address Data Pipe 4
#define  RX_ADDR_P5       0x0F       // Receive Address Data Pipe 5
#define  TX_ADDR          0x10       // Transmit Address(PTX device used only and LSByte is written first)
#define  RX_PW_P0         0x11       // Pipe 0 RX Width - Number of Bytes in RX Payload
#define  RX_PW_P1         0x12       // Pipe 1 RX Width - Number of Bytes in RX Payload
#define  RX_PW_P2         0x13       // Pipe 2 RX Width - Number of Bytes in RX Payload
#define  RX_PW_P3         0x14       // Pipe 3 RX Width - Number of Bytes in RX Payload
#define  RX_PW_P4         0x15       // Pipe 4 RX Width - Number of Bytes in RX Payload
#define  RX_PW_P5         0x16       // Pipe 5 RX Width - Number of Bytes in RX Payload
#define  FIFO_STATUS      0x17       // FIFO Status Register
//#define  DYNPD            0x1C

#define  Wireless_Key1_Num  0x55     // Key1
#define  Wireless_Key2_Num  0x55     // Key2

//------------------------------------------------------------------------
//                   Define Arduino Pin
//------------------------------------------------------------------------
// NRF24L01 in UNO
#define  CE      9
#define  CSN     10
//#define  IRQ     10

//-----------------------------------------------------------------------
#define  TX_ADR_WIDTH      5         // 5 bytes length for TX Address 
#define  TX_PLOAD_WIDTH    32        // 32 bytes length for TX Buffer

unsigned char TX_ADDRESS[TX_ADR_WIDTH] = 
{
  0x34, 0x43, 0x10, 0x10, 0x01
};

unsigned char RX_Buffer[TX_PLOAD_WIDTH] = {0};      // RX Buffer
unsigned char TX_Buffer[TX_PLOAD_WIDTH] = {0};      // TX Buffer

//-----------------------------------------------------------------------------------------------------------------
void setup()
{
// Timer 1 - Interrupt
  Timer1.initialize(100);     // 100us -> 10kHz
  Timer1.attachInterrupt(Timer1_ISR);       // Define ISR of Timer1

// Initial Joystick
  Initial_Joystick();
    
// Communication
  Serial.begin(9600);
  
// Pin  
  pinMode(CE, OUTPUT);
  pinMode(CSN, OUTPUT);
//  pinMode(IRQ, INPUT);
  
// SPI
  SPI.begin();
  delay(50);
  init_io();
  
  unsigned char Status_Data = SPI_Byte_Read(STATUS);  
    
  Serial.println("TX_Mode Start");
  Serial.print("Status = ");
  Serial.println(Status_Data, HEX);
  
  Initial_TX_Mode();
}

//-----------------------------------------------------------------------------------------------------------------
void loop()
{
  if(Event_100ms == 1)
  {
    Event_100ms = 0;
    Read_Joystick_Status();        // Read JoyStick Status
//    Report_Joystick_Status();      // Report Status via UART
    
    Send_Data(TX_Buffer);
  }

//  delay(100);    // 100 ms
}

//------------------------------------------------------------
//                           ISR
//------------------------------------------------------------
// Timer1 Interrupt -> 0.1ms 
void Timer1_ISR()
{
  Tick_01ms++;      // Minimum Tick = 0.1 ms
  if(Tick_01ms % 10 == 0)    // 0.1 ms*10 = 1 ms
  {
// Event - 1ms     
    Event_1ms = 1;
    Tick_1ms = 0;
  }  
  if(Tick_01ms % 100 == 0)    // 0.1 ms*100 = 10 ms
  {
// Event - 10ms     
    Event_10ms = 1;
    Tick_10ms = 0;
  } 
  if(Tick_01ms % 1000 == 0)    // 0.1 ms*1000 = 100 ms
  {
// Event - 100ms     
    Event_100ms = 1;
    Tick_100ms = 0;
  }   
  if(Tick_01ms % 10000 == 0)    // 0.1 ms*10000 = 1000 sec
  {
// Event - 1000ms 
    Event_1000ms = 1;
    Tick_1000ms = 0;
  }  

}

//------------------------------------------------------------
//                      Sub-Routine
//------------------------------------------------------------
void init_io(void)
{
//  digitalWrite(IRQ, 0);
  digitalWrite(CE, 0);
  digitalWrite(CSN, 0);
  
}

//------------------------------------------------------------------------------------
// Transmit data
void Send_Data(unsigned char *ptrInput_Data)
{
// Transmit "Input_Data"  
  unsigned char Status_Data = SPI_Byte_Read(STATUS);      // Read STATUS

  if(Status_Data & TX_DS)        // Condition?
  {
    SPI_Byte_Write(FLUSH_TX, 0);
    SPI_Write_Buf(WR_TX_PLOAD, ptrInput_Data, TX_PLOAD_WIDTH);
  }
  
  if(Status_Data & MAX_RT)       // Condition?
  {
    SPI_Byte_Write(FLUSH_TX, 0);
    SPI_Write_Buf(WR_TX_PLOAD, ptrInput_Data, TX_PLOAD_WIDTH);    
  }
  
  SPI_Byte_Write(WRITE_REG + STATUS, Status_Data);
 // delay(40);
}

//----------------------------------------------------------------------------
//                          Initialize NRF24L01
//----------------------------------------------------------------------------
void Initial_TX_Mode(void)
{
  digitalWrite(CE, 0);
  
  SPI_Write_Buf(WRITE_REG + TX_ADDR, TX_ADDRESS, TX_ADR_WIDTH);       // Set TX Address    
  SPI_Write_Buf(WRITE_REG + RX_ADDR_P0, TX_ADDRESS, TX_ADR_WIDTH);    // Set RX Pipe 0 Address 
  
  SPI_Byte_Write(WRITE_REG + EN_AA, 0x01);          // Enable Auto-ACK for Pipe 0
  SPI_Byte_Write(WRITE_REG + EN_RXADDR, 0x01);      // Enable Pipe 0
  SPI_Byte_Write(WRITE_REG + SETUP_RETR, 0x1A);     // Enable Re-Transmit
  
  SPI_Byte_Write(WRITE_REG + RF_CH, 40);          // Select Channel 40
  SPI_Byte_Write(WRITE_REG + RF_SETUP, 0x07);     // RF Setup: 0 dB, 1Mbps
  
  SPI_Byte_Write(WRITE_REG + CONFIG, 0x0E);       // Config register

  SPI_Write_Buf(WR_TX_PLOAD, TX_Buffer, TX_PLOAD_WIDTH);      // Send TX_Buffer[TX_PLOAD_WIDTH] = {0} to TX buffer for what?  
  
  digitalWrite(CE, 1);  
}

//---------------------------------------------------------------------
//                      Read Function for NRF24L01
//---------------------------------------------------------------------
// Read a Byte Data from Register
unsigned char SPI_Byte_Read(unsigned char Register)
{
  unsigned char Register_Data = 0;
  
  digitalWrite(CSN, 0);     // CSN -> Low, Start SPI
  
  SPI_RW(Register);         // Select NRF24L01 Register
  Register_Data = SPI_RW(0);       // Read Register Data
  
  digitalWrite(CSN, 1);     // CSN -> High, Stop SPI
  
  return Register_Data;
}

//-------------------------------------------------------------------
unsigned char SPI_Read_Buf(unsigned char Register, unsigned char *Buffer, unsigned char WIDTH)
{
  unsigned char Status = 0, i = 0;
  
  digitalWrite(CSN, 0);     // CSN -> Low, Start SPI
  
  Status = SPI_RW(Register);
  
  for(i = 0; i < WIDTH; i++)
  {
    Buffer[i] = SPI_RW(0);      // Read Register Data
  }
  
  digitalWrite(CSN, 1);     // CSN -> High, Stop SPI
  
  return Status;
}

//---------------------------------------------------------------------
//                      Write Function for NRF24L01
//---------------------------------------------------------------------
// Send a Byte data to SPI TX Register using SPI Function
unsigned char SPI_RW(unsigned char Input_Data)
{
  return SPI.transfer(Input_Data);
}

//-------------------------------------------------------------------
// Write a Byte Data to Register
unsigned char SPI_Byte_Write(unsigned char Register, unsigned char Input_Data)
{
  unsigned char Response = 0;
  
  digitalWrite(CSN, 0);     // CSN -> Low, Start SPI
  
  SPI_RW(Register);         // Select NRF24L01 Register
  SPI_RW(Input_Data);       // Transmit a byte data to NRF24L01
  
  digitalWrite(CSN, 1);     // CSN -> High, Stop SPI
  
  return Response;
}

//----------------------------------------------------------------------------------------------
unsigned char SPI_Write_Buf(unsigned char Register, unsigned char *Buffer, unsigned char WIDTH)
{
  unsigned char Status = 0, i = 0;
  
  digitalWrite(CSN, 0);     // CSN -> Low, Start SPI
  
  Status = SPI_RW(Register);
  
  for(i = 0; i < WIDTH; i++)
  {
    SPI_RW(*Buffer++);      // Write data to Buffer 
  }
  
  digitalWrite(CSN, 1);     // CSN -> High, Stop SPI
  
  return Status;
}

//---------------------------------------------------------------
//                   Joystick Sub-Routine
//---------------------------------------------------------------
// Initial Setting
void Initial_Joystick(void)
{
// UP
  pinMode(PIN_BUTTON_UP, INPUT);  
  digitalWrite(PIN_BUTTON_UP, HIGH);
  
// DOWN  
  pinMode(PIN_BUTTON_DOWN, INPUT);  
  digitalWrite(PIN_BUTTON_DOWN, HIGH);
  
// LEFT 
  pinMode(PIN_BUTTON_LEFT, INPUT);  
  digitalWrite(PIN_BUTTON_LEFT, HIGH);
  
// RIGHT  
  pinMode(PIN_BUTTON_RIGHT, INPUT);  
  digitalWrite(PIN_BUTTON_RIGHT, HIGH);
  
// SELECT  
  pinMode(PIN_BUTTON_SELECT, INPUT);  
  digitalWrite(PIN_BUTTON_SELECT, HIGH); 
  
// START 
  pinMode(PIN_BUTTON_START, INPUT);  
  digitalWrite(PIN_BUTTON_START, HIGH);   
  
// STICK 
  pinMode(PIN_BUTTON_STICK, INPUT);  
  digitalWrite(PIN_BUTTON_STICK, HIGH); 
}

//----------------------------------------------------------------------------
// Report JoyStick Status to via UART
void Report_Joystick_Status(void)
{
  Serial.print("UP:");
  Serial.print(digitalRead(PIN_BUTTON_UP)); 
  Serial.print(" ");
  
  Serial.print("DOWN:");
  Serial.print(digitalRead(PIN_BUTTON_DOWN));
  Serial.print(" ");
  
  Serial.print("LEFT:");
  Serial.print(digitalRead(PIN_BUTTON_LEFT));
  Serial.print(" ");
  
  Serial.print("RIGHT:");
  Serial.print(digitalRead(PIN_BUTTON_RIGHT));
  Serial.print(" ");  

  Serial.print("X:");
  Serial.print(analogRead(PIN_ANALOG_X));
  Serial.print(" ");
  
  Serial.print("Y:");
  Serial.print(analogRead(PIN_ANALOG_Y));
  Serial.print(" ");  
  
  Serial.print("SELECT:");
  Serial.print(digitalRead(PIN_BUTTON_SELECT));
  Serial.print(" ");
  
  Serial.print("START:");
  Serial.print(digitalRead(PIN_BUTTON_START));
  Serial.print(" ");  
  
  Serial.println();
}

//----------------------------------------------------------------------------
// Read Joystick Status
void Read_Joystick_Status(void)
{ 
  Status_ANALOG_X = analogRead(PIN_ANALOG_X);    // X Axis(Shield Stick X)
  Status_ANALOG_Y = analogRead(PIN_ANALOG_Y);    // Y Axis(Shield Stick Y)
  
  Status_Button = (digitalRead(PIN_BUTTON_SELECT) << 15) + (digitalRead(PIN_BUTTON_START) << 14)
                   + (digitalRead(PIN_BUTTON_LEFT) << 13) + (digitalRead(PIN_BUTTON_DOWN) << 12)
                   + (digitalRead(PIN_BUTTON_RIGHT) << 11) + (digitalRead(PIN_BUTTON_UP) << 10);      // For Buttons

// Load data into TX_Buffer  
  TX_Buffer[0] = (unsigned char) (Status_ANALOG_X);    
  TX_Buffer[1] = (unsigned char) (Status_ANALOG_X >> 8);   
  
  TX_Buffer[2] = (unsigned char) (Status_ANALOG_Y);    
  TX_Buffer[3] = (unsigned char) (Status_ANALOG_Y >> 8);    
  
  TX_Buffer[4] = Status_Button >> 8;

// NRF24L01 Key1 and Key2  
  TX_Buffer[5] = Wireless_Key1_Num;
  TX_Buffer[6] = Wireless_Key2_Num;
}





