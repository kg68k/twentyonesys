/*
 * $Project: TwentyOne for Human Ver 3 on X680x0 $
 * $Copyright: 1991,92,93 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/TwentyOne3/namecheck.c,v $
 * $Author: kawamoto $
 * $Revision: 1.23 $
 * $Date: 1994/12/13 07:31:19 $
 */


#include "namecheck.h"

#ifdef	USE_VFAT
#include "vfat.h"
#endif	/* USE_VFAT */


/****************************************************************/
/*								*/
/*			nameckeck ドライバ本体			*/
/*								*/
/****************************************************************/


int namecheck_init(/* OSservice *OSservice_ptr */)
{
#define DEBUG_BASE_LEVEL 3
  static cmpdatbuf cmpdat_yenslash = {
    {0, 0}, 0, 1 - 1, {' '}
  };

  DEBUGS(0, "namecheck_init" CRLF);
#ifdef	USE_VFAT
  vfat_init();
#endif	/* USE_VFAT */
  OSservice_ptr->cmpdat = &cmpdat_yenslash;
  return 0;
#undef DEBUG_BASE_LEVEL
}

int namecheck_close(/* OSservice *OSservice_ptr */)
{
#define DEBUG_BASE_LEVEL 3
  DEBUGS(0, "namecheck_close end" CRLF);
  return 0;
#undef DEBUG_BASE_LEVEL
}

static inline int chrdevck(unsigned char *name /* , OSservice *OSservice_ptr */)
{
#define DEBUG_BASE_LEVEL 3
  register int retcode asm("d0");

  asm volatile("move.l %1,a0\n\t"
	       "jsr (%2)\n\t" :
	       "=d" (retcode) :
	       "a" (name), "a" (OSservice_ptr) :
	       "a0");
  return retcode;
#undef DEBUG_BASE_LEVEL
}

int namecheck_chrdevck(namebuf *buf /* , OSservice *OSservice_ptr */)
{
#define DEBUG_BASE_LEVEL 1
  int retcode;

  DEBUGS(0, "namecheck_chrdevck");
  DEBUGS(1, " (");
  DEBUGL(1, (int)buf->primary);
  DEBUGS(1, ") [");
  DEBUGS(1, (char *)buf->primary);
  DEBUGS(1, "]");

  if (buf->secondary[0] != 0 && buf->secondary[0] != ' ')
    {
      DEBUGS(0, CRLF);
      return -1;
    }
  if (buf->extendary[0] != ' ')
    {
      DEBUGS(0, CRLF);
      return -1;
    }
#ifdef	ALLOW_CON_IN_COMMAND
  {
    int i;
    unsigned char primary[9];

    for (i = 0; i < 8; ++i)
      {
	primary[i] = buf->primary[i];
	if (primary[i] == '.')
	  {
	    if (i < 7 && buf->primary[i + 1] != ' ')
	      {
		DEBUGS(1, "con.x.y");
		DEBUGS(0, CRLF);
		return -1;
	      }
	    break;
	  }
      }
    for (; i < 8; ++i)
      primary[i] = ' ';
    primary[8] = 0;
    retcode = chrdevck(primary /* , OSservice_ptr */);
  }
#else
  retcode = chrdevck(buf->primary /* , OSservice_ptr */);
#endif	/* ALLOW_CON_IN_COMMAND */
  if (retcode != -1)
    {
      DEBUGS(1, " retcode ");
      DEBUGL(1, retcode);
    }
  DEBUGS(0, CRLF);
  return retcode;
#undef DEBUG_BASE_LEVEL
}

char is_2ndbyte;
char warning;

int _compare(/* unsigned char *one_ptr, unsigned char *two_ptr, */
		       int num, int space, int wildcard)
{
#define DEBUG_BASE_LEVEL 3
  unsigned char one, two;

  while (num--)
    {
      one = *one_ptr++;
      if (is_2ndbyte)
	{
	  if (one < 0x40 || one == 0x7f || 0xfc < one)
	    {
	      is_2ndbyte = 0;
	    }
	  else
	    {
	      if (one != *two_ptr++)
		return COMPARE_NOT_EQUAL;
	      is_2ndbyte = 0;
	      continue;
	    }
	}
      if (0x80 <= one && (one <= 0x9f || (0xe0 <= one && one <= 0xef)))
	{
	  if (one != *two_ptr++)
	    return COMPARE_NOT_EQUAL;
	  is_2ndbyte = 1;
	  continue;
	}
      if (one == space)
	one = ' ';
      if ((two = *two_ptr++) == space)
	two = ' ';
      if (one == wildcard)
	continue;
      if (one == 0 && two == 0)
	return COMPARE_EQUAL;
      if (one == 0 || two == 0)
	return COMPARE_NOT_EQUAL;
      if (!flags.case_option)
	{
	  if ('a' <= one && one <= 'z')
	    one += 'A' - 'a';
	  if ('a' <= two && two <= 'z')
	    two += 'A' - 'a';
	  if (one != two)
	    return COMPARE_NOT_EQUAL;
	}
      else if (flags.casewarn_option)
	{
	  unsigned char oneUpper, twoUpper;

	  oneUpper = one;
	  if ('a' <= oneUpper && oneUpper <= 'z')
	    oneUpper += 'A' - 'a';
	  twoUpper = two;
	  if ('a' <= twoUpper && twoUpper <= 'z')
	    twoUpper += 'A' - 'a';
	  if (oneUpper != twoUpper)
	    return COMPARE_NOT_EQUAL;
	  if (one != two)
	    warning = 1;
	}
      else if (one != two)
	return COMPARE_NOT_EQUAL;
    }
  return COMPARE_EQUAL;
#undef DEBUG_BASE_LEVEL
}

#ifdef	LOG_OPTION_SUPPORT

#define LOGFILENAME "/etc/TwentyOne.log"

#ifdef	WRITE_LOG_TO_CONSOLE
static unsigned char last_log_buffer[256];
static unsigned char log_buffer[256];
static int log_offset = 0;
#endif	/* WRITE_LOG_TO_CONSOLE */

static void writelog(const unsigned char *contents)
{
#define DEBUG_BASE_LEVEL 3
#ifdef	WRITE_LOG_TO_FILE
  char *logfilename;
  char buffer[256];
  int logdesc, option_save;
#endif	/* WRITE_LOG_TO_FILE */

  DEBUGS(0, "writelog");
  DEBUGS(1, " [");
  DEBUGS(1, (const char *)contents);
  DEBUGS(1, "]");
#ifdef	WRITE_LOG_TO_FILE
  option_save = flags.casewarn_option;
  flags.casewarn_option = 0;
  logfilename = buffer;
  if (GETENV("TwentyOneLogFileName", NULL, buffer) < 0)
    logfilename = LOGFILENAME;
  DEBUGS(1, " logfilename <");
  DEBUGS(1, logfilename);
  DEBUGS(1, ">");
  if ((logdesc = OPEN(logfilename, FILE_WRITING)) >= 0)
    {
      SEEK(logdesc, 0, 2);
      while (*contents)
	FPUTC(*contents++, logdesc);
      CLOSE(logdesc);
      DEBUGS(0, " wrote");
    }
  flags.casewarn_option = option_save;
#endif	/* WRITE_LOG_TO_FILE */
#ifdef	WRITE_LOG_TO_CONSOLE
  while (*contents && log_offset < 256)
    log_buffer[log_offset++] = *contents++;
#endif	/* WRITE_LOG_TO_CONSOLE */
  DEBUGS(0, CRLF);
#undef DEBUG_BASE_LEVEL
}

#endif	/* LOG_OPTION_SUPPORT */

static void writelog_eol()
{
#ifdef	WRITE_LOG_TO_FILE
  writelog((unsigned char *)CRLF);
#endif	/* WRITE_LOG_TO_FILE */

#ifdef	WRITE_LOG_TO_CONSOLE
  log_buffer[log_offset] = 0;
  log_offset = 0;
  {
    unsigned char *ptr1, *ptr2;

    ptr1 = log_buffer;
    ptr2 = last_log_buffer;
    while (1)
      {
	if (*ptr1 == 0 && *ptr2 == 0)
	  return;
	if (*ptr1 == '\t')
	  *ptr1 = '/';
	if (*ptr1 != *ptr2)
	  break;
	++ptr1;
	++ptr2;
      }
    while (*ptr1)
      *ptr2++ = *ptr1++;
    *ptr2 = 0;
  }
  iocs_print("TwentyOne : ");
  iocs_print((char *)last_log_buffer);
  iocs_print(CRLF);
#endif	/* WRITE_LOG_TO_CONSOLE */
}

static void writelog_direntry(const unsigned char *p, const unsigned char *s,
			      const unsigned char *e)
{
  unsigned char logbuf[8 + 10 + 3 + 4];
  int i;
  unsigned char *ptr;

  ptr = logbuf;
  for (i = 0; i < 8 && *p != ' '; ++i)
    *ptr++ = *p++;
  for (i = 0; i < 10 && *s != ' ' && *s; ++i)
    *ptr++ = *s++;
  if (e[0] != ' ')
    {
      *ptr++ = '.';
      for (i = 0; i < 3 && *e != ' ' && *e; ++i)
	*ptr++ = *e++;
    }
  *ptr = 0;
  writelog(logbuf);
}

static int disk_changed = 0;
static unsigned char disk_change[32];

int namecheck_dir_cmp(unsigned char *buf, unsigned char *disk)
{
#define DEBUG_BASE_LEVEL 4
  int ret_code;
  unsigned char *disk_p = disk;

  DEBUGS(0, "namecheck_dir_cmp");
  DEBUGS(1, "(");
  DEBUGL(1, (int)buf);
  DEBUGS(1, ",");
  DEBUGL(1, (int)disk);
  DEBUGS(1, ")");
  DEBUGS(1, "[");
  DEBUGS(1, (char *)buf);
  DEBUGS(1, "][");
  DEBUGS(1, (char *)disk);
  DEBUGS(1, "]");
#ifdef	USE_VFAT
  if ( !disk_changed ) {
    if ( vfat_ready == 2 ) vfat_ready = -1;
  }
#endif	/* USE_VFAT */
  disk_changed = 0;
  if (disk[0] == 5)
    disk[0] = 0xe5;
  is_2ndbyte = 0;
  warning = 0;
  if ( disk[11] == 0x0f ) {
#ifdef	USE_VFAT
    vfat_store_longname_directory( disk );
#endif	/* USE_VFAT */
    ret_code = COMPARE_NOT_EQUAL;
    goto quit;
  }
#ifdef	USE_VFAT
  if ( !flags.return_alias_vfat_option )
    {
      if ( vfat_ready > 0 )
	{
	  memcpy( disk_change, disk, sizeof(disk_change) );
	  disk_p = disk_change;
	  disk_changed = 1;
	  ret_code = vfat_compare(buf, disk_change);
	  if (ret_code == COMPARE_EQUAL) goto equal;
	  disk_p = disk; disk_changed = 0;
	}
    }
#endif	/* USE_VFAT */

  memcpy (disk_change, disk, sizeof (disk_change));
  disk_changed = 0;
  if (disk[12] < ' ')
    {
      int i;
#ifdef	USE_VFAT
      if (!flags.return_alias_vfat_option)
	{
	  if (disk_change[12] & 0x08)
	    {
	      /* disk_changed = 1; */
	      namecheck_dir_cmp_tolower (disk_change + 0, 8);
	      DEBUGS (1, " base tolower[");
	      DEBUGS (1, (char*) disk_change);
	      DEBUGS (1, "]");
	    }
	  if (disk_change[12] & 0x10)
	    {
	      /* disk_changed = 1; */
	      namecheck_dir_cmp_tolower (disk_change + 8, 3);
	      DEBUGS (1, " ext tolower[");
	      DEBUGS (1, (char*) disk_change);
	      DEBUGS (1, "]");
	    }
	}
#endif	/* USE_VFAT */
      for (i = 12; i < (12 + 10); i++) {
	if (disk_change[i]) {
	  if (!disk_changed) {
	    DEBUGS (1, " clear expansion[");
	    DEBUGS (1, (char*) disk_change);
	    DEBUGS (1, "]");
	  }
	  disk_change[i] = 0;
	  disk_changed = 1;
	}
      }
    }
  else
    {
      int i;
      for (i = (12 + 1); i < (12 + 6); i++) {

	/* 1998/10/24 立花: 時刻をクリアするようにした. */
	/* Windows(MS-DOS 6.x?)ではショートファイル名エントリの  */
	/* 18～19 バイト目にアクセス時刻が書き込まれる. Human68k */
	/* 拡張のノード名が 8+6 バイト未満なのにその二バイトが 0 */
	/* でなければ、時刻が書き込まれていると見なしクリアする. */

	if (disk_change[i] == 0) {
	  disk_change[12 + 6] = 0;
	  disk_change[12 + 7] = 0;
	  disk_changed = 1;
	  break;
	}
      }
    }
  disk_p = disk_changed ? disk_change : disk;

  if ((ret_code = compare(buf, disk_p, 8, ' ', '?')) == COMPARE_EQUAL)
    {
      if (flags.eleven_option)
	is_2ndbyte = 0;
      else
	ret_code = compare(buf + 11, disk_p + 12, 10, 0, '?');
      if (ret_code == COMPARE_EQUAL)
	ret_code = compare(buf + 8, disk_p + 8, 3, ' ', '?');
    }
  if (ret_code == COMPARE_NOT_EQUAL) {
    disk_p = disk; disk_changed = 0;
  }
#ifdef	USE_VFAT
equal:;
#endif	/* USE_VFAT */
  if (ret_code == COMPARE_EQUAL && warning)
    {
      writelog_direntry(buf, buf + 11, buf + 8);
      writelog((unsigned char *)" -1> ");
      writelog_direntry(disk_p, disk_p + 12, disk_p + 8);
      writelog_eol();
      ret_code = COMPARE_NOT_EQUAL;
    }
quit:;
  if (disk[0] == 0xe5)
    disk[0] = 5;
  DEBUGS(1, ret_code ? "not equal" CRLF : "equal");
  DEBUGS(0, CRLF);
  /* asm( "illegal" ); */
  return ret_code;
#undef DEBUG_BASE_LEVEL
}

int namecheck_namebf_cmp(namebuf *buf, namebuf *cmp)
{
#define DEBUG_BASE_LEVEL 4
  int ret_code;

  DEBUGS(0, "namecheck_namebf_cmp");
  DEBUGS(1, " drive [");
  DEBUGC(1, buf->drive + 'A');
  DEBUGS(1, "][");
  DEBUGC(1, cmp->drive + 'A');
  DEBUGS(1, "]");
  DEBUGS(1, " path [");
  DEBUGS(1, (char *)buf->path);
  DEBUGS(1, "][");
  DEBUGS(1, (char *)cmp->path);
  DEBUGS(1, "]");
  DEBUGS(1, " name [");
  DEBUGS(1, (char *)buf->primary);
  DEBUGS(1, "][");
  DEBUGS(1, (char *)cmp->primary);
  DEBUGS(1, "]");
  if (buf->drive != cmp->drive)
    return 1;
  is_2ndbyte = 0;
  warning = 0;
  if ((ret_code = compare(buf->path, cmp->path, 65, -1, -1)) == COMPARE_EQUAL
   && (ret_code = compare(buf->primary, cmp->primary, 8, 0, 0)) == COMPARE_EQUAL)
    {
      if (flags.eleven_option)
	is_2ndbyte = 0;
      else
	ret_code = compare(buf->secondary, cmp->secondary, 10, 0, 0);
      if (ret_code == COMPARE_EQUAL)
	ret_code = compare(buf->extendary, cmp->extendary, 3, 0, 0);
    }
  if (ret_code == COMPARE_EQUAL && warning)
    {
      writelog(buf->path);
      writelog((unsigned char *)" -2> ");
      writelog(cmp->path);
      writelog_eol();
      ret_code = COMPARE_NOT_EQUAL;
    }
  DEBUGS(1, ret_code ? " not equal" : " equal");
  DEBUGS(0, CRLF);
  return ret_code;
#undef DEBUG_BASE_LEVEL
}

int namecheck_knj_case_cmp(/* unsigned char *one_ptr, unsigned char *two_ptr, */
			   int num)
{
#define DEBUG_BASE_LEVEL 4
  int ret_code;

  DEBUGS(0, "namecheck_knj_case_cmp");
  DEBUGS(1, " num ");
  DEBUGL(1, num);
  DEBUGS(1, "[");
  DEBUGS(1, (char *)one_ptr);
  DEBUGS(1, "][");
  DEBUGS(1, (char *)two_ptr);
  DEBUGS(1, "]");
  is_2ndbyte = 0;
  warning = 0;
  ret_code = compare(one_ptr, two_ptr, num, 0, 0);
  if (ret_code == COMPARE_EQUAL && warning)
    {
      writelog(one_ptr);
      writelog((unsigned char *)" -3> ");
      writelog(two_ptr);
      writelog_eol();
      ret_code = COMPARE_NOT_EQUAL;
    }
  DEBUGS(1, ret_code ? " not equal" : " equal");
  DEBUGS(0, CRLF);
  return ret_code;
#undef DEBUG_BASE_LEVEL
}

int namecheck_pathset(int length, unsigned char *top_ptr
		      /* , unsigned char *now_ptr, unsigned char *setbuf_ptr */)
{
#define DEBUG_BASE_LEVEL 3
  unsigned char code;

  DEBUGS(0, "namecheck_pathset");
  DEBUGS(1, " top_ptr-1 ");
  DEBUGL(1, (int)top_ptr-1);
  DEBUGS(1, "[");
  DEBUGS(1, (char *)top_ptr-1);
  DEBUGS(1, "]");
  DEBUGS(1, " len ");
  DEBUGL(1, length);
  DEBUGS(1, " now-1 ");
  DEBUGL(1, (int)now_ptr-1);
  DEBUGS(1, "[");
  DEBUGS(1, (char *)now_ptr-1);
  DEBUGS(1, "]");
  DEBUGS(1, " setbuf_ptr ");
  DEBUGL(1, (int)setbuf_ptr);
  DEBUGS(0, CRLF);
  for (code = now_ptr[-1]; length > 0; code = *now_ptr++)
    {
      DEBUGS(2, "1<");
      DEBUGC(2, code);
      DEBUGS(2, ">" CRLF);
      if (code == '.')
	{
	  if (*now_ptr == '.')
	    {
	      if (now_ptr[1] == PATH_DELIM)
		{
		  length -= 2;
		  now_ptr += 2;
		  if (--setbuf_ptr != top_ptr)
		    while (*--setbuf_ptr != PATH_DELIM)
		      ;
		  code = PATH_DELIM;
		}
	      else if (now_ptr[1] == 0 || !flags.many_periods_option)
		return BADNAM;
	    }
	  else if (*now_ptr == PATH_DELIM)
	    {
	      --length;
	      now_ptr++;
	      if (--length == 0)
		break;
	      continue;
	    }
	  else if (*now_ptr == 0 || !flags.many_periods_option)
	    return BADNAM;
	}
      while (code != PATH_DELIM)
	{
	  DEBUGS(2, "2<");
	  DEBUGC(2, code);
	  DEBUGS(2, "> now ");
	  DEBUGL(2, (int)now_ptr);
	  DEBUGS(2, "[");
	  DEBUGS(2, (char *)now_ptr);
	  DEBUGS(2, "]" CRLF);
	  *setbuf_ptr++ = code;
	  if (--length == 0)
	    return 0;
	  code = *now_ptr++;
	}
#if 0 /* オリジナル */
      if (setbuf_ptr[-1] != PATH_DELIM)
#else /* バッファ先頭の場合は、強制的に PATH_DELIM を */
      /* 置かなければならない？ 93/10/11 */
      if (setbuf_ptr == top_ptr || setbuf_ptr[-1] != PATH_DELIM)
#endif
	*setbuf_ptr++ = PATH_DELIM;
      DEBUGS(2, "3<delimiter>" CRLF);
      if (--length == 0)
	break;
    }
  DEBUGS(1, "   end ");
  DEBUGL(1, (int)setbuf_ptr);
  DEBUGS(1, CRLF);
  return 0;
#undef DEBUG_BASE_LEVEL
}

int namecheck_pathok(unsigned char *filename, unsigned char *path_top,
		     namebuf *nameptr, unsigned char *path_bottom)
{
#define DEBUG_BASE_LEVEL 3
#define NORMAL 0
#define WILDCARDS 1
#define SPACES 2
#define ENDED 3
  int count, ch, flag;
  unsigned char *ptr;

  DEBUGS(0, "namecheck_pathok");
  DEBUGS(1, " filename (");
  DEBUGL(1, (int)filename);
  DEBUGS(1, ") [");
  DEBUGS(1, (char *)filename);
  DEBUGS(1, "]");
  DEBUGS(0, CRLF);
  DEBUGS(2, "    .. check" CRLF);

  if ((ch = filename[0]))
    {
      if (ch == '.')
	{
	  if ((ch = filename[1]) == 0)
	    goto no_filename_ended;
	  else if (ch == '.')
	    {
	      if ((ch = filename[2]) == 0)
		{
	       /* filename += 2; */
		  if (--path_bottom != path_top)
		    while (*--path_bottom != PATH_DELIM)
		      ;
		    *path_bottom++ = PATH_DELIM;
		    *path_bottom = 0;
		    goto no_filename_ended;
		}
	      else if (!flags.many_periods_option)
		return BADNAM;
	    }
	}
    }
  else
    {
no_filename_ended:
      nameptr->wild_card = -1;
      ptr = nameptr->primary;
      for (count = 0; count < 8; count++)
	*ptr++ = OSservice_ptr->noflcd;
      ptr = nameptr->secondary;
      for (count = 0; count < 10; count++)
	*ptr++ = 0;
      ptr = nameptr->extendary;
      for (count = 0; count < 3; count++)
	*ptr++ = OSservice_ptr->noflcd;
      DEBUGS(2, "    noflcd ");
      DEBUGC(2, OSservice_ptr->noflcd);
      DEBUGS(2, CRLF);
      return 0;
    }
  flag = NORMAL;
  DEBUGS(2, "    primary[");
  ptr = nameptr->primary;
  for (count = 0; count < 8; (void)({ DEBUGC(2, ch); count++; *ptr++ = ch; }))
    {
      ch = *filename++;
      if (ch == 0)
	{
	  for ( ; count < 8; count++)
	    *ptr++ = ' ';
	  DEBUGS(2, "]" CRLF "    secondary [(zero)]" CRLF);
	  ptr = nameptr->secondary;
	  for (count = 0; count < 10; count++)
	    *ptr++ = 0;
	  ptr = nameptr->extendary;
	  for (count = 0; count < 3; count++)
	    *ptr++ = OSservice_ptr->noflcd;
	  DEBUGS(2, "    extendary [(noflcd ");
	  DEBUGC(2, OSservice_ptr->noflcd);
	  DEBUGS(2, ")]" CRLF);
	  return 0;
	}
      if (ch == ':')
	{
	  DEBUGS(2, "] BADNAM error" CRLF);
	  return BADNAM;
	}
      if (ch == '*')
	{
	  flag = WILDCARDS;
	  break;
	}
      if (ch == '?')
	{
	  nameptr->wild_card++;
	  continue;
	}
      if (ch != '.')
	continue;
      if (!flags.many_periods_option)
	{
	  flag = SPACES;
	  break;
	}
      if (count == 0)
	continue;
      if (filename[0] == 0)
	continue;
      if (filename[0] == '.')
	continue;
      if (filename[1] == 0)
	{
	  flag = SPACES;
	  break;
	}
      if (filename[1] == '.')
	continue;
      if (filename[2] == 0)
	{
	  flag = SPACES;
	  break;
	}
      if (filename[2] == '.')
	continue;
      if (filename[3] == 0)
	{
	  flag = SPACES;
	  break;
	}
    }
  if (flag == SPACES)
    {
      if (count == 0)
	{
	  nameptr->wild_card = 8;
	  for ( ; count < 8; count++)
	    *ptr++ = OSservice_ptr->noflcd;
	}
      else
	for ( ; count < 8; count++)
	  *ptr++ = ' ';
    }
  else if (flag == WILDCARDS)
    {
      DEBUGS(2, "(wildcard)");
      nameptr->wild_card += 8 - count;
      for ( ; count < 8; count++)
	*ptr++ = '?';
    }
  DEBUGS(2, "]" CRLF "    secondary[");
  count = 0;
  ptr = nameptr->secondary;
  if (flag == NORMAL)
    {
      for ( ; count < 10; (void)({ DEBUGC(2, ch); count++; *ptr++ = ch; }))
	{
	  ch = *filename++;
	  if (ch == 0)
	    {
	      for ( ; count < 10; count++)
		*ptr++ = 0;
	      ptr = nameptr->extendary;
	      for (count = 0; count < 3; count++)
		*ptr++ = OSservice_ptr->noflcd;
	      DEBUGS(2, "]" CRLF "    extendary [(noflcd ");
	      DEBUGC(2, OSservice_ptr->noflcd);
	      DEBUGS(2, ")]" CRLF);
	      return 0;
	    }
	  if (ch == ':')
	    {
	      DEBUGS(2, "] BADNAM error" CRLF);
	      return BADNAM;
	    }
	  if (ch == '*')
	    {
	      flag = WILDCARDS;
	      break;
	    }
	  if (ch == '?')
	    {
	      nameptr->wild_card++;
	      continue;
	    }
	  if (ch != '.')
	    continue;
	  if (!flags.many_periods_option)
	    {
	      flag = SPACES;
	      break;
	    }
	  if (filename[0] == 0)
	    continue;
	  if (filename[0] == '.')
	    {
	      if (filename[1] || count != 9)
		continue;
	      flag = SPACES;
	      break;
	    }
	  if (filename[1] == 0)
	    {
	      flag = SPACES;
	      break;
	    }
	  if (filename[1] == '.')
	    {
	      if (filename[2] || count != 8)
		continue;
	      flag = SPACES;
	      break;
	    }
	  if (filename[2] == 0)
	    {
	      flag = SPACES;
	      break;
	    }
	  if (filename[2] == '.')
	    {
	      if (filename[3] || count != 7)
		continue;
	      flag = SPACES;
	      break;
	    }
	  if (filename[3] == 0)
	    {
	      flag = SPACES;
	      break;
	    }
	}
    }
  if (flag == SPACES)
    {
      for ( ; count < 10; count++)
	*ptr++ = 0;
      --filename;
    }
  else if (flag == WILDCARDS)
    {
      DEBUGS(2, "(wildcard)");
      nameptr->wild_card += 10 - count;
      for ( ; count < 10; count++)
	*ptr++ = '?';
    }
  DEBUGS(2, "]" CRLF "    extendary[");
  if (*filename)
    {
      if (*filename++ != '.')
	return NAMELEN;
      /* 1999/12/11 立花: 「空」の拡張子はエラーにする */
      if (*filename == 0 && flags.many_periods_option)
	return NAMELEN;
      OSservice_ptr->extflag = 1;
    }
  else
    {
      ptr = nameptr->extendary;
      for (count = 0; count < 3; count++)
	*ptr++ = OSservice_ptr->noflcd;
      DEBUGS(2, "(noflcd ");
      DEBUGC(2, OSservice_ptr->noflcd);
      DEBUGS(2, ")]" CRLF);
      return 0;
    }
  ptr = nameptr->extendary;
  for (count = 0; count < 3; (void)({ DEBUGC(2, ch); count++; *ptr++ = ch; }))
    {
      ch = *filename++;
      if (ch == 0)
	{
	  for ( ; count < 3; count++)
	    *ptr++ = ' ';
	  --filename;
	  break;
	}
      if (ch == ':')
	{
	  DEBUGS(2, "] BADNAM error" CRLF);
	  return BADNAM;
	}
      if (ch == '*')
	{
	  DEBUGS(2, "(wildcard)");
	  nameptr->wild_card += 3 - count;
	  for ( ; count < 3; count++)
	    *ptr++ = '?';
	  break;
	}
      if (ch == '?')
	{
	  nameptr->wild_card++;
	  continue;
	}
      if (ch == '.' && !flags.many_periods_option)
	return BADNAM;
    }
  if (*filename)
    {
      DEBUGS(2, "] EXTLEN error" CRLF);
      return EXTLEN;
    }
  DEBUGS(2, "] ");
  DEBUGB(2, nameptr->wild_card);
  DEBUGS(2, " normal end" CRLF);
  return 0;
#undef DEBUG_BASE_LEVEL
}

hexnamebuf *get_hexnamebuf(filbuf *entry_ptr)
{
#define DEBUG_BASE_LEVEL 3
  int offset;
  hexnamebuf *hexname;

  DEBUGS(0, "namecheck_get_hexnamebuf");
  DEBUGS(1, "(");
  DEBUGL(1, (int)entry_ptr);
  DEBUGS(1, ")");
  hexname = NULL;
  for (offset = 0; offset < flags.buffers_option; ++offset)
    {
#ifdef	PURGE_HEXNAMEBUF_IF_DUAL_LINK_BROKEN
      filbuf *work;

      work = hexnamebufs[offset].pair;
      if (((int)work)&0xffffff )
      if (work->name.pair != hexnamebufs + offset ||
	  work->name.Twen != Magic_Twen ||
	  work->name.ty != Magic_ty)
	hexnamebufs[offset].pair = NULL;
#endif	/* PURGE_HEXNAMEBUF_IF_DUAL_LINK_BROKEN */
      if (hexnamebufs[offset].pair == entry_ptr) {
        DEBUGS(1, " ");
        DEBUGL(1, (int)(hexnamebufs + offset) );
        DEBUGS(0, CRLF);
	return hexnamebufs + offset;
      }
      if ((hexnamebufs[offset].pair == NULL) && (hexname == NULL))
	hexname = hexnamebufs + offset;
    }
  DEBUGS(1, " ");
  DEBUGL(1, (int)(hexname) );
  DEBUGS(0, CRLF);
  return hexname;
#undef DEBUG_BASE_LEVEL
}

int namecheck_files_set(namebuf *buf, filbuf *entry)
{
#define DEBUG_BASE_LEVEL 3
  int count;
  unsigned char *work1, *work2;
  hexnamebuf *hexname;

  DEBUGS(0, "namecheck_files_set");
  DEBUGS(1, "(");
  DEBUGL(1, (int)buf);
  DEBUGS(1, ",");
  DEBUGL(1, (int)entry);
  DEBUGS(1, ")");
  DEBUGS(0, CRLF);
  if ((int)entry < 0)
    {
      entry->name.Twen = 0;
      return 0;
    }
  if ((hexname = get_hexnamebuf(entry)) == NULL)
    return NOMORE;
  hexname->pair = entry;
  entry->name.pair = hexname;
  entry->name.Twen = Magic_Twen;
  entry->name.ty = Magic_ty;
  entry->name.end_flag = 0;
  work1 = (unsigned char *)&(hexname->name);
  work2 = buf->primary;
  for (count = 0; count < 8 + 3 + 12; count++)
    *work1++ = *work2++;
  return 0;
#undef DEBUG_BASE_LEVEL
}

exnamebuf *namecheck_files_get(filbuf *entry)
{
#define DEBUG_BASE_LEVEL 3
  hexnamebuf *hexname;

  DEBUGS(0, "namecheck_files_get");
  DEBUGS(1, "(");
  DEBUGL(1, (int)entry);
  DEBUGS(1, ")");
  DEBUGS(0, CRLF);
  if ((int)entry < 0)
    return (exnamebuf *)&(entry->name);
  if (entry->name.Twen != Magic_Twen)
    return (exnamebuf *)NOMORE;
  if (entry->name.ty != Magic_ty)
    return (exnamebuf *)NOMORE;
  if (entry->name.end_flag)
    return (exnamebuf *)NOMORE;
  hexname = entry->name.pair;
  if (hexname->pair != entry)
    return (exnamebuf *)NOMORE;
  return &(hexname->name);
#undef DEBUG_BASE_LEVEL
}

void namecheck_files_err(filbuf *entry)
{
#define DEBUG_BASE_LEVEL 3
  hexnamebuf *hexname;

  DEBUGS(0, "namecheck_files_err");
  DEBUGS(1, "(");
  DEBUGL(1, (int)entry);
  DEBUGS(1, ")");
  DEBUGS(0, CRLF);
  if ((int)entry < 0)
    return;
  if (entry->name.Twen != Magic_Twen)
    return;
  if (entry->name.ty != Magic_ty)
    return;
  if (entry->name.end_flag)
    return;
  entry->name.end_flag = 1;
  hexname = entry->name.pair;
  entry->name.pair = NULL;
  hexname->pair = NULL;
#undef DEBUG_BASE_LEVEL
}

static int
check (unsigned char code)
{
#define DEBUG_BASE_LEVEL 3
  return (code == ':' || code == '?' || code == '*');
#undef DEBUG_BASE_LEVEL
}


/* 1998/10/09 立花: 末尾以外の空白を削除しないように修正. */

int namecheck_files_ent(/* unsigned char *one_ptr, */ int flag, unsigned char *buf)
{
#define DEBUG_BASE_LEVEL 3
  int count, exitcode = -1;
  unsigned char *src_ptr, *top;
  unsigned char code;

  DEBUGS(0, "namecheck_files_ent");
  DEBUGS(1, " (");
  DEBUGL(1, (int)buf );
  DEBUGS(1, ")");

#if 0	/* lndrv が常駐していないと _CHDIR に失敗する不具合の修正. */
  if (disk_changed) {
    buf = disk_change;
    flag = 1;
  }
#else	/* 1998/06/26 立花: 多分これでいい筈… */
  if (flag && disk_changed)
    buf = disk_change;
#endif
  src_ptr = buf;
  DEBUGS(1, " (");
  DEBUGL(1, (int)src_ptr );
  DEBUGS(1, ")");
  DEBUGS(0, "[");
  DEBUGS(0, src_ptr);
  DEBUGS(0, "]");

  /* 第一ファイル名 */
  top = one_ptr;
  code = *src_ptr++;
  if (flag && code == 0x05)
    code = 0xe5;
  for (count = 0; count < 8; ++count)
    {
      if (check(code))
	goto err_exit;
      *one_ptr++ = code;
      code = *src_ptr++;
    }

  /* 第二ファイル名 */
  src_ptr = buf + 11 + flag;
  for (count = 0; count < 10; ++count)
    {
      if ((code = *src_ptr++) == 0)
	break;
      if (check(code))
	goto err_exit;
      *one_ptr++ = code;
    }
  /* ノード末尾の空白を削除する */
  while (*--one_ptr == ' ')
    if (one_ptr == top)
      goto err_exit;
  one_ptr++;

  /* 拡張子 */
  top = one_ptr;
  *one_ptr++ = '.';
  src_ptr = buf + 8;
  for (count = 0; count < 3; ++count)
    {
      code = *src_ptr++;
      if (check(code))
	goto err_exit;
      *one_ptr++ = code;
    }
  /* 拡張子末尾の空白を削除する(全て空白なら '.' も不要) */
  while (*--one_ptr == ' ')
    ;
  if (top < one_ptr)
    one_ptr++;

  *one_ptr = 0;
  exitcode = 0;
err_exit:;

  DEBUGS(0, "[");
  DEBUGS(0, p);
  DEBUGS(0, "]" CRLF);
  return exitcode;
#undef DEBUG_BASE_LEVEL
}

#ifdef	C_ERRCHR
int namecheck_errchr(int ch)
{
#define DEBUG_BASE_LEVEL 3
  DEBUGS(0, "namecheck_errchr");
  DEBUGS(1, " [");
  DEBUGB(1, ch);
  DEBUGS(1, "(");
  DEBUGC(1, ch);
  DEBUGS(1, ")]");
  DEBUGS(0, CRLF);

#if 0	/* original */
  return (ch != '/');
#else
  return 1;
#endif

#undef DEBUG_BASE_LEVEL
}
#endif	/* C_ERRCHR */

int convert_yen_slash(unsigned char *src, /* unsigned char *setbuf_ptr, */ int num)
{
#define DEBUG_BASE_LEVEL 4
  int count, start_flag;
  unsigned char code;

  DEBUGS(0, "convert_yen_slash");
  DEBUGS(1, "(");
  DEBUGL(1, (int)src);
  DEBUGS(1, ",");
  DEBUGL(1, (int)setbuf_ptr);
  DEBUGS(1, ")");
  DEBUGS(1, " filepath [");
  DEBUGS(1, (char *)src);
  DEBUGS(1, "] num ");
  DEBUGL(1, num);
  DEBUGS(0, CRLF);
  start_flag = 1;
  for (count = num; count > 0; --count, *setbuf_ptr++ = code)
    {
      code = *src++;
      if (code == ' ' && start_flag)
	{
	  while (*src++ == code)
	    ;
	  code = src[-1];
	}
      if (code == '\0')
	{
	  *setbuf_ptr = 0;
	  return 0;
	}

      if (0x80 <= code && (code < 0xa0 || 0xe0 <= code))
	{
	  *setbuf_ptr++ = code;
	  if (--count == 0)
	    return BADNAM;
	  code = *src++;
	  if (code < ' ')
	    return BADNAM;
	  start_flag = 0;
	  continue;
	}
      if (code == ' ')
	{
	  unsigned char *save = src;

	  while (*src++ == code)
	    ;
	  code = src[-1];
	  if (code == 0)
	    {
	      *setbuf_ptr = 0;
	      return 0;
	    }
	  if (code == '.' || code == '/' || code == '\\')
	    ;		/* ピリオド直前か末尾の空白は無視する */
	  else
	    {
	      if (flags.special_option)
		{
		  src = save;
		  code = ' ';
		}
	      else
		return BADNAM;
	    }
	}
      if (flags.special_option)
	{
	  if (code == 8 || code == 9)
	    return BADNAM;
	}
      else
	{
	  if (code < ' ')
	    return BADNAM;
	  if (code == '-' && start_flag)
	    return BADNAM;
	  if (code =='\"' || code =='\'' || code == '+' || code == ',' ||
	      code == ';' || code == '<' || code == '=' || code == '>' ||
	      code == '[' || code == ']' || code == '|')
	    return BADNAM;
	}

      if (code == ':')
	{
	  start_flag = 1;
	  continue;
	}
      if (code == '/' || code == '\\')
	{
	  code = 9;
	  start_flag = 1;
	  continue;
	}
      start_flag = 0;
    }
  return BADNAM;
#undef DEBUG_BASE_LEVEL
}

int namecheck_yenslh(/* unsigned char *now_ptr, unsigned char *setbuf_ptr */)
{
#define DEBUG_BASE_LEVEL 3
  int num;

  DEBUGS(0, "namecheck_yenslh");
  DEBUGS(1, "(");
  DEBUGL(1, (int)now_ptr);
  DEBUGS(1, ",");
  DEBUGL(1, (int)setbuf_ptr);
  DEBUGS(1, ")");
  DEBUGS(1, " filepath [");
  DEBUGS(1, (char *)now_ptr);
  DEBUGS(1, "]");
  DEBUGS(0, CRLF);

  *setbuf_ptr = 0;

  /* 先頭の空白を無視する */
  while (*now_ptr++ == (char)' ')
    ;
  if (*--now_ptr == (char)'\0')
    {
      *setbuf_ptr = '\0';
      return 0;
    }

  num = 90;
  if ((((*now_ptr == '/') && flags.sysroot_option) ||
      ((*now_ptr == '\\') && flags.sysroot2_option)) && sysroot[0])
    {
      int retcode;
      unsigned char *ptr_save;

      ptr_save = setbuf_ptr;
      retcode = convert_yen_slash(sysroot, /* setbuf_ptr, */ num);
      if (retcode < 0)
	return retcode;
      num -= setbuf_ptr - ptr_save;
    }
  return convert_yen_slash(now_ptr, /* setbuf_ptr, */ num);
#undef DEBUG_BASE_LEVEL
}



/****************************************************************/
/*								*/
/*			デバッグ用ルーチン			*/
/*								*/
/****************************************************************/

#ifdef	DEBUG

static int _DEBUG_MATCH(int level)
{
  if (flags.debug1_option && (level <= 1))
    return 1;
  if (flags.debug2_option && (level <= 2))
    return 1;
  if (flags.debug3_option && (level <= 3))
    return 1;
  if (flags.debug4_option && (level <= 4))
    return 1;
  if (flags.debug5_option && (level <= 5))
    return 1;
  return 0;
}

void _DEBUGS(int level, const char *str)
{
  if (_DEBUG_MATCH(level))
    if ( (*((unsigned char *)(0x810))) & 0x20 ) {
      debug_print(str);
    }
}

void _DEBUGC(int level, int cod)
{
  if (_DEBUG_MATCH(level))
    if ( (*((unsigned char *)(0x810))) & 0x20 ) {
      debug_putchar(cod);
    }
}

void _DEBUGN(int level, int cod)
{
  int val;

  if (!_DEBUG_MATCH(level))
    return;
  val = cod & 15;
  _DEBUGC(level, (val < 10) ? val + '0' : val + 'A' - 10);
}

void _DEBUGB(int level, int cod)
{
  if (!_DEBUG_MATCH(level))
    return;
  _DEBUGN(level, cod >> 4);
  _DEBUGN(level, cod);
}

void _DEBUGW(int level, int cod)
{
  if (!_DEBUG_MATCH(level))
    return;
  _DEBUGB(level, cod >> 8);
  _DEBUGB(level, cod);
}

void _DEBUGL(int level, int cod)
{
  if (!_DEBUG_MATCH(level))
    return;
  _DEBUGW(level, cod >> 16);
  _DEBUGW(level, cod);
}

#endif	/* DEBUG */


/* EOF */
