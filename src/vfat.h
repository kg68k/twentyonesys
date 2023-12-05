#include "u2stbl.h"

extern UniCode vfat_namebuf[VFAT_NAME_LEN+1];
extern unsigned char vfat_sjisnamebuf[VFAT_NAME_LEN+1];
extern int vfat_nameidx;
extern int vfat_diridx;
extern int vfat_ready;
extern int vfat_checksum;

void namecheck_dir_cmp_tolower( unsigned char *buf, int len );
int mbforword( unsigned char *p, int idx, int len );
void vfat_init( void );
void vfat_store_longname_directory( unsigned char *disk );
int vfat_compare( unsigned char *buf, unsigned char *disk );

/* EOF */
