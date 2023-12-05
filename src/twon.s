		.title	twon - (V)TwentyOne.sys の設定及び状態表示

program:	.reg	'twon'
version:	.reg	'1.06'
date:		.reg	'1999/05/20'
author:		.reg	'立花えり子'


* Include Files ------------------------------- *

		.include	fefunc.mac	;__LTOS
		.include	doscall.mac
		.include	twoncall.mac

		.ifdef	_TWON_D_BIT
		.fail	1
		.endif


STDERR:		.equ	2

LF:		.equ	$0a
CR:		.equ	$0d
CRLF:		.reg	CR,LF


* Text Section -------------------------------- *

		.cpu	68000

		.text
		.even

		bra	@f
		.dc.b	'#HUPAIR',0
@@:
		bsr	twon_sys_check_sub
		bne	@f			;getid 以外は呼び出さない
		move	#_TWON_GETOPT,-(sp)
		DOS	_TWON			;オプションの収得
		addq.l	#2,sp
@@:
		move.l	d0,d7			;表示用
		move.l	d0,d6			;変更用

		moveq	#0,d5			;bit 31=1:オプション設定
						;bit  7=1:SYSROOT 設定
		pea	(1,a2)
		bsr	GetArgCharInit
		addq.l	#4,sp
next_arg:
		bsr	GetArgChar
		tst.l	d0
		bmi	arg_end
		beq	arg_nul			;twon "" : SYSROOT の初期化
next_option:
		moveq	#0,d1
		cmpi.b	#'-',d0
		beq	arg_minus
		moveq	#1,d1
		cmpi.b	#'+',d0
		beq	arg_plus
* SYSROOT の指定
arg_nul:
		lea	(str_buf,pc),a0
		move.b	d0,(a0)+
		moveq	#_SYSROOT_MAX-2,d1
@@:
		bsr	GetArgChar
		move.b	d0,(a0)+
		dbeq	d1,@b
		bne	sysr_long_error		;SYSROOT が長すぎる

		tas	d5			;bit 7=1
		bra	next_arg

arg_minus:
		bsr	GetArgChar
		cmpi.b	#'-',d0
		beq	long_option		;--long-option
		bra	@f
arg_plus:
		bsr	GetArgChar
@@:		tst.b	d0
		beq	arg_error		;'-','+'のみはエラー
next_flag:
		cmpi.b	#'?',d0
		beq	print_usage
		andi	#$df,d0
		cmpi.b	#'H',d0
		beq	print_usage
		cmpi.b	#'O',d0
		beq	print_flag		;フラグ表示

		bset	#31,d5			;bit 31=1
		lea	(flag_table,pc),a0
		moveq	#32,d2
flag_loop:
		subq	#1,d2
		move.b	(a0)+,d3
		beq	flag_error
		cmp.b	d0,d3
		bne	flag_loop

		bclr	d2,d6			;-x
		tst	d1
		beq	flag_next
		bset	d2,d6			;+x
flag_next:
		bsr	GetArgChar
		tst.b	d0
		beq	next_arg

		cmpi.b	#'-',d0
		beq	@f
		cmpi.b	#'+',d0
@@:		beq	next_option		;-x+y
		bra	next_flag


* --sysroot, --help, --version
long_option:
		subq.l	#LONGOPT_MAX+1,sp
		lea	(sp),a0
		moveq	#LONGOPT_MAX,d1
get_long_opt_loop:
		bsr	GetArgChar
		move.b	d0,(a0)+
		dbeq	d1,get_long_opt_loop
		bne	arg_error		;長すぎる

		lea	(long_opt_tbl,pc),a0
long_opt_cmp_loop:
		move	(a0)+,d0
		beq	arg_error		;存在しないオプション
		lea	(long_opt_tbl,pc,d0.w),a1
		move	(a0)+,d0
		lea	(sp),a2
@@:
		cmpm.b	(a1)+,(a2)+		;文字列比較
		bne	long_opt_cmp_loop
		tst.b	(-1,a1)
		bne	@b
		jmp	(long_opt_tbl,pc,d0.w)

LONGOPT:	.macro	str,job
		.dc	str-long_opt_tbl,job-long_opt_tbl
		.endm
long_opt_tbl:
		LONGOPT	str_help	,print_usage
		LONGOPT	str_sysroot	,print_sysroot
		LONGOPT	str_sysver	,print_sysver
		LONGOPT	str_version	,print_version
		.dc	0


* 引数解析終了.
* SYSROOT やオプションの指定がなければ設定状況の表示へ飛ぶ.
* どちらかの指定があればそれを設定する.
arg_end:
		bsr	twon_sys_check		;常駐検査

		tst.l	d5
		beq	print_all_flag
		bpl	set_sysroot		;SYSROOT のみ
*set_flag:
		move.l	d6,-(sp)
		move	#_TWON_SETOPT,-(sp)
		DOS	_TWON			;オプションの設定
		addq.l	#6,sp
		tst	d5
		beq	skip_set_sysr		;オプションのみ
set_sysroot:
		pea	(str_buf,pc)
		move	#_TWON_SETSYSR,-(sp)
		DOS	_TWON			;SYSROOT の設定
		addq.l	#6,sp
		tst.l	d0
		bmi	sysr_error
skip_set_sysr:
		DOS	_EXIT


* 全ての設定状況を表示する.
* 形式は「+OPTION -OPTION -B=bufsize SYSROOT=SYSROOT」.

print_all_flag:
		lea	(str_buf,pc),a0
		move.l	a0,-(sp)

		moveq	#'+',d0
		bsr	flag_to_str2		;'+'設定のオプション

		not.l	d7
		moveq	#'-',d0
		bsr	flag_to_str2
		not.l	d7

		move.b	#'-',(a0)+
		move.b	#'B',(a0)+
		move.b	#'=',(a0)+
		moveq	#0,d0
		move	d7,d0			;バッファサイズ
		FPACK	__LTOS

		lea	(a0),a1
		lea	(sysr_equ,pc),a2
@@:		move.b	(a2)+,(a1)+		;" SYSROOT="
		bne	@b
		subq.l	#1,a1
print_sysroot2:
		pea	(a1)
		move	#_TWON_GETSYSR,-(sp)
		DOS	_TWON			;SYSROOT の収得
		addq.l	#6,sp
		tst.b	(a1)
		beq	print_flag_flush	;空なら表示しない
@@:
		tst.b	(a0)+			;SYSROOT を飛ばす
		bne	@b
		move.b	#'/',(-1,a0)		;末尾に'/'追加
		bra	print_flag_flush


* --sysroot
* 現在の SYSROOT を表示する.
print_sysroot:
		bsr	twon_sys_check		;常駐検査

		lea	(str_buf,pc),a0
		move.l	a0,-(sp)
		lea	(a0),a1
		bra	print_sysroot2


* --sysver
* 組み込まれている (V)TwentyOne.sys のバージョンを表示する.
print_sysver:
		bsr	twon_sys_check		;常駐検査

		move	#_TWON_GETVER,-(sp)
		DOS	_TWON
		addq.l	#2,sp

		lea	(sysver_mes_end,pc),a0
		bsr	print_sysver_num	;整数部
		move.b	#'.',(a0)+
		bsr	print_sysver_num	;小数部
		bsr	print_sysver_num
		bsr	take_4bit
		beq	skip_alphabet
		addi.b	#'a'-$a,d1
		move.b	d1,(a0)+		;英字
skip_alphabet:
		bsr	take_8bit
		beq	skip_modified
		move.b	#'+',(a0)+
		bsr	print_sysver_num2	;modified
skip_modified:
		lea	(patchlevel_mes,pc),a1
@@:		move.b	(a1)+,(a0)+
		bne	@b
		subq.l	#1,a0
		bsr	take_8bit
		bsr	print_sysver_num2	;patchlevel
* 表示
		pea	(sysver_mes,pc)
		move.l	#_TWON_U2S<<16+' ',-(sp)
		DOS	_TWON
		move.l	d0,(sp)+
		bpl	@f			;VTwentyOne.sys
		addq.l	#1,(sp)
@@:		bra	print_flag_flush	; TwentyOne.sys

print_sysver_num:
		bsr	take_4bit
		bra	print_sysver_num1
take_4bit:
		rol.l	#4,d0
		moveq	#$f,d1
		and.b	d0,d1
		rts
print_sysver_num2:
		divu	#10,d1
		beq	@f
		bsr	print_sysver_num1
@@:		swap	d1
print_sysver_num1:
		addi.b	#'0',d1
		move.b	d1,(a0)+
		rts
take_8bit:
		rol.l	#8,d0
		moveq	#0,d1
		move.b	d0,d1
		rts


* +o/-o
* '+'または'-'設定のオプションのみ表示する.
print_flag:
		bsr	twon_sys_check		;常駐検査

		tst	d1
		bne	@f
		not.l	d7			;-oの時は0のbitを表示する
@@:
		lea	(str_buf,pc),a0
		move.l	a0,-(sp)
		bsr	flag_to_str
print_flag_flush:
		move.b	#CR,(a0)+
		move.b	#LF,(a0)+
		clr.b	(a0)
		DOS	_PRINT
		addq.l	#4,sp
		DOS	_EXIT

flag_to_str2:
		move.b	d0,(a0)+
		bsr	flag_to_str
		move.b	d0,(a0)+
		bne	@f
		subq.l	#2,a0			;一つもなければ'-'/'+'も削る
@@:		rts

flag_to_str:
		moveq	#0,d0
		move.l	d7,d2
		lea	(flag_table,pc),a1
		bra	@f
print_flag_loop:
		add.l	d2,d2
		bcc	@f
		move.b	d1,(a0)+
		moveq	#' ',d0
@@:
		move.b	(a1)+,d1
		bne	print_flag_loop
print_flag_print:
		rts


* (V)TwentyOne.sys が組み込まれているか調べる.
* 組み込まれていなければエラー終了する.
twon_sys_check:
		bsr	twon_sys_check_sub
		bne	no_twentyone		;(V)TwentyOne.sys が組み込まれていない
		rts

twon_sys_check_sub:
		.if	 _TWON_GETID
		move	#_TWON_GETID,-(sp)
		.else
		clr	-(sp)
		.endif
		DOS	_TWON			;識別子の収得
		addq.l	#2,sp
		cmpi.l	#_TWON_ID,d0
		rts


* エラー処理 ---------------------------------- *

print_version:
		lea	(ver_mes_end,pc),a0
		move.b	#CR,(a0)+
		move.b	#LF,(a0)+
		clr.b	(a0)
		pea	(title_mes,pc)
		DOS	_PRINT
		addq.l	#4,sp
		DOS	_EXIT

print_usage:
		pea	(title_mes,pc)
		DOS	_PRINT
		pea	(usage_mes,pc)
		DOS	_PRINT
		addq.l	#8,sp
		DOS	_EXIT

no_twentyone:
		lea	(nosys_err_mes,pc),a0
		bra	error_exit
flag_error:
		lea	(flag_err_mes,pc),a0
		bra	error_exit
arg_error:
		lea	(arg_err_mes,pc),a0
		bra	error_exit
sysr_long_error:
		lea	(sysr_lerr_mes,pc),a0
		bra	error_exit
sysr_error:
		lea	(sysr_err_mes,pc),a0
		bra	error_exit

error_exit:
		move	#STDERR,-(sp)
		move.l	a0,-(sp)
		DOS	_FPUTS
		addq.l	#6,sp

		move	#1,-(sp)
		DOS	_EXIT2


* HUPAIR Decoder ------------------------------ *

		.if	0
GetArgChar_p:	.dc.l	0
GetArgChar_c:	.dc.b	0
		.even
		.else
GetArgChar_p:	.equ	GetArgCharInit
GetArgChar_c:	.equ	GetArgCharInit+4
		.endif

GetArgChar:
		movem.l	d1/a0-a1,-(sp)
		moveq	#0,d0
		lea	(GetArgChar_p,pc),a0
		movea.l	(a0)+,a1
		move.b	(a0),d0
		bmi	GetArgChar_noarg
GetArgChar_quate:
		move.b	d0,d1
GetArgChar_next:
		move.b	(a1)+,d0
		beq	GetArgChar_endarg
		tst.b	d1
		bne	GetArgChar_inquate
		cmpi.b	#' ',d0
		beq	GetArgChar_separate
		cmpi.b	#"'",d0
		beq	GetArgChar_quate
		cmpi.b	#'"',d0
		beq	GetArgChar_quate
GetArgChar_end:
		move.b	d1,(a0)
		move.l	a1,-(a0)
GetArgChar_abort:
		movem.l	(sp)+,d1/a0-a1
		rts
GetArgChar_endarg:
		st	(a0)
		bra	GetArgChar_abort
GetArgChar_noarg:
		moveq	#1,d0
		ror.l	#1,d0
		bra	GetArgChar_abort

GetArgChar_inquate:
		cmp.b	d0,d1
		bne	GetArgChar_end
		clr.b	d1
		bra	GetArgChar_next

GetArgChar_separate:
		cmp.b	(a1)+,d0
		beq	GetArgChar_separate
		moveq	#0,d0
		tst.b	-(a1)
		beq	GetArgChar_endarg
		bra	GetArgChar_end

GetArgCharInit:
		movem.l	a0-a1,-(sp)
		movea.l	(12,sp),a1
GetArgCharInit_skip:
		cmpi.b	#' ',(a1)+
		beq	GetArgCharInit_skip
		tst.b	-(a1)
		lea	(GetArgChar_c,pc),a0
		seq	(a0)
		move.l	a1,-(a0)
		movem.l	(sp)+,a0-a1
		rts


* Data Section -------------------------------- *

		.data
		.even

title_mes:	.dc.b	program,' version ',version
ver_mes_end:	.dc.b	' ',date,' ',author,'.',CRLF,0

flag_table:	.dc.b	'VCSPTFRWYA',0
sysr_equ:	.dc.b	' SYSROOT=',0

* 追加する場合は long_opt_tbl も変更すること.
str_help:	.dc.b	'help',0
str_sysroot:	.dc.b	'sysroot',0
str_sysver:	.dc.b	'sysver',0
str_version:	.dc.b	'version',0
LONGOPT_MAX:	.equ	7

patchlevel_mes:	.dc.b	' patchlevel ',0
sysver_mes:	.dc.b	'V','TwentyOne.sys version '
sysver_mes_end:
* 以下、破壊される.

str_buf:
usage_mes:	.dc.b	'usage: ',program,' [option] [+flag] [-flag] [sysroot]',CRLF
		.dc.b	'option:',CRLF
		.dc.b	'	+v / -v		詳細表示',CRLF
		.dc.b	'	+c / -c		大文字小文字区別',CRLF
		.dc.b	'	+s / -s		特殊記号許可',CRLF
		.dc.b	'	+p / -p		複数終止符',CRLF
		.dc.b	'	+t / -t		21 文字判別',CRLF
		.dc.b	'	+f / -f		検索属性補正',CRLF
		.dc.b	'	+r / -r		SYSROOT 使用(/)',CRLF
		.dc.b	'	+w / -w		不一致警告',CRLF
		.dc.b	'	+y / -y		SYSROOT 使用(\)',CRLF
		.dc.b	'	+a / -a		短縮名使用',CRLF
		.dc.b	'	+o / -o		設定表示',CRLF
		.dc.b	'	(省略)		詳細表示',CRLF
		.dc.b	'	--sysroot	SYSROOT 表示',CRLF
		.dc.b	'	--sysver	ドライババージョン表示',CRLF
		.dc.b	0

nosys_err_mes:	.dc.b	'(V)TwentyOne.sys が組み込まれていません.',CRLF,0
flag_err_mes:	.dc.b	'フラグが正しくありません.',CRLF,0
arg_err_mes:	.dc.b	'引数が正しくありません.',CRLF,0
sysr_lerr_mes:	.dc.b	'SYSROOT が長すぎます.',CRLF,0
sysr_err_mes:	.dc.b	'SYSROOT が正しくありません.',CRLF,0


		.end

* End of File --------------------------------- *
