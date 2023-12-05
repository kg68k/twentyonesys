/*
 * $Project: TwentyOne for Human Ver 3 on X680x0 $
 * $Copyright: 1991,92,93 by Ｅｘｔ(T.Kawamoto) $
 * $Source: /home/src/cvs/TwentyOne3/namecheck.c,v $
 * $Author: kawamoto $
 * $Revision: 1.23 $
 * $Date: 1994/12/13 07:31:19 $
 */

#include "namecheck.h"
#include "vfat.h"


UniCode vfat_namebuf[VFAT_NAME_LEN+1];
unsigned char vfat_sjisnamebuf[VFAT_NAME_LEN+1];
int vfat_nameidx;
int vfat_diridx;
int vfat_ready;
int vfat_checksum;


void namecheck_dir_cmp_tolower( unsigned char *buf, int len )
{
	int	i;
	unsigned char	c;
	int	knjflag;

	knjflag = 0;
	for ( i=0; i<len; i++ ) {
		c = buf[i];
		if ( knjflag ) {
			knjflag = 0;
		} else {
			if (0x80 <= c && c <= 0x9f || 0xe0 <= c && c <= 0xef) {
				knjflag = 1;
			} else {
				if ( ('A'<=c) && (c<='Z') ) buf[i] = c+('a'-'A');
			}
		}
	}
}

int mbforword( unsigned char *p, int idx, int len )
{
	int knjflag;
	int l;
	int c;

	knjflag = 0;
	for ( l=idx; l<idx+len; l++ ) {
		c = p[l];
		if ( knjflag ) {
			if ( c == 0 ) {
				l--;
				break;
			}
			knjflag = 0;
		} else {
			if ( c == 0 ) break;
			if ( ((0x80 <= c) && (c <= 0x9f)) || ((0xe0 <= c) && (c <= 0xef)) ) {
				if ( l+1 >= len ) break;
				knjflag = !0;
			}
		}
	}

	return (l);
}

void vfat_init( void )
{
#define DEBUG_BASE_LEVEL 4
	DEBUGS(0, "vfat_init");

	vfat_nameidx = -1;
	vfat_diridx = 0;
	vfat_ready = -1;
	vfat_checksum = 0;
	vfat_namebuf[VFAT_NAME_LEN] = 0x0000;

	DEBUGS(0, "\r\n");
#undef DEBUG_BASE_LEVEL
}

void vfat_store_longname_directory( unsigned char *disk )
{
#define DEBUG_BASE_LEVEL 4
	DEBUGS(0, "vfat_store_longname_directory");
	DEBUGS(1, "[");
	DEBUGS(1, (char *)disk);
	DEBUGS(1, "]");

	/* VFATエントリ開始であることをチェック */
	if ( disk[0]&0x40 ) {
		/* VFATエントリ開始 */
		vfat_nameidx = -1;
		vfat_diridx = disk[0] & 0x3f;
		vfat_ready = 0;
		vfat_checksum = disk[13];
	}
	/* VFATエントリ番号が不正であることをチェック */
	if ( ( (disk[0]&0x3f) != vfat_diridx) ||
		/* VFATエントリ番号が不正 */
	       (vfat_checksum != disk[13]) ) {
		vfat_nameidx = -1;
		vfat_ready = -1;
	}
	/* VFATエントリ用ワークのクリア */
	if ( vfat_nameidx == -1 ) {
		vfat_nameidx = VFAT_NAME_LEN;
	}
	/* VFATエントリ用ワークへ追加 */
	if ( vfat_ready == 0 ) {
		vfat_nameidx -= 13;
		if ( vfat_nameidx < 0 ) {
			/* VFATエントリ用ワーク追加失敗 */
			vfat_ready = -1;
		} else {
			/* VFATエントリ用ワーク追加 */
			memcpy( &vfat_namebuf[vfat_nameidx+ 0], disk+ 1, 10 );
			memcpy( &vfat_namebuf[vfat_nameidx+ 5], disk+14, 12 );
			memcpy( &vfat_namebuf[vfat_nameidx+11], disk+28,  4 );
			vfat_diridx--;
			if ( vfat_diridx == 0 ) {
				/* VFATエントリ用ワーク完成 */
				vfat_ready = 1;
			}
		}
	}

	DEBUGS(1, " vfat_ready ");
	DEBUGL(1, vfat_ready);
	DEBUGS(0, "\r\n");
#undef DEBUG_BASE_LEVEL
}

int vfat_compare( unsigned char *buf, unsigned char *disk )
{
#define DEBUG_BASE_LEVEL 4
	int ret_code;
	int i;
	int idx;
	UniCode uc;
	SJisCode sc;
	int len;
	int lastidx;
	int lastbaseidx = -1;
	int lastextidx = -1;
	int firstextidx = -1;
	unsigned char cmpbuf[TWENTYONE_BASE_MAX];
	int sum;

	DEBUGS(0, "\r\nvfat_compare");
	DEBUGS(1, "[");
	DEBUGS(1, (char *)buf);
	DEBUGS(1, "]\r\n");

	/* VFATエントリ用ワークが完成していることのチェック */
	if ( vfat_ready < 1 ) {
		/* VFATエントリ用ワークが未完成ならコンペア失敗 */
		DEBUGS(0, "vfat_entry not ready\r\n");
		vfat_ready = -1;
		return (COMPARE_NOT_EQUAL);
	}

	DEBUGS(0, "vfat_compare:entry ready\r\n");

	/* VFATエントリのチェックサム情報と実際のエントリの */
	/* チェックサムが合っていることのチェック */
	sum = 0;
	for ( i=0; i<11; i++ ) {
		sum = ((sum&1) ? 0x80 : 0x00 )+(sum>>1);
		sum = (sum+disk[i])&0xff;
	}
	if ( sum != vfat_checksum ) {
		/* チェックサムが違えばコンペア失敗 */
		vfat_ready = -1;
		DEBUGS(0, "vfat_compare:bad checksum\r\n");
		return (COMPARE_NOT_EQUAL);
	}

	DEBUGS(0, "vfat_compare:ok checksum\r\n");

	/* UnicodeをShift-JISへ変換 */
	idx = 0;
	for ( i=vfat_nameidx; i<VFAT_NAME_LEN; i++ ) {
		uc = vfat_namebuf[i];
		if ( uc == 0 ) break;
		uc = ((uc&0xff)<<8)|(uc>>8);
		sc = Uni2SJis(uc);
		if ( sc < 256 ) {
			if ( sc == ' ' ) sc = '_';
			vfat_sjisnamebuf[idx++] = sc;
		} else {
			vfat_sjisnamebuf[idx++] = (sc>>8)&0xff;
			vfat_sjisnamebuf[idx++] = sc&0xff;
		}
	}
	vfat_sjisnamebuf[idx] = 0x00;
	len = idx;
	lastidx = idx;

	DEBUGS(1, "vfat_compare:convert [");
	DEBUGS(1, (char *)vfat_namebuf);
	DEBUGS(1, "] to [");
	DEBUGS(1, (char *)vfat_sjisnamebuf);
	DEBUGS(1, "]\r\n");
	DEBUGS(1, "phase 1 [len=");
	DEBUGL(1, len);
	DEBUGS(1, "][lastidx=");
	DEBUGL(1, lastidx);
	DEBUGS(1, "]\r\n");

	/* 最終拡張子がTWENTYONE_EXT_MAX文字以内であれば、lastextとして得る */
	/* TWENTYONE_EXT_MAX文字を超えていれば、TWENTYONE_BASE_MAX文字まで */
	/* をfirstextとして得る */
	while ( idx > 1 ) {
		idx--;
		if ( vfat_sjisnamebuf[idx] == '.' ) {
			if ( lastidx-idx <= TWENTYONE_EXT_MAX+1 ) {
				lastextidx = idx;
			} else {
				firstextidx = idx;
				lastextidx = mbforword( vfat_sjisnamebuf, firstextidx, TWENTYONE_BASE_MAX );
				lastidx = lastextidx;
			}
			lastbaseidx = idx;
			break;
		}
	}

	DEBUGS(1, "phase 2 [lastbaseidx=");
	DEBUGL(1, lastbaseidx);
	DEBUGS(1, "][firsextidx=");
	DEBUGL(1, firstextidx);
	DEBUGS(1, "][lastextidx=");
	DEBUGL(1, lastextidx);
	DEBUGS(1, "][lastidx=");
	DEBUGL(1, lastidx);
	DEBUGS(1, "]\r\n");

	/* 非最終拡張子を後ろからTWENTYONE_BASE_MAX文字以内でfirstextとして得る */
	while ( idx > 0 ) {
		idx--;
		if ( vfat_sjisnamebuf[idx] == '.' ) {
			if ( lastextidx-idx <= TWENTYONE_BASE_MAX-(idx==0 ? 0 : 1) ) {
				firstextidx = idx;
			}
			lastbaseidx = idx;
		}
	}

	DEBUGS(1, "phase 3 [lastbaseidx=");
	DEBUGL(1, lastbaseidx);
	DEBUGS(1, "][firsextidx=");
	DEBUGL(1, firstextidx);
	DEBUGS(1, "][lastextidx=");
	DEBUGL(1, lastextidx);
	DEBUGS(1, "][lastidx=");
	DEBUGL(1, lastidx);
	DEBUGS(1, "]\r\n");

	/* 各インデックス補完処理 */
	if ( lastextidx < 0 ) lastextidx = lastidx;
	if ( firstextidx < 0 ) firstextidx = lastextidx;
	if ( lastbaseidx < 0 ) lastbaseidx = firstextidx;

	DEBUGS(1, "phase 4 [lastbaseidx=");
	DEBUGL(1, lastbaseidx);
	DEBUGS(1, "][firsextidx=");
	DEBUGL(1, firstextidx);
	DEBUGS(1, "][lastextidx=");
	DEBUGL(1, lastextidx);
	DEBUGS(1, "][lastidx=");
	DEBUGL(1, lastidx);
	DEBUGS(1, "]\r\n");

	/* base＋firstextがTWENTYONE_BASE_MAX文字であれば、baseを切り詰める */
	if ( lastbaseidx + (lastextidx-firstextidx) > TWENTYONE_BASE_MAX ) {
		lastbaseidx = mbforword( vfat_sjisnamebuf, 0, TWENTYONE_BASE_MAX - (lastextidx-firstextidx) );
	}

	DEBUGS(1, "phase 5 [lastbaseidx=");
	DEBUGL(1, lastbaseidx);
	DEBUGS(1, "][firsextidx=");
	DEBUGL(1, firstextidx);
	DEBUGS(1, "][lastextidx=");
	DEBUGL(1, lastextidx);
	DEBUGS(1, "][lastidx=");
	DEBUGL(1, lastidx);
	DEBUGS(1, "]\r\n");

	/* cmpbufへ転送 */
	memset( cmpbuf, ' ', TWENTYONE_PRIMARY_MAX );
	memset( cmpbuf+TWENTYONE_PRIMARY_MAX, 0, TWENTYONE_SECONDARY_MAX );
	memcpy( cmpbuf, &vfat_sjisnamebuf[0], lastbaseidx );
	memcpy( cmpbuf+lastbaseidx, &vfat_sjisnamebuf[firstextidx], lastextidx-firstextidx );

	/* diskへ転送 */
	memcpy( disk, cmpbuf, TWENTYONE_PRIMARY_MAX );
	memset( disk+8, ' ', TWENTYONE_EXT_MAX );
	memcpy( disk+8, &vfat_sjisnamebuf[lastextidx+1], lastidx - (lastextidx+1) );
	memcpy( disk+12, cmpbuf+TWENTYONE_PRIMARY_MAX, TWENTYONE_SECONDARY_MAX );

	DEBUGS(1, "[");
	DEBUGL(1, lastbaseidx);
	DEBUGS(1, " ");
	DEBUGL(1, firstextidx);
	DEBUGS(1, " ");
	DEBUGL(1, lastextidx);
	DEBUGS(1, " ");
	DEBUGL(1, len);
	DEBUGS(1, "][");
	DEBUGS(1, (char *)disk);
	DEBUGS(1, "]");

	/* 比較 */
	if ((ret_code = compare(buf, disk, 8, ' ', '?')) == COMPARE_EQUAL) {
		if (flags.eleven_option) {
			is_2ndbyte = 0;
		} else {
			ret_code = compare(buf + 11, disk + 12, 10, 0, '?');
			if (ret_code == COMPARE_EQUAL) {
				ret_code = compare(buf + 8, disk + 8, 3, ' ', '?');
			}
		}
	}

	if ( ret_code ) vfat_ready = -1; else vfat_ready = 2;

	DEBUGS(1, ret_code ? "not equal\r\n" : "equal");
	DEBUGS(0, "\r\n");

	return (ret_code);
#undef DEBUG_BASE_LEVEL
}

/* EOF */
