#if !defined( YM_MMLP_H_INCLUDED )
#define YM_MMLP_H_INCLUDED
#include	"arduino.h"

#define		FLG_HALT		(1<<0)
#define		FLG_SYNCWAIT	(1<<1)
#define		FLG_SYNCWAITRUN	(1<<2)
#define		FLG_NKEYOFF		(1<<3)
#define		FLG_NEXTNKEYOFF	(1<<4)

#define		FLG_NEXTMPT			(1<<0)		// ポルタメント実行中
#define		FLG_NEXTFOUT		(1<<1)		// ボリュームフェード実行中
#define		FLG_MPT				(1<<4)		// ポルタメント実行中
#define		FLG_FOUT			(1<<5)		// ボリュームフェード実行中
#define		FLG_VLFO			(1<<6)		// ボリュームLFO実行中
#define		FLG_PLFO			(1<<7)		// ピッチLFO実行中

class	MMLParser{
	static	const	uint8_t	RepeatCnt=4;
	public:
		uint8_t		StatusF;
		uint8_t		FunctionF;
		uint8_t		Channel;
		uint16_t	MMLOffset;
		uint16_t	CurrentAddr;
		uint8_t		Clock;
		uint8_t		KeyOffClock;
		
		typedef		struct	{
			uint16_t	Addr;
			uint8_t		Count;
		}RepeatFrame;
		RepeatFrame	RepeatList[4];
		int8_t		KeyLength;
		
		uint32_t	Portamento;
		uint32_t	PortamentoDelta;
		uint16_t	Detune;
		uint8_t		Note;
		uint8_t		Volume;
		uint8_t		KeyOnDelay;
		uint8_t		KeyOnDelayClock;
		
		uint8_t		RegPAN;
		uint8_t		RegPMSAMS;

		typedef		struct	{
			uint32_t	OffsetStart;		//32->16
			uint32_t	DeltaStart;			//32->16
			uint32_t	Offset;				//32->16
			uint32_t	Delta;				//32->16
			uint16_t	LengthFixd;
			uint16_t	Length;
			uint16_t	LengthCounter;
			int8_t		Type;
		}PLFOFrame;
		PLFOFrame	PLFO;

		typedef		struct	{
			uint16_t	DeltaStart;
			uint16_t	DeltaFixd;
			uint16_t	Delta;
			uint16_t	Offset;
			uint16_t	Length;
			uint16_t	LengthCounter;
			int8_t		Type;
		}VLFOFrame;
		VLFOFrame	VLFO;
		
		void		Init(uint8_t,uint16_t,uint16_t);
		void		Elapse();
		void		Calc();
		void		SetTone();
		void		KeyOn();
		void		KeyOff();
		void		UpdateVolume();
		
		void		C_xx_Unknown();
		void		C_e7_Fadeout();
		void		C_e8_PCM8Ext();
		void		C_e9_LFODelay();
		void		C_ea_LFOCtrl();
		void		C_eb_LFOVolumeCtrl();
		void		C_ec_LFOPitchCtrl();
		void		C_ed_NoisePitch();
		void		C_ee_SyncWait();
		void		C_ef_SyncSend();
		void		C_f0_KeyOnDelay();
		void		C_f1_EndOfData();
		void		C_f2_Portamento();
		void		C_f3_Detune();
		void		C_f4_ExitRepeat();
		void		C_f5_BottomRepeat();
		void		C_f6_StartRepeat();
		void		C_f7_DisableKeyOn();
		void		C_f8_KeyOnTime();
		void		C_f9_VolumeUp();
		void		C_fa_VolumeDown();
		void		C_fb_Volume();
		void		C_fc_Panpot();
		void		C_fd_Timbre();
		void		C_fe_Registar();
		void		C_ff_Tempo();

};
#endif  //YM_MMLP_H_INCLUDED
