#if !defined( YM_IO_H_INCLUDED )
#define YM_IO_H_INCLUDED
#include	"arduino.h"
#include	<avr/pgmspace.h>

//#define	BORD_10
#define	BORD_11

class	IO{
	public:
		IO();
		void	Init();
		void	Assert(char *msg);
		void	PrintByte(char *msg,uint8_t data);
		void	PrintWord(char *msg,uint16_t data);
		void	PrintLong(char *msg,uint32_t data);
	private:
};
extern	IO	io;
#endif  //YM_IO_H_INCLUDED
