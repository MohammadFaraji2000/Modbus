#include "stm32f3xx_hal.h"
//--------------------------------PDU--------------------------------
// PDU field is included [function field] and [data field]
// MODBUS Protocol defines three PDUs.They are:

//----------------------------Modbus Handle--------------------------
/*typedef struct{
	uint8_t              ID;
	uint8_t							 Run Indicator Status;
	//uint32_t            F_CPU_CLK;
	uint32_t             BAUD_RATE;
	uint32_t             PARITY;	
	uint8_t              STOP_BIT;
	uint8_t              DATALENG;
	GPIO_TypeDef        *RS485_PORT;
	uint32_t             RS485_PIN;	
	UART_HandleTypeDef  *UART;
	//void (*interrupt_routin)(void); 	
}ModbusHandle;
*/
//
//-------------------------Modbus data Model-------------------------
// 
char Discretes_input;  			//Single bit		Read Only
char coils;									//Single bit		Read Write
uint16_t Input_Registers;		//16 bit Word		Read Only
uint16_t Holding_Registers;	//16 bit Word		Read Write

extern char ModbusHandleID[2];
extern char Final_Output[64];
extern UART_HandleTypeDef huart1;
extern uint8_t RxBuffer[64];
//------------------------------Functions----------------------------
uint16_t attachByte(uint16_t object, uint8_t LowByte, uint8_t HighByte);
uint16_t changeBit(uint8_t object,uint8_t subject,uint8_t bitNumber);
uint8_t function1(uint16_t StAddress, uint16_t QuCoils);
void MBProcess();

