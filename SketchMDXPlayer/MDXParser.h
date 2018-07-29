#if !defined( YM_MDXP_H_INCLUDED )
#define YM_MDXP_H_INCLUDED
#include	"arduino.h"
#include	"MMLParser.h"
class	MDXParser{
	public:
		static	const	uint8_t		ChNum=8;

		uint16_t	DataBP;
		uint16_t	BaseOffset;
		uint16_t	TimbreOffset;
		uint8_t		Tempo;
		uint32_t	ClockMicro;
		MMLParser	OPMChannel[ChNum];
		void		Setup(uint16_t);
		uint16_t	Elapse(uint16_t);
		uint16_t	Forward(uint16_t);
		void		SetTempo(uint8_t);
		uint16_t	GetTimbreAddr(uint8_t);
		void		SendSyncRelease(uint8_t);
		uint32_t	ClockToMilliSec(uint8_t clock);
		uint32_t	ClockToMicroSec(uint8_t clock);

		inline	uint8_t	ReadData8(uint16_t addr){
			return (uint8_t)(pgm_read_byte_near(DataBP + (addr)));
		}
		
		inline	uint16_t	ReadData16(uint16_t addr){
			uint16_t rdata = pgm_read_word_near(DataBP + (addr));
			return	 (rdata << 8) | (rdata >> 8);
		}


};
extern	MDXParser	mdx;
#endif  //YM_MDXP_H_INCLUDED
