// twoncall.h 1.1.0
//   (V)TwentyOne.sys function call header by TcbnErik

// Copying and distribution of this file, with or without modification,
// are permitted in any medium without royalty provided the copyright
// notice and this notice are preserved.  This file is offered as-is,
// without any warranty.

#ifndef __twoncall_h__
#define __twoncall_h__

#ifndef __GNUC__
#error This file can be compiled only by GNU-C compiler.
#endif
#ifndef __MARIKO_CC__
#error This file can be compiled only by GCC Mariko version.
#endif

typedef struct {
  unsigned short                //
      verbose_option : 1,       //
      case_option : 1,          //
      special_option : 1,       //
      multi_period_option : 1,  //
      twentyone_option : 1,     //
      files_option : 1,         //
      sysroot_option : 1,       //
      casewarn_option : 1,      //
      sysroot2_option : 1,      //
      alias_option : 1,         //
      dummy_option : 1,         //
      debug_dummy_option : 5;   //
  unsigned short buffers_option;
} TwonFlags;

#define TWON_VERBOSE_BIT 31
#define TWON_CASE_BIT 30
#define TWON_SPECIAL_BIT 29
#define TWON_PERIOD_BIT 28
#define TWON_TWENTYONE_BIT 27
#define TWON_FILES_BIT 26
#define TWON_SYSROOT_BIT 25
#define TWON_CASEWARN_BIT 24
#define TWON_SYSROOT2_BIT 23
#define TWON_ALIAS_BIT 22

#define TWON_ID (('T' << 24) | ('w' << 16) | ('O' << 8) | 'n')
#define SYSROOT_MAX 90

#define TWON_GETID 0
#define TWON_GETVER 1
#define TWON_GETADR 2
#define TWON_GETOPT 3
#define TWON_SETOPT 4
#define TWON_GETSYSR 5
#define TWON_SETSYSR 6
#define TWON_U2S 7

__asm(
    ".ifndef _di_twon\n"
    "_di_twon:  .equ $ffb0\n"
    "_di_twonw: .equ $ffb0\n"
    "_di_twonl: .equ $ffb0\n"
    ".endif");

#ifdef __cplusplus
extern "C" {
#endif
__DOSCALL int _di_twon(short);
__DOSCALL int _di_twonw(short, short);
__DOSCALL int _di_twonl(short, int);
#ifdef __cplusplus
}
#endif

static __inline int _dos_twon(int mode) { return _di_twon((short)mode); }

static __inline int _dos_twonw(int mode, int code) {
  return _di_twonw((short)mode, (short)code);
}

static __inline int _dos_twonl(int mode, int flag) {
  return _di_twonl((short)mode, flag);
}

#define GetTwentyOneID() _dos_twon(TWON_GETID)
#define GetTwentyOneVersion() _dos_twon(TWON_GETVER)
#define GetTwentyOneAddress() _dos_twon(TWON_GETADR)
#define GetTwentyOneOptions() _dos_twon(TWON_GETOPT)
#define SetTwentyOneOptions(x) _dos_twonl(TWON_SETOPT, (x))
#define GetTwentyOneSysroot(p) _dos_twonl(TWON_GETSYSR, (int)(p))
#define SetTwentyOneSysroot(p) _dos_twonl(TWON_SETSYSR, (int)(p))
#define Unicode2Sjis(x) _dos_twonw(TWON_U2S, (x))

#endif
