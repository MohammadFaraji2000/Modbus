#include "stm32f3xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//-------------------------------------------Memmory----------------------------------------------
// Here we define a Memmory for saving data, read and write registers and 
// coils.
// The Structure of this Memmory is Like this:
//  [0 ... 199] ---> Discrete Input Memmory.
//	[0 ... 199] ---> Coil Memmory.
//	[0 ... 399] ---> input Registers Memmory.
//	[0 ... 399] ---> Holding Register Memmory.
extern char DIMemmory[100];			//Discrete Input Memmory.
extern char CMemmory[100];			//Coil Memmory.
extern char IRMemmory[400];			//Input Registers Memmory.
extern char HRMemmory[400];			//Holding Register Memmory.

char ExceptionStatus[32];						//
int EventCounter;						//This is my event counter which is incremented once for
						// each seccessful message complesion
char Status[5];									//This is my Status word Which is FFFF when a previosly
						// issued program command is still being processed by the remote device. Otherwise,
						// the status word will be all zeros.
char Events[130];					//We store our event report here.
int MessageCounter;				//here we count contains the couantity of messages processed by.
						// the remote device.
//----------------------------incoming data--------------------------
extern UART_HandleTypeDef huart1;
//-------------incoming information from master/slave----------------
extern char ModbusHandleID[2];
//-------------------------Final Output Data-------------------------
char Final_Output[300];  
//-------------------------Protocol description----------------------
//-------------------------------------------------------------------
// In this function we will one bit from object Byte. I check this, it works correctly.
uint16_t changeBit(uint8_t object,uint8_t subject,uint8_t bitNumber){
	uint8_t a= 1 << bitNumber;
	if( ((object & a) ==0)& ((subject & a) == 1))			//if the bit is zero and we want to make it one
		return object |= subject & a;
	else if(((object & a) ==1) & ((subject & a) == 1))	//if the bit is one and we want to make it one
		return object |= subject & a;
	else if(((object & a) ==0) & ((subject & a) == 0))		//if the bit is zero and we want to make it zero
		return object &= subject & (~a);
	else if( ((object & a) ==1) & ((subject & a) == 0) )		//if the bit is one and we want to make it zero
		return object &= subject & (~a);
}


//
uint16_t attachByte(uint8_t LowByte, uint8_t HighByte){
	uint16_t object;
	return object|= (HighByte<<8)|(LowByte);
}



//---------------------------help Functions--------------------------

/**
* @brief
*	Exception_Engine:
* In this function we want to make exceptions. We do not handle them here.
* The first input, funcNum, is the function which make the exception. we add 
* this input with 0x80. The next input is Exception_Code which shows what
* have been ocured.
*/
void Exception_Engine(char funcNum[], char Exception_Code[]){
	char Output[10];
	for(int i=0;i<10;i++){
		Output[i]=0;
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,funcNum);
	strcat(Output,Exception_Code);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
	static int f=0;
	f++;
	if(f>16)
		f=0;
	ExceptionStatus[2*f-2]=Exception_Code[0];
	ExceptionStatus[2*f-1]=Exception_Code[1];
}


/**
* @brief
*	Function1: Read Coils, Physical Coils,	Bit access, Data Access.
*	Inputs: StAddress is Abbreviation of (start address). in this 
*					function we start from this address to QuCoils unit 
*					latter.
*					QuCoils is Abbreviation of Quantity of Coils. tjis is number
*					coils we have to send.
*	Outputs:	here we dont return datas, we just return Exceptions. datas
*						are globally changing and after finishing this function in 
*						the primery functions it will be send.
*/
void function1(int StAddress, int QuCoils){
	char Output[110];
	int CN=0;   //Coil Counter.
	for(int i=0;i<110;i++){
		Output[i]=0;
	}
	if(QuCoils>100 | QuCoils<1){	//if 1 <= Quantity of Outputs <= 100 
		// was not correct we build ExceptionCode = 01.
		Exception_Engine("81","03");
	}
	else if(StAddress < 0 & (StAddress + QuCoils/8) >= 100){	
			//	if (starting Address) == OK AND (Starting Address + Quantity of Outputs) == OK
			//	conditions was not Correct we build ExceptionCode = 02.	
			Exception_Engine("81","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"01");	// we write function code: 01 in 0 address;
	char r[4];
	sprintf(r,"%d",QuCoils);
	strcat(Output,r);
	char temp[100];
	int j=0;
	for(CN;CN < QuCoils;CN++){
		if(StAddress<100){								//if we are in Coil Memmory region
			temp[j]=CMemmory[StAddress];
			j++;
			StAddress++;
		}
	}
	strcat(Output,temp);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}

/**
* @brief
*	Function2: Read Discrete inputes, Physical Discrete Inputs, Bit access,	Data Access.
*	Inputs: StAddress is Abreviation of (Start Address), we start from this address to 
*					QuInput unit later in Discrete input Memmory.
*	Outputs: Here we dont return final resoult. the variable Final_resoult
*						is a global variable and here we just make some changes in its 
*						datas. here we just return exceptions.
*/
void function2(int StAddress, int QuInput){
	char Output[110];
	for(int i=0;i<110;i++){
		Output[i]=0;
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"02");
	int IN=0;   //Discrete Input Counter.
	if(QuInput>200 | QuInput<1){	//if 0x0001 <= Quantity of inputs <= 0x07D0 
		// was not correct we build ExceptionCode = 01.
		Exception_Engine("02","03");
	}
	else if( StAddress <= 0000 & (StAddress + QuInput/8) >= 100){	
			//	if (starting Address) == OK AND (Starting Address + Quantity of Outputs) == OK
			//	conditions was not Correct we build ExceptionCode = 02.
		Exception_Engine("82","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"02");	// we write function code: 01 in 0 address;
	char r[4];
	sprintf(r,"%d",QuInput);
	strcat(Output,r);
	char temp[100];
	int j=0;
	for(IN;IN < QuInput;IN++){
		if(StAddress<100){								//if we are in Coil Memmory region
			temp[j]=CMemmory[StAddress];
			j++;
			StAddress++;
		}
	}
	strcat(Output,temp);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}

/**
* @brief
*	Function3: Read Holding Registers, Internal Registers, 16 Bits Access, Data Access.
* inputs: StAddress is Abreviation of Start Address for Holding registers Memmory
*					we start reading registers from this address to QuRegisters later.
*					QuRegister is Abreviation of Quantity of Registers.
*
*					in this function instead of using MBMemmory we used HRMemmory[4000].
*					because MBmemmory is 16 bit Mem and we cant send 16 bit Registers to
*					8 bit Memmory. the Low Byte of each Register is firt and its high Byte
*					is next.
*					StAddress must be start from zero. and 
*
* Output: Here we dont return final resoult. the variable Final_resoult
*						is a global variable and here we just make some changes in its 
*						datas. here we just return exceptions.
*/
void function3(int StAddress, int QuRegisters){
	int j=0;					// j is our Byte Counter for FinalOutput
	char Output[260];
	for(int i=0;i<260;i++){
		Output[i]=0;
	}
	if(QuRegisters>100 | QuRegisters<1){	//if 0001 <= Quantity of Registers <= 200 
		// was not correct we build ExceptionCode = 03.
		Exception_Engine("83","03");
	}
	else if( StAddress <= 0000 & (StAddress + 2*QuRegisters) >= 400){	
			//	if (starting Address) == OK AND (Starting Address + Quantity of Registers) == OK
			//	conditions was not Correct we build ExceptionCode = 02.	
		Exception_Engine("83","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"03");
	char r[4];
	sprintf(r,"%d",2*QuRegisters);
	strcat(Output,r);		// In next Byte we write (Byte Count) field.
	char data[250];
	for(int i=0;i<250;i++){
		data[i]=0;
	}
	for(j; j<(QuRegisters * 2); j++){
		if(StAddress<400){
			data[j]=HRMemmory[StAddress];				//here we move Low Byte of Register.
			j++;
			StAddress++;
			data[j]=HRMemmory[StAddress];		//here we move high byte of Register.
		}
		StAddress++;
	}
	strcat(Output,data);
	HAL_UART_Transmit(&huart1,Output,sizeof(Output),100);
}


/**
* @brief
*	Function4: Read Input Registers, Physical Input Registers, 16 Bits Access, Data Access.
* inputs: StAddress is Abreviation of Start Address for Holding registers Memmory
*					we start reading registers from this address to QuRegisters later.
*					QuRegister is Abreviation of Quantity of Registers.
*
*					in this function instead of using MBMemmory, we used IRMemmory[4000].
*					because MBmemmory is 16 bit Mem and we cant send 16 bit Registers to
*					8 bit Memmory. the Low Byte of each Register is firt and its high Byte
*					is next.
*					StAddress must be start from zero. and 
*
* Output: Here we dont return final resoult. the variable Final_resoult
*						is a global variable and here we just make some changes in its 
*						datas. here we just return exceptions.
*/
void function4(int StAddress, int QuRegisters){
	int j=0;					// j is our Byte Counter for FinalOutput
	char Output[260];
	for(int i=0;i<260;i++){
		Output[i]=0;
	}
	if(QuRegisters>2000 | QuRegisters<1){	//if 0x0001 <= Quantity of Registers <= 0x07D0 
		// was not correct we build ExceptionCode = 03.
		Exception_Engine("84","03");
	}
	else if( StAddress <= 0 & (StAddress + 2*QuRegisters) >= 400){	
			//	if (starting Address) == OK AND (Starting Address + Quantity of Registers) == OK
			//	conditions was not Correct we build ExceptionCode = 02.	
		Exception_Engine("84","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"04");
	char r[4];
	sprintf(r,"%d",2*QuRegisters);
	strcat(Output,r);		// In next Byte we write (Byte Count) field.
	char data[250];
	for(int i=0;i<250;i++){
		data[i]=0;
	}
	for(j; j<(QuRegisters * 2); j++){
		if(StAddress<400){
			data[j]=IRMemmory[StAddress];				//here we move Low Byte of Register.
			j++;
			StAddress++;
			data[j]=IRMemmory[StAddress];		//here we move high byte of Register.
		}
		StAddress++;
	}
	strcat(Output,data);
	HAL_UART_Transmit(&huart1,Output,sizeof(Output),100);
}

/**
* @brief
*	Function5: Write Single Coil, Physical coils, Bit Access, Data Access.
*	inputs: Output_register, Output Value 16 bits 
*	Output: Here we dont return final resoult. the variable Final_resoult
*						is a global variable and here we just make some changes in its 
*						datas. here we just return exceptions.
*/
void function5(int Output_Address, char Output_Value[]){
	char Output[10];
	for(int i=0;i<10;i++){
		Output[i]=0;
	}
	if((strcmp(Output_Value,"0000")==0) & ((strcmp(Output_Value,"FF00")==0))){
		Exception_Engine("85","03");
	}
	else if( (Output_Address >100) & (Output_Address <= 0)){	
			//	if (starting Address) == OK AND (Starting Address + Quantity of Registers) == OK
			//	conditions was not Correct we build ExceptionCode = 02.	
		Exception_Engine("85","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"05");
	char r[4];
	sprintf(r,"%d",Output_Address);
	strcat(Output,r);
	if(Output_Value=="FF00"){
		char s[1]="1";
		CMemmory[Output_Address]=s[0];
	}
	else if(Output_Value=="0000"){
		char s[1]="0";
		CMemmory[Output_Address]=s[0];
	}
	strcat(Output,Output_Value);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}

/**
* @brief
*	Function6: Write Single Register, Physical Output Registers, 16 Bits Access, Data Access.
*/
void function6(int Register_Address, char Register_Value[]){
	char Output[13];
	for(int i=0;i<10;i++){
		Output[i]=0;
	}
	if((strcmp(Register_Value,"0000")<0),(strcmp(Register_Value,"FFFF")>0)){
		Exception_Engine("86","03");
	}
	else if( (Register_Address >400) & (Register_Address < 0)){	
		Exception_Engine("86","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"06");
  int RA=Register_Address;
	char RegAdd[5];
	if(Register_Address<100)
		strcat(Output,"00");
	sprintf(RegAdd,"%d",Register_Address);
	strcat(Output,RegAdd);
	strcat(Output,Register_Value);
	for(int j=0;j<4;j++){
		HRMemmory[RA]=Register_Value[j];
		RA++;
	}
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}


/**
* @brief
*	Function7:  Read Exception status, Diagnostics.
*/
void function7(){
	char Output[40];
	for(int i=0;i<40;i++){
		Output[i]=0;
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"07");
	strcat(Output,ExceptionStatus);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}

/**
* @brief
*	Function11: Get Com event counter, Diagnostics.
*	Input: None.
* Output: ExceptionCode=0x04 if occurs.
*				Here we just send status register and EventCounter to final_Output register.
*/
char function11(){
	char Output[13];
	for(int i=0;i<10;i++){
		Output[i]=0;
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"11");
	strcat(Output,Status);
	char s[5];
	sprintf(s,"%d",EventCounter);
	strcat(Output,s);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}

/**
* @brief
*	Function12: Get Com Event Log, Diagnostics.
*/
void function12(){
	char Output[152];
	for(int i=0;i<152;i++){
		Output[i]=0;
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"12");
	int N;
	char g[5];
	if(MessageCounter<65){
		N=2*MessageCounter;
	}
	else{
		N=130;
	}
	N+=12;
	strcat(Output,"0");
	if(N<100){
		strcat(Output,"0");
		if(N<10){
			strcat(Output,"0");
		}
	}
	sprintf(g,"%d",N);
	g[4]=0;
	strcat(Output,g);
	strcat(Output,Status);
	char s[5];
	sprintf(s,"%d",EventCounter);
	s[4]=0;
	if(EventCounter<1000){
		strcat(Output,"0");
		if(EventCounter<100){
			strcat(Output,"0");
			if(EventCounter<10){
				strcat(Output,"0");
			}
		}
	}
	strcat(Output,s);
	char b[5];
	sprintf(b,"%d",MessageCounter);
	b[4]=0;
	if(MessageCounter<1000){
		strcat(Output,"0");
		if(MessageCounter<100){
			strcat(Output,"0");
			if(MessageCounter<10){
				strcat(Output,"0");
			}
		}
	}
	strcat(Output,b);
	strcat(Output,Events);
	
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}


/**
* @brief
*	Function16:Write Multiple Registers, Physical Output Registers,	16 Bits access,	Data Access.
*	inputs: St_Address is abreviation of start address, we start from this address in Input Register
*					Memmory to Qu_Registers home later.
*					Qu_register is abreviation of Quantity of Registers, it is equal to N.
*					Registers_Value is an 8bit array which incoming registers is saved ther fist home is low
*					Byte and second one is High Byte.
*
*					Byte_Count=2 * N.
* Outputs: we just set the Final_Output array and if there was an exception we return it if not return 0.
*/
void function16(int St_Address, int Qu_Register, char Registers_Value[]){
	char Output[74];
	for(int i=0;i<70;i++){
		Output[i]=0;
	}
	int i=St_Address, j=0;
	
	if((Qu_Register > 0) & (Qu_Register >= 200)){	//if 0x0001 <= Quantity of Registers <= 0x07D0 
		// was not correct we build ExceptionCode = 03.
		Exception_Engine("96","03");
	}
	else if( (St_Address <0) & (St_Address > 400)){	
			//	if (starting Address) == OK AND (Starting Address + Quantity of Registers) == OK
			//	conditions was not Correct we build ExceptionCode = 02.	
		Exception_Engine("96","02");
	}
	strcat(Output,ModbusHandleID);
	strcat(Output,"16");
	for(i; i < (St_Address+ 4 * Qu_Register); i++){
		IRMemmory[i]= Registers_Value[j];				//Moving Bytes of Register
		j++;
	}
	char a[5],b[5];
	sprintf(a,"%d",St_Address);
	sprintf(b,"%d",Qu_Register);
	strcat(Output,a);
	strcat(Output,b);
	HAL_UART_Transmit(&huart1,Output,strlen(Output),100);
}




//------------------------------Functions----------------------------
int MBProcess(char RxBuffer[]){
	char ID[3];
	ID[0]=RxBuffer[0];
	ID[1]=RxBuffer[1];
	ID[2]=0;
	static int counter=0;
	if(counter>65)
		counter=0;
	if(strcmp(ID,ModbusHandleID)==0){
		char function[3];
		function[0]=RxBuffer[2];
		function[1]=RxBuffer[3];
		function[2]=0;
		if(strcmp(function,"01")==0){
			strcpy(Status,"FFFF");
			char StAdr[5],QuCoil[5];
			for(int j=0;j<5;j++)
				StAdr[j]=RxBuffer[j+4];
			StAdr[4]=0;
			for(int j=0;j<5;j++)
				QuCoil[j]=RxBuffer[j+8];
			QuCoil[4]=0;
			int StAddress=atoi(StAdr);
			int QuCoils=atoi(QuCoil);
			function1(StAddress, QuCoils);
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"02")==0){
			strcpy(Status,"FFFF");
			char StAdr[5],QuCoil[5];
			for(int j=0;j<5;j++)
				StAdr[j]=RxBuffer[j+4];
			StAdr[4]=0;
			for(int j=0;j<5;j++)
				QuCoil[j]=RxBuffer[j+8];
			QuCoil[4]=0;
			int StAddress=atoi(StAdr);
			int QuCoils=atoi(QuCoil);
			function2(StAddress, QuCoils);
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"03")==0){
			strcpy(Status,"FFFF");
			char StAdr[5],QuRegister[5];
			for(int j=0;j<5;j++)
				StAdr[j]=RxBuffer[j+4];
			StAdr[4]=0;
			for(int j=0;j<5;j++)
				QuRegister[j]=RxBuffer[j+8];
			QuRegister[4]=0;
			int StAddress=atoi(StAdr);
			int QuRegisters=atoi(QuRegister);
			function3(StAddress, QuRegisters);
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			MessageCounter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"04")==0){
			strcpy(Status,"FFFF");
			char StAdr[5],QuRegister[5];
			for(int j=0;j<5;j++)
				StAdr[j]=RxBuffer[j+4];
			StAdr[4]=0;
			for(int j=0;j<5;j++)
				QuRegister[j]=RxBuffer[j+8];
			QuRegister[4]=0;
			int StAddress=atoi(StAdr);
			int QuRegisters=atoi(QuRegister);
			function4(StAddress, QuRegisters);
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			MessageCounter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"05")==0){
			strcpy(Status,"FFFF");
			char OutAdr[5],Value[5];
			for(int j=0;j<5;j++)
				OutAdr[j]=RxBuffer[j+4];
			OutAdr[4]=0;
			for(int j=0;j<5;j++)
				Value[j]=RxBuffer[j+8];
			Value[4]=0;
			int Output_Address=atoi(OutAdr);
			function5(Output_Address, Value);
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"06")==0){
			strcpy(Status,"FFFF");
			char OutAdr[5],Value[5];
			for(int j=0;j<5;j++)
				OutAdr[j]=RxBuffer[j+4];
			OutAdr[4]=0;
			for(int j=0;j<5;j++)
				Value[j]=RxBuffer[j+8];
			Value[4]=0;
			int Output_Address=atoi(OutAdr);
			//int Register_Value=atoi(Value);
			function6(Output_Address, Value);
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"07")==0){
			strcpy(Status,"FFFF");
			function7();
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"11")==0){
			function11();
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"12")==0){
			strcpy(Status,"FFFF");
			function12();
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else if(strcmp(function,"16")==0){
			strcpy(Status,"FFFF");
			char StAdr[4],QuReg[4],ByteCnt[3],Value[123];
			for(int j=0;j<4;j++){											//read starting address
				StAdr[j]=RxBuffer[j+4];
			}
			StAdr[4]=0;
			for(int j=0;j<4;j++){											//read Quantity of register
				QuReg[j]=RxBuffer[j+8];
			}
			QuReg[4]=0;
			ByteCnt[0]=RxBuffer[12];									//High digit of Byte Count
			ByteCnt[1]=RxBuffer[13];			//Low digit of Byte Count
			ByteCnt[2]=0;
			HAL_UART_Transmit(&huart1,ByteCnt,2,10);
			int Start_Address=atoi(StAdr);
			int QuantityOFRegister=atoi(QuReg);
			int ByteCount=atoi(ByteCnt);
			ByteCount =ByteCount*2;
			if(QuantityOFRegister>123){
				if(ByteCount==QuantityOFRegister*2){
					Exception_Engine("96","03");
				}
			}
			
			for(int j=0;j<ByteCount;j++){							//READ Values of Registers
				Value[j]=RxBuffer[j+14];
				//HAL_UART_Transmit(&huart1,"This shut is Ok. ",17,10);
			}
			Value[ByteCount]=0;
			HAL_UART_Transmit(&huart1,"\r\n Value is: ",13,10);
			HAL_UART_Transmit(&huart1,Value,sizeof(Value),10);
			function16(Start_Address,QuantityOFRegister,Value);
			MessageCounter++;
			Events[2*counter]=function[0];
			Events[2*counter+1]=function[1];
			counter++;
			EventCounter++;
			HAL_UART_Transmit(&huart1,"\r\n",2,10);
		}
		else{
			int funcnum=atoi(function)+80;
			char func[3];
			sprintf(func,"%d",funcnum);
			Exception_Engine(func,"01");
		}
	}
	strcpy(Status,"0000");
	return 0;
}



///////////////////////////////////////////////////////////////////////////////////////

