/*
	SketchMDXPlayer	v0.31
	author:ISH
*/
#include	"YM2151.h"
#include	"IO.h"
#include	"MMLParser.h"
#include	"MDXParser.h"

IO				io;
MDXParser		mdx;

int32_t		waittime=0;
uint32_t		proctime=0;

void setup()
{
Serial.begin(9600);
	io.Init();
	YM2151.begin();
	mdx.Setup(0x2800);
	mdx.Elapse(0);
}

void loop()
{
	waittime = mdx.ClockToMicroSec(1);
	waittime -= proctime;
	while(waittime > 0){
		if(waittime > 16383){
			delayMicroseconds(16383);
			waittime -= 16383;
		} else {
			delayMicroseconds(waittime);
			waittime = 0;
		}
	}
	proctime = micros();
	mdx.Elapse(1);
	proctime = micros() - proctime;
}
