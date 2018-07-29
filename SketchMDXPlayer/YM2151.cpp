/*
	YM2151 library	v0.13
	author:ISH
*/
#include <arduino.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include  "Common.h"
#include  "YM2151.h"

#define	DIRECT_IO

YM2151_Class	YM2151;

YM2151_Class::YM2151_Class()
{
}

/*! IOの初期設定を行いYM2151/YM3012をハードリセットする、必ず呼ぶ必要あり
 */
void	YM2151_Class::begin()
{
	digitalWrite(PIN_IC, HIGH);
	digitalWrite(PIN_ADDR0, HIGH);
	digitalWrite(PIN_WR, HIGH);
	digitalWrite(PIN_RD, HIGH);

	pinMode(PIN_ADDR0, OUTPUT); 
	pinMode(PIN_WR, OUTPUT); 
	pinMode(PIN_RD, OUTPUT); 
	pinMode(PIN_IC, OUTPUT); 
	digitalWrite(PIN_IC, LOW);
	delay(100);
	digitalWrite(PIN_IC, HIGH);
	delay(100);
	return;
}

//
#define		RD_HIGH		(PORTB = PORTB | 0x4)
#define		RD_LOW		(PORTB = PORTB & ~0x4)
#define		WR_HIGH		(PORTB = PORTB | 0x8)
#define		WR_LOW		(PORTB = PORTB & ~0x8)
#define		A0_HIGH		(PORTB = PORTB | 0x10)
#define		A0_LOW		(PORTB = PORTB & ~0x10)

#define		BUS_READ	DDRD=0x02;DDRB=0x3c;
#define		BUS_WRITE	DDRD=0xfe;DDRB=0x3f;

#ifdef DIRECT_IO

static	uint8_t last_write_addr=0x00;

/*! 指定アドレスのレジスタに書き込みを行う
	\param addr		アドレス
	\param data		データ
 */
void	YM2151_Class::write(uint8_t addr,uint8_t data)
{
	uint8_t i,wi;
	volatile	uint8_t	*ddrD=&DDRD;
	volatile	uint8_t	*ddrB=&DDRB;
	volatile	uint8_t	*portD=&PORTD;
	volatile	uint8_t	*portB=&PORTB;
	
	if(last_write_addr != 0x20){
		// addr 0x20へのアクセスの後busyフラグが落ちなくなる病 '86の石だと発生
		// 他のレジスタに書くまで落ちないので、強引だが0x20アクセス後ならチェックしない
		*ddrD &= ~(_BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7));
		*ddrB &= ~(_BV(0) | _BV(1));
		wait(8);
		A0_LOW;
		wait(4);
		for(i=0;i<32;i++){
			RD_LOW;
			wait(4);
			if((*portB & _BV(1))==0){	// Read Status
				RD_HIGH;
				wait(4);
				break;
			}
			RD_HIGH;
			wait(8);
			if(i>16){
				delayMicroseconds(1);
			}
		}
	}
	wait(4);
	
	*ddrD |= (_BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7));
	*ddrB |= (_BV(0) | _BV(1));
	wait(8);
	A0_LOW;
	*portD = (addr << 2) | (*portD & 0x03);
	*portB = (addr >> 6) | (*portB & 0xfc);
	wait(4);
	WR_LOW;		// Write Address
	wait(4);
	WR_HIGH;
	wait(2);
	A0_HIGH;
	wait(2);
	*portD = (data << 2) | (*portD & 0x03);
	*portB = (data >> 6) | (*portB & 0xfc);
	
	wait(4);
	WR_LOW;		// Write Data
	wait(4);
	WR_HIGH;
	wait(2);
	last_write_addr = addr;
}

/*! ステータスを読み込む、bit0のみ有効
 */
uint8_t	YM2151_Class::read()
{
	uint8_t i,wi,data;
	volatile	uint8_t	*ddrD=&DDRD;
	volatile	uint8_t	*ddrB=&DDRB;
	volatile	uint8_t	*portD=&PORTD;
	volatile	uint8_t	*portB=&PORTB;
	*ddrD &= ~(_BV(2) | _BV(3) | _BV(4) | _BV(5) | _BV(6) | _BV(7));
	*ddrB &= ~(_BV(0) | _BV(1));
	A0_HIGH;
	wait(4);
	RD_LOW;		// Read Data
	wait(4);
	data = 0;
	data |= (*portD)>>2;
	data |= (*portB)<<6;
	RD_HIGH;
	wait(4);
}

/*! 約300nSec x loop分だけ待つ、あまり正確でない。
	\param loop		ループ数
 */
void	YM2151_Class::wait(uint8_t loop)
{
	uint8_t wi;
	for(wi=0;wi<loop;wi++){
		// 16MHz  nop = @60nSec
		asm volatile("nop\n\t nop\n\t nop\n\t nop\n\t");
	}
}

/*! LFOを初期化する
 */
void	YM2151_Class::initLFO()
{
	write(0x1,0x1);
}

#endif

/*! MDX形式の音色データを読み込みレジスタにセットする、音色データセット後は
	ボリューム、パンポットの設定を必ず行い関連レジスタの整合性を取る。
	\param ch				設定するチャンネル
	\param prog_addr		プログラムアドレス
 */
void	YM2151_Class::loadTimbre(uint8_t ch,uint16_t prog_addr)
{
	static uint8_t carrier_slot_tbl[] = {
		0x08,0x08,0x08,0x08,0x0c,0x0e,0x0e,0x0f,
	};
	uint16_t taddr = prog_addr;
	uint8_t	no = pgm_read_byte_near(taddr++);
	RegFLCON[ch] = pgm_read_byte_near(taddr++);
	CarrierSlot[ch] = carrier_slot_tbl[RegFLCON[ch] & 0x7];
	RegSLOTMASK[ch] = pgm_read_byte_near(taddr++);

	for(int i=0;i<32;i+=8){
		uint8_t	dt1_mul =pgm_read_byte_near(taddr++);
		write(0x40+ch+i,dt1_mul);
	}
	for(int i=0;i<4;i++){
		RegTL[ch][i] =pgm_read_byte_near(taddr++);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	ks_ar =pgm_read_byte_near(taddr++);
		write(0x80+ch+i,ks_ar);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	ame_d1r =pgm_read_byte_near(taddr++);
		write(0xa0+ch+i,ame_d1r);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	dt2_d2r =pgm_read_byte_near(taddr++);
		write(0xc0+ch+i,dt2_d2r);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	d1l_rr =pgm_read_byte_near(taddr++);
		write(0xe0+ch+i,d1l_rr);
	}
}

/*! 一般的なFM音色配列を読み込みレジスタにセットする、音色データセット後は
	ボリューム、パンポットの設定を必ず行い関連レジスタの整合性を取る。
	\param ch				設定するチャンネル
	\param prog_addr		プログラムアドレス
 */
void	YM2151_Class::loadSeparationTimbre(uint8_t ch,uint16_t prog_addr)
{
	// D2R(SR)が2151は5bit、DT2(追加デチューン)が無視されているので微妙に使いきれない
	// D2Rは<<1するとどうも変なのでそのまま
	static uint8_t carrier_slot_tbl[] = {
		0x08,0x08,0x08,0x08,0x0c,0x0e,0x0e,0x0f,
	};
	uint16_t taddr = prog_addr;
	uint8_t	con = pgm_read_byte_near(taddr++);
	uint8_t	fl = pgm_read_byte_near(taddr++);
	RegFLCON[ch] = (fl << 3) | con;
	CarrierSlot[ch] = carrier_slot_tbl[con];
	RegSLOTMASK[ch] = 0xf;
	for(int i=0;i<4;i++){
		uint8_t	slotindex = i*8+ch;
		uint8_t	ar = pgm_read_byte_near(taddr++);
		uint8_t	dr = pgm_read_byte_near(taddr++);
		uint8_t	sr = pgm_read_byte_near(taddr++);
		uint8_t	rr = pgm_read_byte_near(taddr++);
		uint8_t	sl = pgm_read_byte_near(taddr++);
		uint8_t	ol = pgm_read_byte_near(taddr++);
		uint8_t	ks = pgm_read_byte_near(taddr++);
		uint8_t	ml = pgm_read_byte_near(taddr++);
		uint8_t	dt1 = pgm_read_byte_near(taddr++);
		uint8_t	ams = pgm_read_byte_near(taddr++);
		
		RegTL[ch][i] = ol;
		write(0x40+slotindex,(dt1 << 4) | ml);		// DT1 MUL
		write(0x80+slotindex,(ks << 6) | ar);		// KS AR
		write(0xa0+slotindex,(ams << 7) | dr);		// AMS D1R
		write(0xc0+slotindex, 0 | (sr));			// DT2 D2R
		write(0xe0+slotindex,(sl << 4)| rr);		// D1L RR
	}
}

/*! MDX形式の音色データを読み込みコンソールにダンプする
	\param prog_addr		プログラムアドレス
 */
void	YM2151_Class::dumpTimbre(uint16_t prog_addr){
	uint16_t taddr = prog_addr;
	uint8_t	no = pgm_read_byte_near(taddr++);
	uint8_t	flcon = pgm_read_byte_near(taddr++);
	uint8_t	slotmask = pgm_read_byte_near(taddr++);

	PRINTH("No:",no);
	PRINTH("RegFLCON:",flcon);
	PRINTH("RegSLOTMASK:",slotmask);

	for(int i=0;i<32;i+=8){
		uint8_t	dt1_mul =pgm_read_byte_near(taddr++);
		PRINTH("DT1_MUL:",dt1_mul);
	}
	for(int i=0;i<4;i++){
		uint8_t	tl =pgm_read_byte_near(taddr++);
		PRINTH("TL:",tl);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	ks_ar =pgm_read_byte_near(taddr++);
		PRINTH("KS_AR:",ks_ar);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	ame_d1r =pgm_read_byte_near(taddr++);
		PRINTH("AME_D1R:",ame_d1r);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	dt2_d2r =pgm_read_byte_near(taddr++);
		PRINTH("DT2_D2R:",dt2_d2r);
	}
	for(int i=0;i<32;i+=8){
		uint8_t	d1l_rr =pgm_read_byte_near(taddr++);
		PRINTH("D1L_RR:",d1l_rr);
	}
}

/*! 音量を設定する、あくまでオペレーターのレベルを操作するだけなので
	音色と音量の組み合わせによっては破綻することがあります。
	\param ch				設定するチャンネル
	\param volume			ボリューム、0(最小)～15(最大) 最上位ビット(0x80)Onの場合は0x80をマスク後TLの値にそのまま加算
	\param offset			上位8bitをTLの値に加算、MDX再生用
 */
void	YM2151_Class::setVolume(uint8_t ch,uint8_t volume,uint16_t offset)
{
	static  uint8_t	volume_tbl[] = {
		0x2a,0x28,0x25,0x22,0x20,0x1d,0x1a,0x18,
		0x15,0x12,0x10,0x0d,0x0a,0x08,0x05,0x02,
	};
	int16_t	tl,att;
	if(volume & (0x80)){
		tl = volume & 0x7f;
	} else {
		if(volume>15)ASSERT("Illegal volume.");
		tl = volume_tbl[volume];
	}
	tl += offset>>8;
	for(int i=0;i<4;i++){
		if(CarrierSlot[ch] & (1<<i)){
			att = RegTL[ch][i]+tl;
		} else {
			att = RegTL[ch][i];
		}
		if(att > 0x7f || att < 0) att=0x7f;
		write(0x60 + i*8 + ch,att);
	}
}

/*! ノートオンする
	\param ch				オンするチャンネル
 */
void	YM2151_Class::noteOn(uint8_t ch)
{
	write(0x08,(RegSLOTMASK[ch] << 3)  + ch);
}

/*! ノートオフする
	\param ch				オフするチャンネル
 */
void	YM2151_Class::noteOff(uint8_t ch)
{
		write(0x08,0x00 + ch);
}

const char KeyCodeTable[] PROGMEM= {
	0x00,0x01,0x02,0x04,0x05,0x06,0x08,0x09,
	0x0a,0x0c,0x0d,0x0e,0x10,0x11,0x12,0x14,
	0x15,0x16,0x18,0x19,0x1a,0x1c,0x1d,0x1e,
	0x20,0x21,0x22,0x24,0x25,0x26,0x28,0x29,
	0x2a,0x2c,0x2d,0x2e,0x30,0x31,0x32,0x34,
	0x35,0x36,0x38,0x39,0x3a,0x3c,0x3d,0x3e,
	0x40,0x41,0x42,0x44,0x45,0x46,0x48,0x49,
	0x4a,0x4c,0x4d,0x4e,0x50,0x51,0x52,0x54,
	0x55,0x56,0x58,0x59,0x5a,0x5c,0x5d,0x5e,
	0x60,0x61,0x62,0x64,0x65,0x66,0x68,0x69,
	0x6a,0x6c,0x6d,0x6e,0x70,0x71,0x72,0x74,
	0x75,0x76,0x78,0x79,0x7a,0x7c,0x7d,0x7e,
};

/*! 音程を設定する
	\param ch				設定するチャンネル
	\param keycode			オクターブ0のD#を0とした音階、D# E F F# G G# A A# B (オクターブ1) C C# D....と並ぶ
	\param kf				音階微調整、64で1音分上がる。
 */
void	YM2151_Class::setTone(uint8_t ch,uint8_t keycode,int16_t kf){
	int16_t	offset_kf = (kf & 0x3f);
	int16_t	offset_note = keycode + (kf >> 6);
	if(offset_note < 0)offset_note=0;
	if(offset_note > 0xbf)offset_note=0xbf;
	
	write(0x30 + ch, offset_kf<<2);
	write(0x28 + ch, pgm_read_byte_near(KeyCodeTable + offset_note));
}
/*! パンポットを設定する
	\param ch				設定するチャンネル
	\param pan				パン設定、0:出力なし 1:左 2:右 3:両出力
 */
void	YM2151_Class::setPanpot(uint8_t ch,uint8_t pan){
	write(0x20+ch,
		(pan<<6) | (RegFLCON[ch] ));

}
