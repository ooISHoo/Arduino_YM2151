#include	"Common.h"
#include	"MMLParser.h"
#include	"MDXParser.h"

// ※クロックの１単位は全音符／１９２
typedef	void (MMLParser::*CommandFunc)();
static const CommandFunc CommandFuncTable[]={
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_xx_Unknown,
	&MMLParser::C_e7_Fadeout,
	&MMLParser::C_e8_PCM8Ext,
	&MMLParser::C_e9_LFODelay,
	&MMLParser::C_ea_LFOCtrl,
	&MMLParser::C_eb_LFOVolumeCtrl,
	&MMLParser::C_ec_LFOPitchCtrl,
	&MMLParser::C_ed_NoisePitch,
	&MMLParser::C_ee_SyncWait,
	&MMLParser::C_ef_SyncSend,
	&MMLParser::C_f0_KeyOnDelay,
	&MMLParser::C_f1_EndOfData,
	&MMLParser::C_f2_Portamento,
	&MMLParser::C_f3_Detune,
	&MMLParser::C_f4_ExitRepeat,
	&MMLParser::C_f5_BottomRepeat,
	&MMLParser::C_f6_StartRepeat,
	&MMLParser::C_f7_DisableKeyOn,
	&MMLParser::C_f8_KeyOnTime,
	&MMLParser::C_f9_VolumeUp,
	&MMLParser::C_fa_VolumeDown,
	&MMLParser::C_fb_Volume,
	&MMLParser::C_fc_Panpot,
	&MMLParser::C_fd_Timbre,
	&MMLParser::C_fe_Registar,
	&MMLParser::C_ff_Tempo,
};

void	MMLParser::Init(uint8_t ch,uint16_t base,uint16_t mmloffset){
	StatusF = 0;
	FunctionF = 0;
	CurrentAddr = base + mmloffset;
	Channel = ch;
	RegPAN = 0x3;
	KeyLength = 64;
	Volume = 0xf;
	VLFO.Offset = 0;
	PLFO.Offset = 0;
	Portamento = 0;
	for(int i=0;i<RepeatCnt;i++){
		RepeatList[i].Addr = 0;
	}
}
void	MMLParser::Elapse(){
	uint8_t		command = mdx.ReadData8(CurrentAddr++);
	if(command < 0x80){ // 休符
		// [$00 ～ $7F] 長さはデータ値+1クロック
		Clock = command+1;
		KeyOffClock = 0;
		YM2151.write(0x08,0x00 + Channel);
		return;
	}
	if(command < 0xe0){ // 音符
		Note = command - 0x80;
		uint8_t		keytime = mdx.ReadData8(CurrentAddr++);
		// [$80 ～ $DF] + [クロック - 1]  音程は$80がo0d+ $DFがo8d  PCMﾊﾟｰﾄではﾃﾞｰﾀ番号
		Clock = keytime + 1;
		KeyOffClock = (uint8_t)(KeyOnDelay + (((uint16_t)Clock * (uint16_t)KeyLength) >> 6));	// 
		KeyOff();	//
		if(StatusF & FLG_NEXTNKEYOFF){
			StatusF |= FLG_NKEYOFF;
			StatusF &= ~FLG_NEXTNKEYOFF;
		} else {
			StatusF &= ~FLG_NKEYOFF;
		}
		if(KeyOnDelay==0){
			KeyOn();
		} else {
			KeyOnDelayClock = KeyOnDelay;
		}
		return;
	}
	(this->*(CommandFuncTable[command-0xe0]))();
}

void	MMLParser::Calc(){
	if(FunctionF & FLG_FOUT){
	}
	if(FunctionF & FLG_VLFO){
		uint16_t	Delta;
		switch(VLFO.Type){
			case	0:	// non op
				break;
			case	1:	// L001120
				VLFO.Offset += VLFO.Delta;
				if(--VLFO.LengthCounter == 0){
					VLFO.LengthCounter = VLFO.Length;
					VLFO.Offset = VLFO.DeltaFixd;
				}
				break;
			case	2:	// L001138
				if(--VLFO.LengthCounter == 0){
					VLFO.LengthCounter = VLFO.Length;
					VLFO.Offset += VLFO.Delta;
					VLFO.Delta = -VLFO.Delta;
				}
				break;
			case	3:	// L00114e
				VLFO.Offset += VLFO.Delta;
				if(--VLFO.LengthCounter == 0){
					VLFO.LengthCounter = VLFO.Length;
					VLFO.Delta = -VLFO.Delta;
				}
				break;
			case	4:	// L001164
				if(--VLFO.LengthCounter == 0){
					VLFO.LengthCounter = VLFO.Length;
					VLFO.Offset = VLFO.Delta * random(255);
				}
				break;
			default:
				ASSERT("Unknown VLFO type!");
				break;
		}
		UpdateVolume();
	}
	if(FunctionF & FLG_PLFO){
		uint16_t	Delta;
		switch(PLFO.Type){
			case	0:	// non op
				break;
			case	1:	// L0010be
				PLFO.Offset += PLFO.Delta;
				if(--PLFO.LengthCounter == 0){
					PLFO.LengthCounter = PLFO.Length;
					PLFO.Offset = -PLFO.Offset;
				}
				break;
			case	2:	// L0010d4
				PLFO.Offset = PLFO.Delta;
				if(--PLFO.LengthCounter == 0){
					PLFO.LengthCounter = PLFO.Length;
					PLFO.Delta = -PLFO.Delta;
				}
				break;
			case	3:	// L0010ea
				PLFO.Offset += PLFO.Delta;
				if(--PLFO.LengthCounter == 0){
					PLFO.LengthCounter = PLFO.Length;
					PLFO.Delta = -PLFO.Delta;
				}
				break;
			case	4:	// L001100
				if(--PLFO.LengthCounter == 0){
					PLFO.LengthCounter = PLFO.Length;
					PLFO.Offset = PLFO.Delta * random(255);
				}
				break;
			default:
				ASSERT("Unknown PLFO type!");
				break;
			
		}
	}
	if((FunctionF & FLG_MPT) || (FunctionF & FLG_PLFO)){
		if(FunctionF & FLG_MPT){
			Portamento += PortamentoDelta;
		}
		SetTone();
	}

}

void	MMLParser::KeyOn(){
	Portamento = 0;
	SetTone();
	YM2151.noteOn(Channel);
	//YM2151.write(0x08,(RegSLOTMASK << 3)  + Channel);
	FunctionF &= ~(FLG_MPT | FLG_FOUT);
	if(FunctionF & FLG_NEXTMPT){
		FunctionF |= FLG_MPT;
	}
	if(FunctionF & FLG_NEXTFOUT){
		FunctionF |= FLG_FOUT;
	}
	FunctionF &= ~(FLG_NEXTMPT | FLG_NEXTFOUT);
}

void	MMLParser::KeyOff(){
	if((StatusF & FLG_NKEYOFF) == 0){
		YM2151.write(0x08,0x00 + Channel);
	} else {
		StatusF &= ~FLG_NKEYOFF;
	}
	KeyOffClock = 0;
}

void	MMLParser::SetTone(){
	int16_t	offset;
	offset = Detune;
	if(FunctionF & FLG_MPT){
		offset += Portamento>>16;	// 右シフトは問題なし
	}
	if(FunctionF & FLG_PLFO){
		offset += PLFO.Offset>>16;
	}
	YM2151.setTone(Channel,Note,offset);
	return;
}

void	MMLParser::UpdateVolume(){
	YM2151.setVolume(Channel,Volume,VLFO.Offset);
}

//・未定義コマンド
void	MMLParser::C_xx_Unknown(){
	ASSERT("Unknown MML Command.");
	StatusF |= FLG_HALT;
}
//・フェードアウト
//    [$E7] + [$01] + [SPEED]       $FOコマンド対応
void	MMLParser::C_e7_Fadeout(){
	mdx.ReadData8(CurrentAddr++);
	mdx.ReadData8(CurrentAddr++);
}
//・PCM8拡張モード移行
//    [$E8]                         Achの頭で有効
void	MMLParser::C_e8_PCM8Ext(){
}
//・LFOディレイ設定
//    [$E9] + [???]                 MDコマンド対応
void	MMLParser::C_e9_LFODelay(){
	mdx.ReadData8(CurrentAddr++);
}
//・OPMLFO制御
//    [$EA] + [$80]                 MHOF
//    [$EA] + [$81]                 MHON
//    [$EA] + [SYNC/WAVE] + [LFRQ] + [PMD] + [AMD] + [PMS/AMS]
void	MMLParser::C_ea_LFOCtrl(){
	uint8_t		lfocom = mdx.ReadData8(CurrentAddr++);
	if(lfocom & 0x80){
		if(lfocom & 0x01){		// ??
			YM2151.write(0x38+Channel,RegPMSAMS);
		} else {
			YM2151.write(0x38+Channel,0);
		}
		return;
	}
	YM2151.write(0x1b,lfocom);

	uint8_t		lfrq = mdx.ReadData8(CurrentAddr++);
	YM2151.write(0x18,lfrq);
	uint8_t		pmd = mdx.ReadData8(CurrentAddr++);
	YM2151.write(0x19,pmd);
	uint8_t		amd = mdx.ReadData8(CurrentAddr++);
	YM2151.write(0x19,amd);
	uint8_t		RegPMSAMS = mdx.ReadData8(CurrentAddr++);
	YM2151.write(0x38+Channel,RegPMSAMS);
}
//・音量LFO制御
//    [$EB] + [$80]                 MAOF
//    [$EB] + [$81]                 MAON
//    [$EB] + [WAVE※1] + [周期※2].w + [変移※4].w
void	MMLParser::C_eb_LFOVolumeCtrl(){
	uint8_t		lfocom = mdx.ReadData8(CurrentAddr++);
	if(lfocom & 0x80){
		if(lfocom & 0x01){
			FunctionF |= FLG_VLFO;
		} else {
			FunctionF &= ~FLG_VLFO;
			VLFO.Offset = 0;
		}
		return;
	}
	int16_t	DeltaFixd;
	VLFO.Type = lfocom+1;
	VLFO.Length = mdx.ReadData16(CurrentAddr);
	CurrentAddr+=2;
	DeltaFixd = VLFO.DeltaStart = mdx.ReadData16(CurrentAddr);
	CurrentAddr+=2;
	if((lfocom & 1)==0){
		DeltaFixd *= (int16_t)VLFO.Length;
	}
	DeltaFixd = -DeltaFixd;
	if(DeltaFixd < 0)DeltaFixd = 0;
	VLFO.DeltaFixd = DeltaFixd;
	
	VLFO.LengthCounter = VLFO.Length;
	VLFO.Delta = VLFO.DeltaStart;
	VLFO.Offset = VLFO.DeltaFixd;
	
	FunctionF |= FLG_VLFO;
}
//・音程LFO制御
//    [$EC] + [$80]                 MPOF
//    [$EC] + [$81]                 MPON
//    [$EC] + [WAVE※1] + [周期※2].w + [変移※3].w
void	MMLParser::C_ec_LFOPitchCtrl(){
	uint8_t		lfocom_f = mdx.ReadData8(CurrentAddr++);
	if(lfocom_f & 0x80){
		if(lfocom_f & 0x01){
			FunctionF |= FLG_PLFO;
		} else {
			FunctionF &= ~FLG_PLFO;
			PLFO.Offset = 0;
		}
		return;
	}
	uint8_t		lfocom = lfocom_f;
	lfocom &= 0x3;
	PLFO.Type = lfocom + 1;
	lfocom += lfocom;
	uint16_t	length = PLFO.Length = mdx.ReadData16(CurrentAddr);CurrentAddr+=2;
	if(lfocom != 2){
		length >>= 1;
		if(lfocom == 6){length = 1;}
	}
	PLFO.LengthFixd = length;
	int16_t	delta = mdx.ReadData16(CurrentAddr);CurrentAddr+=2;
	int16_t	delta_l = delta;
	if(lfocom_f >= 0x4){
		lfocom_f &= 0x3;
	} else {
		delta_l >>= 8;
	}

	PLFO.DeltaStart = delta_l;
	if(lfocom_f != 0x2) delta_l = 0;
	PLFO.OffsetStart = delta_l;

	PLFO.LengthCounter = PLFO.LengthFixd;
	PLFO.Delta = PLFO.DeltaStart;
	PLFO.Offset = PLFO.OffsetStart;

	FunctionF |= FLG_PLFO;
}



//・ADPCM/ノイズ周波数設定
//  チャンネルH
//    [$ED] + [???]                 ノイズ周波数設定。ビット7はノイズON/OFF
//  チャンネルP
//    [$ED] + [???]                 Fコマンド対応
void	MMLParser::C_ed_NoisePitch(){
	uint8_t		noise = mdx.ReadData8(CurrentAddr++);
	YM2151.write(0x0f,noise);
}
//・同期信号待機
//    [$EE]
void	MMLParser::C_ee_SyncWait(){
	StatusF |= FLG_SYNCWAIT;
}
//・同期信号送出
//    [$EF] + [チャネル番号(0～15)]
void	MMLParser::C_ef_SyncSend(){
	uint8_t		sendch = mdx.ReadData8(CurrentAddr++);
	mdx.SendSyncRelease(sendch);
}
//・キーオンディレイ
//    [$F0] + [???]                 kコマンド対応
void	MMLParser::C_f0_KeyOnDelay(){
	KeyOnDelay = mdx.ReadData8(CurrentAddr++);
}
//・データエンド
//    [$F1] + [$00]                 演奏終了
//    [$F1] + [ループポインタ].w    ポインタ位置から再演奏
void	MMLParser::C_f1_EndOfData(){
	uint8_t	off_u = (uint8_t)mdx.ReadData8(CurrentAddr++);
	if(off_u==0x0){
		StatusF |= FLG_HALT;
		KeyOff();
	} else {
		uint8_t	off_l = (uint8_t)mdx.ReadData8(CurrentAddr++);
		uint16_t	off=(uint16_t)off_u<< 8 | (uint16_t)off_l;
		off = (off ^ 0xffff)+1;
		CurrentAddr -= off;
	}
}
//・ポルタメント
//    [$F2] + [変移※3].w	          _コマンド対応   １単位は（半音／１６３８４）
// ※3  変移  1クロック毎の変化量。単位はデチューンの1/256
void	MMLParser::C_f2_Portamento(){
	int16_t	port = mdx.ReadData16(CurrentAddr);
	CurrentAddr+=2;
	PortamentoDelta = port;
	for(int i=0;i<8;i++){	// =<< 8 だけどArduinoの32bit演算libで挙動不審
		PortamentoDelta += PortamentoDelta;
	}
	Portamento = 0;
	FunctionF |= FLG_NEXTMPT;
}
//・デチューン
//    [$F3] + [???].w               Dコマンド対応   １単位は（半音／６４）
void	MMLParser::C_f3_Detune(){
	int16_t	dt = mdx.ReadData16(CurrentAddr);
	CurrentAddr+=2;
	Detune = dt;
}
//・リピート脱出
//    [$F4] + [終端コマンドへのオフセット+1].w
void	MMLParser::C_f4_ExitRepeat(){
	int16_t	off = (int16_t)mdx.ReadData16(CurrentAddr);
	CurrentAddr += 2;
	int16_t	btmaddr = CurrentAddr+off;
	int16_t	btmoff = (int16_t)mdx.ReadData16(btmaddr);
	btmaddr += 2;
	btmoff = (btmoff ^ 0xffff)+1;
	int16_t	tgtaddr = btmaddr-btmoff-1;
	uint8_t i;
	for(i=0;i<RepeatCnt;i++){
		if(RepeatList[i].Addr == tgtaddr)break;
	}
	if(i >= RepeatCnt) ASSERT("Address can not found.");
	if(RepeatList[i].Count==1){
		// break
		CurrentAddr = btmaddr;
		RepeatList[i].Addr = 0;
		RepeatList[i].Count = 0;
	}
}
//・リピート終端
//    [$F5] + [開始コマンドへのオフセット+2].w
void	MMLParser::C_f5_BottomRepeat(){
	int16_t	off = (int16_t)mdx.ReadData16(CurrentAddr);
	CurrentAddr += 2;
	off = (off ^ 0xffff)+1;
	int16_t	tgtaddr = CurrentAddr-off-1;
	uint8_t i;
	for(i=0;i<RepeatCnt;i++){
		if(RepeatList[i].Addr == tgtaddr)break;
	}
	if(i >= RepeatCnt) ASSERT("Address can not found.");
	
	RepeatList[i].Count--;
	if(RepeatList[i].Count>0){
		// repeat
		CurrentAddr -= off;
	} else {
		// exit repeat
		RepeatList[i].Addr = 0;
	}
}
//・リピート開始
//    [$F6] + [リピート回数] + [$00]
void	MMLParser::C_f6_StartRepeat(){
	uint8_t	count = mdx.ReadData8(CurrentAddr);
	CurrentAddr+=2;
	uint8_t i;
	for(i=0;i<RepeatCnt;i++){
		if(RepeatList[i].Addr == 0)break;
	}
	if(i >= RepeatCnt) ASSERT("Repeat list overflow.");
	RepeatList[i].Addr = CurrentAddr-1;
	RepeatList[i].Count = count;
}
//・キーオフ無効
//    [$F7]                         次のNOTE発音後キーオフしない
// A6->S0016 |= 0x04;
void	MMLParser::C_f7_DisableKeyOn(){
	StatusF |= FLG_NEXTNKEYOFF;
}
//・発音長指定
//    [$F8] + [$01～$08]            qコマンド対応
//    [$F8] + [$FF～$80]            @qコマンド対応（2の補数）
void	MMLParser::C_f8_KeyOnTime(){
	// 全音長の<num>÷8にあたる時間が経過した時点でキーオフします。
	uint8_t		length = mdx.ReadData8(CurrentAddr++);
	if(length <= 8){
		KeyLength = length * 8;
	} else {
		KeyLength = length - 0x80;	// ??
	}
}
//・音量増大
//    [$F9]                         ※vコマンド後では、 v0  →   v15 へと変化
//                                  ※@vコマンド後では、@v0 → @v127 へと変化
void	MMLParser::C_f9_VolumeUp(){
	if(Volume & 0x80){
		if(Volume > 0x80)Volume--;
	} else {
		if(Volume < 15)Volume++;
	}
	UpdateVolume();
}
//・音量減小
//    [$FA]                         ※vコマンド後では、 v15   →  v0 へと変化
//                                  ※@vコマンド後では、@v127 → @v0 へと変化
void	MMLParser::C_fa_VolumeDown(){
	if(Volume & 0x80){
		if(Volume < 0xff)Volume++;
	} else {
		if(Volume > 0)Volume--;
	}
	UpdateVolume();
}
//・音量設定
//    [$FB] + [$00～$15]            vコマンド対応
//    [$FB] + [$80～$FF]            @vコマンド対応（ビット7無効）
void	MMLParser::C_fb_Volume(){
	Volume = mdx.ReadData8(CurrentAddr++);
	UpdateVolume();
}
//・出力位相設定
//    [$FC] + [???]                 pコマンド対応
void	MMLParser::C_fc_Panpot(){
	uint8_t		pan = mdx.ReadData8(CurrentAddr++);
	RegPAN = pan;
	YM2151.setPanpot(Channel,RegPAN);
}
//・音色設定
//    [$FD] + [???]                 @コマンド対応
#define	DUMP_TIMBRE
void	MMLParser::C_fd_Timbre(){
	uint8_t		tno = mdx.ReadData8(CurrentAddr++);
	uint16_t	taddr = mdx.GetTimbreAddr(tno);
	uint8_t	no = mdx.ReadData8(taddr++);
	if(no!=tno)ASSERT("TimbreNo Unmuch");
	YM2151.loadTimbre(Channel,mdx.GetTimbreAddr(tno) + mdx.DataBP);
	YM2151.setPanpot(Channel,RegPAN);
	UpdateVolume();
}

//・OPMレジスタ設定
//    [$FE] + [レジスタ番号] + [出力データ]
void	MMLParser::C_fe_Registar(){
	uint8_t		reg = mdx.ReadData8(CurrentAddr++);
	uint8_t		data = mdx.ReadData8(CurrentAddr++);
	YM2151.write(reg,data);
}
//・テンポ設定
//    [$FF] + [???]                 @tコマンド対応
void	MMLParser::C_ff_Tempo(){
	uint8_t		tempo = mdx.ReadData8(CurrentAddr++);
	mdx.SetTempo(tempo);
}


