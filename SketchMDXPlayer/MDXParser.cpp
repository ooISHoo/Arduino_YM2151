#include	"Common.h"
#include	"MDXParser.h"

void		MDXParser::Setup(uint16_t bp)
{
	uint16_t	tableaddr=0;
	DataBP = bp;
	SetTempo(100);
	while(ReadData8(tableaddr++)!=0);
	BaseOffset = tableaddr;
	TimbreOffset = ReadData16(tableaddr);
	tableaddr+=2;
	for(int i=0;i<ChNum;i++){
		OPMChannel[i].Init(i,BaseOffset,ReadData16(tableaddr));
		tableaddr+=2;
	}
	YM2151.write(0x0f,0);


}
uint16_t	MDXParser::Elapse(uint16_t c){
	uint8_t		minclock=0xff;
	uint8_t		cmask=0xff;
	
	for(int i=0;i<ChNum;i++){
		if(((1<<i) & cmask)==0)continue;
		if(OPMChannel[i].StatusF & (FLG_HALT | FLG_SYNCWAIT | FLG_SYNCWAITRUN))continue;
		if(OPMChannel[i].Clock < c)ASSERT("Illegal clock");
		OPMChannel[i].Clock -= c;
		if(OPMChannel[i].KeyOnDelayClock > 0){
			OPMChannel[i].KeyOnDelayClock -= c;
			if(OPMChannel[i].KeyOnDelayClock==0){
				OPMChannel[i].KeyOn();
			}
		}
		OPMChannel[i].Calc();
		if(OPMChannel[i].KeyOffClock > 0){
			OPMChannel[i].KeyOffClock -= c;
			if(OPMChannel[i].KeyOffClock==0){
				OPMChannel[i].KeyOff();
			}
		}
		while(OPMChannel[i].Clock==0){
			OPMChannel[i].Elapse();
			if(OPMChannel[i].StatusF & (FLG_HALT | FLG_SYNCWAIT | FLG_SYNCWAITRUN))break;
		}
	}
	for(int i=0;i<ChNum;i++){
		if(((1<<i) & cmask)==0)continue;
		OPMChannel[i].StatusF &= ~FLG_SYNCWAITRUN;
		if(OPMChannel[i].Clock < minclock)minclock = OPMChannel[i].Clock;
		if(OPMChannel[i].KeyOffClock > 0 &&  OPMChannel[i].KeyOffClock < minclock)minclock = OPMChannel[i].KeyOffClock;
	}
	return minclock;
}
void		MDXParser::SetTempo(uint8_t tempo){
	Tempo = tempo;
	ClockMicro = (1024*(256-Tempo))/4;
}
void		MDXParser::SendSyncRelease(uint8_t ch){
	if(ch < ChNum){
		OPMChannel[ch].StatusF &= ~FLG_SYNCWAIT;
		while(OPMChannel[ch].Clock==0){
			OPMChannel[ch].Elapse();
		}
		OPMChannel[ch].StatusF |= FLG_SYNCWAITRUN;
	}
}
uint16_t	MDXParser::GetTimbreAddr(uint8_t timbleno){

	for(int i=0;i<256;i++){
		uint16_t	addr =  BaseOffset + TimbreOffset + 27 * (uint16_t)i;
		uint8_t		tno = ReadData8(addr);
		if(tno == timbleno) return addr;
	}
	ASSERT("can not find timbre");
	return 0;
}
uint32_t	MDXParser::ClockToMilliSec(uint8_t clock){
	return ((uint32_t)clock * ClockMicro)/(uint32_t)1000;
}
uint32_t	MDXParser::ClockToMicroSec(uint8_t clock){
	return ((uint32_t)clock * ClockMicro);
}

