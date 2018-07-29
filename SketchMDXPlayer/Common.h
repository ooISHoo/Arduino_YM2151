#if !defined( YM_COMMON_H_INCLUDED )
#define YM_COMMON_H_INCLUDED
#include	"YM2151.h"
#include	"io.h"

#define		_DEBUG

#ifdef	_DEBUG
#define		ASSERT(msg)		io.Assert(msg)
#define		PRINTH(msg,data)	io.PrintWord(msg,data)
#else
#define		ASSERT(msg)
#define		PRINTH(msg,data)
#endif

#endif  //YM_COMMON_H_INCLUDED
