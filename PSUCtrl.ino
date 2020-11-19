#define PIN_AC_OK PA0
#define PIN_PWR_GOOD PA1
#define PIN_PS_PRESENT PA2
#define PIN_PS_INTERRUPT_L PA3
#define PIN_ISHARE PA4
//#define SCL PB8
//#define SDA PB9


#include <SoftWire.h>
SoftWire SWire(PB8, PB9, SOFT_STANDARD); //scl sda delay
#define Wire SWire

/*
 * SOFT_FAST = 400KHz
 * SOFT_STANDARD = 100KHz
 */
 
byte error;
byte address=0x58;

//{ Type Definition
typedef struct
{
  int16_t base : 11;
  int16_t mantissa : 5;
} linear11_t;

typedef struct
{
  byte lsb;
  byte msb;
} twobytes;

typedef union
{
  linear11_t linear;
  uint16_t raw;
  twobytes tbdata;
} linear11_val_t;

//}

//{ Function Prototype
byte hex_char_to_int(char ch);
double linear11(byte msb, byte lsb);
double linear11(twobytes tbval);
double linear16(byte msb, byte lsb);
double linear16(twobytes tbval);
double directf(byte msb, byte lsb);
double directf(twobytes tbval);
bool CommandExec0(byte DeviceAddr, byte Command);
byte CommandExec1(byte DeviceAddr, byte Command);
twobytes CommandExec2(byte DeviceAddr, byte Command);

//}

//{ Register Values
byte CMDCode; //Command to send
byte DataBytes; //PMBus data length
bool ShowStatus=false;
double Vo        = 0;
double Vi        = 0;
double Io        = 0;
double P         = 0;

int16_t m=0xFFFF;
int16_t b=0xFFFF;
int16_t R=0xFFFF;
int VOUT_MODE=-9;

bool ATRead;
bool ATWrite;
//}

void SendByteAndPrint(byte data, int dataBytes)
{
  uint8_t b[1] = {data};
  Wire.beginTransmission(address);
  Wire.write(b,1);
  Wire.endTransmission();
  Wire.requestFrom(address, dataBytes,true);
  while(Wire.available()>0)
  {
    byte c=Wire.read();
    Serial.print(c,HEX);
    Serial.print(" ");
  }
  Serial.println();
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(100000L);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(100);
  Serial.println();
  Serial.println("MCU Initializing...");
  /*
  Wire.beginTransmission(address);
  error = Wire.endTransmission();
  delay(100);
  if (error==0){
    Serial.print("I2C device found at 0x");
    Serial.println(address,HEX);
  }
  else {
    Serial.print("Unknown error ");
    Serial.print(error);
    Serial.print(" at 0x");
    Serial.println(address,HEX);
  }
  delay(10);
  */
  //pinMode(PIN_ISHARE,analogRead);
  byte* temp1;
  temp1=CommandExecN(address, 0x30,6);
  Serial.print("0x");
  if (*(temp1+0)<=0xF) Serial.print("0");
  Serial.print(*(temp1+0),HEX);
  Serial.print(" 0x");
  if (*(temp1+1)<=0xF) Serial.print("0");
  Serial.print(*(temp1+1),HEX);
  Serial.print(" 0x");
  if (*(temp1+2)<=0xF) Serial.print("0");
  Serial.print(*(temp1+2),HEX);
  Serial.print(" 0x");
  if (*(temp1+3)<=0xF) Serial.print("0");
  Serial.print(*(temp1+3),HEX);
  Serial.print(" 0x");
  if (*(temp1+4)<=0xF) Serial.print("0");
  Serial.print(*(temp1+4),HEX);
  Serial.print(" 0x");
  if (*(temp1+5)<=0xF) Serial.print("0");
  Serial.println(*(temp1+5),HEX);
  

  
  m=*(temp1+0)<<8 | *(temp1+1);
  b=*(temp1+2)<<8 | *(temp1+3);
  R=*(temp1+4)<<8 | *(temp1+5);
  Serial.print("m=");
  Serial.print(m);
  Serial.print(" b=");
  Serial.print(b);
  Serial.print(" R=");
  Serial.println(R);
  
  Serial.println("MCU Ready...");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  if (ShowStatus){
  Serial.print("AC_OK = ");
  Serial.print(digitalRead(PIN_AC_OK));
  
  Serial.print(",PWR_GOOD = ");
  Serial.print(digitalRead(PIN_PWR_GOOD));
  
  Serial.print(",PS_PRESENT = ");
  Serial.print(digitalRead(PIN_PS_PRESENT));
  
  Serial.print(",PS_INTERRUPT_L = ");
  Serial.print(digitalRead(PIN_PS_INTERRUPT_L));
  
  Serial.print(",ISHARE = ");
  Serial.println(analogRead(PIN_ISHARE));
  delay(450);
  }
  delay(50);
  while (Serial.available()) 
  {//从串口中读取数据
    String command="";
	byte bin=0;
	long timeout;
	timeout=100;
	while ((bin!=0x0D)&&(bin!=0x0A))
	{
		while (!Serial.available())
		{
			if (timeout<=0) break;
			delay(1);
			timeout-=1;
			}
		if (timeout<=0) break;
	    bin=Serial.read();
		command+=(char)bin;
	}
	Serial.print("Serial0> ");
	Serial.println(command);
	if (timeout<=0) {Serial.println("Command Input Timeout"); continue;}

	command.trim();
	command.toUpperCase();
	
	if (command.substring(0,3).equals("AT+"))
	{
		String opc;
		String opv;
		if (command.endsWith("?"))
		{
			opc=command.substring(3,command.length()-1);	
			ATRead=true;
			ATWrite=false;

		}else{
			ATRead=false;
			int spos=command.indexOf("=");
			if (spos!=-1)
			{
				ATWrite=true;
				opc=command.substring(3,spos);
				opv=command.substring(spos+1);
				opv.trim();
			}else{
				ATWrite=false;
				opc=command.substring(3);
			}
		}
		if (opc.equals("")||opc.charAt(0)==0x0D||opc.charAt(0)==0x0A) continue;
		Serial.print("Command:");
		Serial.print(opc);
		if (!(opv.equals("")||opv.charAt(0)==0x0D||opv.charAt(0)==0x0A)){
			Serial.print(" Data:");
			Serial.print(opv);
		}
		Serial.print(" Read:");
		Serial.print(ATRead);
		Serial.print(" Write:");
		Serial.print(ATWrite);
		Serial.println();
		if (opc.equals("COMMAND"))
		{
			if (ATRead) {Serial.print("Command Code = "); Serial.println(CMDCode);}
			if (ATWrite) {
				if (opv.startsWith("0X")){
					CMDCode=hex_char_to_int(opv.charAt(2))<<4 | hex_char_to_int(opv.charAt(3));
				}else{
					CMDCode=opv.toInt();
				}
			}
		}else if (opc.equals("DATABYTES")){
			if (ATRead) {Serial.print("Data Bytes = "); Serial.println(DataBytes);}
			if (ATWrite) {
				if (opv.startsWith("0X")){
					DataBytes=hex_char_to_int(opv.charAt(2))<<4 | hex_char_to_int(opv.charAt(3));
				}else{
					DataBytes=opv.toInt();
				}
			}
		}else if (opc.equals("SHOWSTATUS")){
			if (ATRead) {Serial.print("Show Status = "); Serial.println(ShowStatus);}
			if (ATWrite) {
				if (opv.startsWith("0X")){
					ShowStatus=(hex_char_to_int(opv.charAt(2))<<4 | hex_char_to_int(opv.charAt(3)))==1?true:false;
				}else if (opv.equals("TRUE")){
					ShowStatus=true;
				}else if (opv.equals("FALSE")){
					ShowStatus=false;
				}else{
					ShowStatus=opv.toInt()==1?true:false;
				}
			}
		}else if (opc.equals("")){
			
		}else if (opc.equals("")){
			
		}else if (opc.equals("")){
			
		}else if (opc.equals("")){
			
		}else if (opc.equals("")){
			
		}else if (opc.equals("")){
			
		}else if (opc.equals("EXECUTE")){
			twobytes tbval;
			byte* tbres;
			Serial.print("CMDCode = 0x");
			if (CMDCode<=0xF) Serial.print("0");
			Serial.print(CMDCode,HEX);
			Serial.print(" Data Bytes = ");
			Serial.print(DataBytes);
			Serial.println();
			switch (CMDCode)
			{
				case 0x30: //Coefficients
					tbres=CommandExecN(address, 0x30,6);
					Serial.print("0x");
					if (*(tbres+0)<=0xF) Serial.print("0");
					Serial.print(*(tbres+0),HEX);
					Serial.print(" 0x");
					if (*(tbres+1)<=0xF) Serial.print("0");
					Serial.print(*(tbres+1),HEX);
					Serial.print(" 0x");
					if (*(tbres+2)<=0xF) Serial.print("0");
					Serial.print(*(tbres+2),HEX);
					Serial.print(" 0x");
					if (*(tbres+3)<=0xF) Serial.print("0");
					Serial.print(*(tbres+3),HEX);
					Serial.print(" 0x");
					if (*(tbres+4)<=0xF) Serial.print("0");
					Serial.print(*(tbres+4),HEX);
					Serial.print(" 0x");
					if (*(tbres+5)<=0xF) Serial.print("0");
					Serial.println(*(tbres+5),HEX);
					m=*(tbres+0)<<8 | *(tbres+1);
					b=*(tbres+2)<<8 | *(tbres+3);
					R=*(tbres+4)<<8 | *(tbres+5);
					Serial.print("m=");
					Serial.print(m);
					Serial.print(" b=");
					Serial.print(b);
					Serial.print(" R=");
					Serial.println(R);
					break;
				case 0x88: //Vin
					tbval=CommandExec2(address, CMDCode);
					Serial.print("Raw Data = 0x");
					if (tbval.lsb<=0xF) Serial.print("0");
					Serial.print(tbval.lsb,HEX);
					Serial.print(" 0x");
					if (tbval.msb<=0xF) Serial.print("0");
					Serial.println(tbval.msb,HEX);
					Vi=linear11(tbval);
					Serial.print("Vin = ");
					Serial.print(Vi);
					Serial.println(" V");
					break;
				case 0x8B: //Vout
					tbval=CommandExec2(address, CMDCode);
					Serial.print("Raw Data = 0x");
					if (tbval.lsb<=0xF) Serial.print("0");
					Serial.print(tbval.lsb,HEX);
					Serial.print(" 0x");
					if (tbval.msb<=0xF) Serial.print("0");
					Serial.println(tbval.msb,HEX);
					Vo=directf(tbval);
					Serial.print("Vout = ");
					Serial.print(Vo);
					Serial.println(" V");
					break;
				case 0x8C: //Iout
					tbval=CommandExec2(address, CMDCode);
					Serial.print("Raw Data = 0x");
					if (tbval.lsb<=0xF) Serial.print("0");
					Serial.print(tbval.lsb,HEX);
					Serial.print(" 0x");
					if (tbval.msb<=0xF) Serial.print("0");
					Serial.println(tbval.msb,HEX);
					Io=linear11(tbval);
					Serial.print("Iout = ");
					Serial.print(Io);
					Serial.println(" A");
					break;
				case 0x96: //Power
					tbval=CommandExec2(address, CMDCode);
					Serial.print("Raw Data = 0x");
					if (tbval.lsb<=0xF) Serial.print("0");
					Serial.print(tbval.lsb,HEX);
					Serial.print(" 0x");
					if (tbval.msb<=0xF) Serial.print("0");
					Serial.println(tbval.msb,HEX);
					P=linear11(tbval);
					Serial.print("Power = ");
					Serial.print(P);
					Serial.println(" W");
					break;
				case 0x9A:
					tbres=CommandExecN(address, CMDCode, DataBytes);
					Serial.print("Raw Data =");
					for (int i=0;i<DataBytes;i++){
						Serial.print(" 0x");
						if (tbres[i]<=0xF) Serial.print("0");
						Serial.print(tbres[i],HEX);
					}
					Serial.println();
					for (int i=0;i<DataBytes;i++)Serial.print((char)tbres[i]);
					Serial.println();
					break;
				default:
					tbres=CommandExecN(address, CMDCode, DataBytes);
					Serial.print("Raw Data =");
					for (int i=0;i<DataBytes;i++){
						Serial.print(" 0x");
						if (*(tbres+i)<=0xF) Serial.print("0");
						Serial.print(*(tbres+i),HEX);
					}
					Serial.println();
					break;
			}
		}
		Serial.println("OK");
	}
	
	
    
    // byte in2 = hex_char_to_int((char)Serial.read());
    // byte datatosend = in1<<4 | in2;
    // Serial.print("Sending 0x");
    // Serial.println(datatosend,HEX);
    // SendByteAndPrint(datatosend,16);
    delay(25);
  }
  delay(25);
}


//{ Function Definition
byte hex_char_to_int(char ch)
{
  if (ch >= '0' && ch<= '9')
  {
    return ch-'0';
  }
 
  if (ch >= 'A' && ch<='Z')
  {
    return ch-'A'+10;
  }
 
  if (ch >= 'a' && ch<='z')
  {
    return ch-'a'+10;
  }
}

double linear11(byte msb, byte lsb)
{
  // lsb msb = NNNNNYYY YYYYYYYY, X=Y>>N
  linear11_val_t t;
  twobytes tbval;
  tbval.msb=msb;
  tbval.lsb=lsb;
  t.tbdata=tbval;
  return t.linear.base * (double)pow(2, t.linear.mantissa);
}
double linear11(twobytes tbval)
{
  // lsb msb = NNNNNYYY YYYYYYYY, X=Y>>N
  linear11_val_t t;
  t.tbdata=tbval;
  return t.linear.base * (double)pow(2, t.linear.mantissa);
}
double linear16(byte msb, byte lsb)
{
  //
  return (double)msb*pow(2,-(lsb & 0x1F));
}
double linear16(twobytes tbval)
{
  //
  return (double)tbval.msb*pow(2,-(tbval.lsb & 0x1F));
}
double directf(byte msb, byte lsb)
{
  //
  linear11_val_t t;
  t.tbdata.msb=msb;
  t.tbdata.lsb=lsb;
  int16_t X=t.raw;
  return (m*X+b)*pow(10,R);
}
double directf(twobytes tbval)
{
  //
  linear11_val_t t;
  t.tbdata=tbval;
  int16_t X=t.raw;
  return (m*X+b)*pow(10,R);
}


bool CommandExec0(byte DeviceAddr, byte Command)
{
  int wen = 0;
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true);
  if (wen != 0) {
	Serial.print("endTransmission error ");
    Serial.println(wen);
  }
  /*Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true);*/
  return true;
}

byte CommandExec1(byte DeviceAddr, byte Command)
{
  int wen = 0;
  byte res;
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) false);
  if (wen != 0) {
	Serial.print("endTransmission error ");
    Serial.println(wen);
    res=0xFF;
  }
  Wire.requestFrom((uint8_t) DeviceAddr, (uint8_t) 1, (uint8_t) true);
  if (Wire.available()){
    res=Wire.read();
  } else {
  }
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true);
  return res;
}

twobytes CommandExec2(byte DeviceAddr, byte Command)
{
  int wen = 0;
  twobytes res;
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) false);
  if (wen != 0) {
	Serial.print("endTransmission error ");
    Serial.println(wen);
    res.lsb=0xFF;
    res.msb=0xFF;
  }
  Wire.requestFrom((uint8_t) DeviceAddr, (uint8_t) 2, (uint8_t) true);
  if (Wire.available()){
    res.lsb=Wire.read();
    res.msb=Wire.read();
  } else {
	  Serial.println("No data on bus\r\n");
  }
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true);
  return res;
}
byte* CommandExecN(byte DeviceAddr, byte Command, int count)
{
  int wen = 0;
  static byte res[255];
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) false);
  if (wen != 0) {
	Serial.print("endTransmission error ");
    Serial.println(wen);
  }
  Wire.requestFrom((uint8_t) DeviceAddr, (uint8_t) count, (uint8_t) true);
  for(int i=0;i<count;i++)
  {
      if (Wire.available()){
      res[i]=Wire.read();
	  //Serial.print(res[i],HEX);
	  //Serial.print(" ");
    } else {
	  Serial.print("No data on bus at #");
	  Serial.print(i);
	  Serial.println();
      break;
    }
  }
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true); 
  return res;
}
String CommandExecStr(byte DeviceAddr, byte Command, int count)
{
  int wen = 0;
  String res="";
  Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true);
  if (wen != 0) {
	Serial.print("endTransmission error ");
    Serial.println(wen);
  }
  Wire.requestFrom((uint8_t) DeviceAddr, (uint8_t) count, (uint8_t) true);  
  for(int i=0;i<count;i++)
  {
      if (Wire.available()){
      res+=Wire.read();
    } else {
      break;
    }
  }
  /*Wire.beginTransmission(DeviceAddr);
  Wire.write(Command);
  wen = Wire.endTransmission((uint8_t) true);*/
  return res;
}
//}
