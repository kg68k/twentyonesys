/*
 * $Project: TwentyOne for Human Ver 3 on X680x0 $
 * $Copyright: 1991,92,93 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/TwentyOne3/namecheck.h,v $
 * $Author: kawamoto $
 * $Revision: 1.9 $
 * $Date: 1994/12/13 07:10:36 $
 */

/* #define DEBUG */
#define PURGE_HEXNAMEBUF_IF_DUAL_LINK_BROKEN
#define ALLOW_CON_IN_COMMAND
#define LOG_OPTION_SUPPORT
#define WRITE_LOG_TO_CONSOLE
#undef WRITE_LOG_TO_FILE

register unsigned char *one_ptr asm("a5");
#define now_ptr one_ptr
#define OSservice_ptr ((OSservice *)one_ptr)
register unsigned char *two_ptr asm("a4");
#define setbuf_ptr two_ptr

#define compare(one, two, num, space, wildcard) \
	(one_ptr = (one), two_ptr = (two), _compare(num, space, wildcard))

__DOSCALL int const VERNUM(void);
__DOSCALL int GETENV(const char *name, const char *env, char *buffer);

__DOSCALL void PRINT(const char *string);
static __inline void dos_print(const char *string)
{
  PRINT(string);
}
__DOSCALL void PUTCHAR(short code);
static __inline void dos_putchar(int code)
{
  PUTCHAR(code);
}
static __inline void iocs_print(const char *string)
{
  register unsigned int reg_d0 asm ("d0");

  asm volatile ("movea.l %1,a1\n\t"
		"moveq.l #$21,%0\n\t"
		"trap #15"
		:"=d" (reg_d0)
		:"g" ((int)string)
		:"a1");
}
static __inline void iocs_putchar(int code)
{
  register unsigned int reg_d0 asm ("d0");

  asm volatile ("move.l %1,d1\n\t"
		"moveq.l #$20,%0\n\t"
		"trap #15"
		:"=d" (reg_d0)
		:"ri" (code)
		:"d1");
}

static __inline void *memcpy( void *dst, void *src, int size )
{
	unsigned char *d=(unsigned char *)dst;
	unsigned char *s=(unsigned char *)src;
	while ( (--size)>=0 ) *(d++) = *(s++);
	return (dst);
}

static __inline void *memset( void *dst, unsigned int chr, int size )
{
	unsigned char *d=(unsigned char *)dst;
	while ( (--size)>=0 ) *(d++) = chr;
	return (dst);
}

#if USE_DEBUG_DOS_PRINT
#define debug_print(x) dos_print(x)
#define debug_putchar(x) dos_putchar(x)
#else
#define debug_print(x) iocs_print(x)
#define debug_putchar(x) iocs_putchar(x)
#endif

__DOSCALL int OPEN(const char *name, short mode);
__DOSCALL int SEEK(short filedesc, long offset, short mode);
#define FILE_WRITING 1
__DOSCALL int FPUTC(short code, short filedesc);
__DOSCALL int CLOSE(short filedesc);

typedef struct {
  unsigned char path_delimitors[2];
  unsigned short error_only_head_code;
  unsigned short errors_num_minus_1;
  unsigned char error_codes[12];
} cmpdatbuf;

typedef struct {
  unsigned char chrdevck[6];
  cmpdatbuf *cmpdat;
  unsigned char extflag;
  unsigned char noflcd;
} OSservice;

typedef struct EX exnamebuf;
typedef struct FIL filbuf;

struct EX {
  unsigned char primary[8];
  unsigned char extendary[3];
  unsigned char secondary[10];
};

typedef struct {
  filbuf *pair;
  exnamebuf name;
} hexnamebuf;

typedef struct {
  hexnamebuf *pair;
  unsigned long Twen;
  unsigned short ty;
  unsigned char end_flag;
  unsigned char dummy[10];
} dummynamebuf;

typedef struct {
  unsigned char wild_card;
  unsigned char drive;
  unsigned char path[65];
  unsigned char primary[8];
  unsigned char extendary[3];
  unsigned char secondary[10];
} namebuf;

struct FIL {
  unsigned char attribute;
  unsigned char drive;
  unsigned long sector;
  unsigned short sector_length;
  unsigned short sector_position;
  dummynamebuf name;
};

typedef struct {
  unsigned short uni;
  unsigned short sjis;
} u2stable;

enum {
  COMPARE_EQUAL = 0,
  COMPARE_NOT_EQUAL = 1,
};

#define NULL 0
#define NOMORE -18
#define BADNAM -13
#define NAMELEN -13
#define EXTLEN -13
#define PATH_DELIM 9

#define Magic_Twen ('T'<<24|'w'<<16|'e'<<8|'n')
#define Magic_ty ('t'<<8|'y')

#define VFAT_NAME_LEN (13*0x40)
#define TWENTYONE_PRIMARY_MAX 8
#define TWENTYONE_SECONDARY_MAX 10
#define TWENTYONE_BASE_MAX (TWENTYONE_PRIMARY_MAX+TWENTYONE_SECONDARY_MAX)
#define TWENTYONE_EXT_MAX 3
#define TWENTYONE_NAME_MAX (TWENTYONE_BASE_MAX+TWENTYONE_EXT_MAX)

extern unsigned char sysroot[90];
extern char is_2ndbyte;
extern char warning;
extern int _compare(/* unsigned char *one_ptr, unsigned char *two_ptr, */
		       int num, int space, int wildcard);

extern struct FLAG {
  unsigned int verbose_option : 1,
	       case_option : 1,
	       special_option : 1,
	       many_periods_option : 1,
	       eleven_option : 1,
	       files_option : 1,
	       sysroot_option : 1,
	       casewarn_option : 1,
	       sysroot2_option : 1,
	       return_alias_vfat_option : 1,
	       dummy_option : 1,
#ifdef DEBUG
	       debug1_option : 1,
	       debug2_option : 1,
	       debug3_option : 1,
	       debug4_option : 1,
	       debug5_option : 1,
#else
	       debug_dummy_option : 5,
#endif
	       buffers_option : 16;
} flags, masks;

extern hexnamebuf hexnamebufs[];

extern int namecheck_init(/* OSservice *OSservice_ptr */);
extern int namecheck_close(/* OSservice *OSservice_ptr */);
extern int namecheck_chrdevck(namebuf *buf /* , OSservice *OSservice_ptr */);
extern int namecheck_dir_cmp(unsigned char *buf, unsigned char *disk);
extern int namecheck_namebf_cmp(namebuf *buf, namebuf *entry);
extern int namecheck_knj_case_cmp(/* unsigned char *one_ptr,
				     unsigned char *two_ptr, */ int num);
extern int namecheck_pathset(int length, unsigned char *top_ptr
			     /* , unsigned char *now_ptr, unsigned char *setbuf_ptr */);
extern int namecheck_pathok(unsigned char *filename, unsigned char *path_top,
			    namebuf *nameptr, unsigned char *path_bottom);
extern int namecheck_files_set(namebuf *buf, filbuf *entry);
extern exnamebuf *namecheck_files_get(filbuf *entry);
extern void namecheck_files_err(filbuf *entry);
extern int namecheck_files_ent(/* unsigned char *one_ptr, */
				  int flag, unsigned char *buf);
#ifdef	C_ERRCHR
extern int namecheck_errchr(int ch);
#endif
extern int namecheck_yenslh(/* unsigned char *now_ptr, unsigned char *setbuf_ptr */);
extern int start(unsigned char *args);
extern void disp_flags(void);

#ifdef DEBUG

void _DEBUGS(int level, const char *str);
void _DEBUGC(int level, int cod);
void _DEBUGN(int level, int cod);
void _DEBUGB(int level, int cod);
void _DEBUGW(int level, int cod);
void _DEBUGL(int level, int cod);

#define DEBUGS(level, str) _DEBUGS(DEBUG_BASE_LEVEL + level, str)
#define DEBUGC(level, cod) _DEBUGC(DEBUG_BASE_LEVEL + level, cod)
#define DEBUGN(level, cod) _DEBUGN(DEBUG_BASE_LEVEL + level, cod)
#define DEBUGB(level, cod) _DEBUGB(DEBUG_BASE_LEVEL + level, cod)
#define DEBUGW(level, cod) _DEBUGW(DEBUG_BASE_LEVEL + level, cod)
#define DEBUGL(level, cod) _DEBUGL(DEBUG_BASE_LEVEL + level, cod)

#else

#define DEBUGS(level, str)
#define DEBUGC(level, cod)
#define DEBUGN(level, cod)
#define DEBUGB(level, cod)
#define DEBUGW(level, cod)
#define DEBUGL(level, cod)

#endif


#define __CRLF__
#ifdef __CRLF__
  #define CRLF "\r\n"
#else
  #define CRLF "\n"
#endif

/* EOF */
