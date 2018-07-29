#include <avr/io.h>
#include <avr/pgmspace.h>

#include  "Common.h"
#include  "IO.h"

//#define	DIRECT_IO


IO::IO()
{
}

void	IO::Init()
{
	Serial.begin(9600);
	return;
}


#ifdef	_DEBUG
void	IO::Assert(char *msg){
	Serial.println("**ASSERT**");
	Serial.println(msg);
	while(1){};
}
void	IO::PrintByte(char *msg,uint8_t data){
	Serial.print(msg);
	Serial.println(data,HEX);
}
void	IO::PrintWord(char *msg,uint16_t data){
	Serial.print(msg);
	Serial.println(data,HEX);
}
void	IO::PrintLong(char *msg,uint32_t data){
	Serial.print(msg);
	Serial.println(data,HEX);
}
#endif


