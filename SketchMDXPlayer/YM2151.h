#if !defined( YM2151_H_INCLUDED )
#define YM2151_H_INCLUDED
#include	<avr/pgmspace.h>

class	YM2151_Class{
	public:
		YM2151_Class();
		void	begin();
		void	initLFO();
		uint8_t	read();
		void	write(uint8_t addr,uint8_t data);
		
		void	setTone(uint8_t ch,uint8_t keycode,int16_t kf);
		void	setVolume(uint8_t ch,uint8_t volume,uint16_t offset);
		void	noteOn(uint8_t ch);
		void	noteOff(uint8_t ch);
		void	loadTimbre(uint8_t ch,uint16_t prog_addr);
		void	loadSeparationTimbre(uint8_t ch,uint16_t prog_addr);
		void	dumpTimbre(uint16_t prog_addr);
		void	setPanpot(uint8_t ch,uint8_t pan);
	private:
		static	const	uint8_t		PIN_D0=2;
		static	const	uint8_t		PIN_D1=3;
		static	const	uint8_t		PIN_D2=4;
		static	const	uint8_t		PIN_D3=5;
		static	const	uint8_t		PIN_D4=6;
		static	const	uint8_t		PIN_D5=7;
		static	const	uint8_t		PIN_D6=8;
		static	const	uint8_t		PIN_D7=9;
		
		static	const	uint8_t		PIN_RD=10;
		static	const	uint8_t		PIN_WR=11;
		static	const	uint8_t		PIN_ADDR0=12;
		static	const	uint8_t		PIN_IC=13;
		
		uint8_t		RegFLCON[8];
		uint8_t		RegSLOTMASK[8];
		uint8_t		CarrierSlot[8];
		uint8_t		RegTL[8][4];

		void	wait(uint8_t loop);
};
extern YM2151_Class YM2151;
#endif  //YM2151H_INCLUDED
