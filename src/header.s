*
* $Project: TwentyOne for Human Ver 3 on X680x0 $
* $Copyright: 1991,92,93 by Ｅｘｔ(T.Kawamoto) $
* $Source: /home/src/cvs/TwentyOne3/header.s,v $
* $Author: kawamoto $
* $Revision: 1.12 $
* $Date: 1994/12/13 07:10:18 $
*

* Include File -------------------------------- *

		.include	doscall.mac
		.include	twoncall.mac
		.include	patchlevel.mac


* Constant ------------------------------------ *

NAMECHK_ID:	.equ	4

HumanMemEnd:	.equ	$1c00

TAB:		.equ	$09
LF:		.equ	$0a
CR:		.equ	$0d

CRLF:		.reg	CR,LF

sizeof_EXBUF:	.equ	26


* Text Section -------------------------------- *

		.cpu	68000

		.text
		.even
*keep_top:
device_table:
		.dc.l	-1
		.dc	$8000
		.dc.l	device_stratage
		.dc.l	device_interrupt
		.dc.b	'?Twenty?'

_flags::
options:	.dc	0
option_B:	.dc	256
*_masks::
		.dc	0,0
_sysroot::	.ds.b	_SYSROOT_MAX
		.fail	_SYSROOT_MAX.ne.90

namecheck_table:
		jmp	namecheck_reset
		jmp	namecheck_init
		jmp	namecheck_close
		jmp	namecheck_chrdevck
		jmp	namecheck_dir_cmp
		jmp	namecheck_namebf_cmp
		jmp	namecheck_knj_case_cmp
		jmp	namecheck_pathset
		jmp	namecheck_pathok
		jmp	namecheck_files_set
		jmp	namecheck_files_get
		jmp	namecheck_files_err
		jmp	namecheck_files_ent
		jmp	namecheck_errchr
		jmp	namecheck_yenslh
		jmp	dummy
		jmp	dummy
		jmp	dummy
		jmp	dummy
		jmp	dummy

***************************************************************************
*
* device driver initialize
*
device_request:
		.dc.l	0
device_stratage:
		move.l	a5,(device_request)
		rts

device_interrupt:
		movem.l	d1-d7/a0-a6,-(sp)
		movea.l	(device_request,pc),a5
		move	#$5003,d0
		tst.b	(2,a5)
		bne	device_ret		;コマンド番号が異常

* デバイス初期化
		pea	(title_mes,pc)		;タイトル表示
		DOS	_PRINT

		bsr	check_option		;引数解析
		bsr	do_initialize		;初期化
		tst.l	d0
		beq	device_ret

		move.l	d0,(sp)			;組み込み不可
		DOS	_PRINT
		move	#$700d,d0		
device_ret:
		addq.l	#4,sp
		move.b	d0,(3,a5)
		move	d0,-(sp)
		move.b	(sp)+,(4,a5)
		movem.l	(sp)+,d1-d7/a0-a6
		rts

***************************************************************************
*
*将来の拡張用なのでd0=-1でリターンしておくこと。
*
dummy:
namecheck_reset:
		moveq	#-1,d0
		rts

***************************************************************************
*
*全てのＮＡＭＥ管理バッファを初期状態に初期化する。
*このルーチンはＮＡＭＥ管理をパッチするときにＯＳから呼ばれる。
*in	a5=ＯＳサービスエントリーの先頭アドレス
*break	flg
*
namecheck_init:
	movem.l	d0-d2/a0-a2,-(sp)
	.xref	_namecheck_init
	bsr	_namecheck_init
	movem.l	(sp)+,d0-d2/a0-a2
	rts
*
***************************************************************************
*
*全てのバッファを、未使用状態にする。
*このルーチンはFFLUSHファンクションやSTOPキーを処理したときあるいは、
*ＮＡＭＥ管理を別のものにパッチする前に呼ばれる。
*in	a5=ＯＳサービスエントリーの先頭アドレス
*break	flg
*
namecheck_close:
	movem.l	d0-d2/a0-a2,-(sp)
	.xref	_namecheck_close
	bsr	_namecheck_close
	movem.l	(sp)+,d0-d2/a0-a2
	rts
*
***************************************************************************
*
*	namebfの名前が、chrデバイスかどうかをチェックする。
*	chrdevckを使ってチェックする。
*in	a2=namebf
*	a5=ＯＳサービスエントリーの先頭アドレス
*out	d0=dev_adr or -1
*break	a0,flg
*
namecheck_chrdevck:
	movem.l	d1/d2/a1/a2,-(sp)
	move.l	a2,-(sp)
	.xref	_namecheck_chrdevck
	bsr	_namecheck_chrdevck
	addq.l	#4,sp
	movem.l	(sp)+,d1/d2/a1/a2
	rts
*
***************************************************************************
*	指定バッファの内容が、ドライブ上の内容と同じかどうかをチェックする
*	files/nfilesやopen/delete/rename/chdir/rmdir等で使用している。
*in	a2	比較するバッファ
*	a1	ドライブ上のデータバッファ
*	d0	(a1)の内容（$05なら$e5に変換ずみ）
*out
*	zero_flg
*break
*	d0.b
*
namecheck_dir_cmp:
	movem.l	d0-d2/a0-a2,-(sp)
	move.l	a1,-(sp)
	move.l	a2,-(sp)
	.xref	_namecheck_dir_cmp
	bsr	_namecheck_dir_cmp
	add.l	#8,sp
	tst.l	d0
	movem.l	(sp)+,d0-d2/a0-a2
	rts
*
***************************************************************************
*
*	filesファンクションが成功した場合にdirbufにnfiles用の名前をセットする。
*in	a2=namebf
*	a1=dirbuf
*out	d0=error code
*break	a1/a3
*
namecheck_files_set:
	movem.l	d1/d2/a0/a2,-(sp)
	move.l	a1,-(sp)
	bclr	#7,(sp)
	move.l	a2,-(sp)
	.xref	_namecheck_files_set
	bsr	_namecheck_files_set
	add.l	#8,sp
	movem.l	(sp)+,d1/d2/a0/a2
	rts
*
***************************************************************************
*
*in	a1=dirbuf
*break	d0/a1
*
namecheck_files_err:
	movem.l	d1/d2/a0/a2,-(sp)
	move.l	a1,-(sp)
	bclr	#7,(sp)
	.xref	_namecheck_files_err
	bsr	_namecheck_files_err
	addq.l	#4,sp
	movem.l	(sp)+,d1/d2/a0/a2
	rts
*
***************************************************************************
*
*	nfilesのバッファ先頭を与えて、名前比較バッファ先頭(上記a2)を返す。
*in	a2=user files buffer
*out	a2=check name pos	d0=error code
*break	flg
*
namecheck_files_get:
	movem.l	d1/d2/a0/a1,-(sp)
	move.l	a2,-(sp)
	bclr	#7,(sp)
	.xref	_namecheck_files_get
	bsr	_namecheck_files_get
	move.l	d0,(sp)+
	bmi	namecheck_files_get_error
	move.l	d0,a2
	moveq	#0,d0
namecheck_files_get_error
	movem.l	(sp)+,d1/d2/a0/a1
	rts
*
***************************************************************************
*in	d0=top char
*	d2=length
*	a0=top_adr
*	a1=now_adr
*	a3=set_buffer
*out	d0=error_code(0=ok)
*	a1=next adr
*	a3=next set adr
*break	d2.w
*
namecheck_pathset:
	movem.l	d1/a0/a2/a4/a5,-(sp)
	move.l	a3,a4
	move.l	a1,a5
	move.l	a0,-(sp)
	move.l	d2,-(sp)
	.xref	_namecheck_pathset
	bsr	_namecheck_pathset
	add.l	#8,sp
	move.l	a5,a1
	move.l	a4,a3
	movem.l	(sp)+,d1/a0/a2/a4/a5
	rts
*
***************************************************************************
*
*in	a1=file name buffer
*	a0=path_top_adr
*	a2=name_buffer(88 bytes)
*	a3=path_end_adr(\ pos)
*	a5=ＯＳサービスエントリーの先頭アドレス
*out	d0.l=error
*break	d0/d1
*
namecheck_pathok:
	movem.l	d2/a0-a2,-(sp)
	move.l	a3,-(sp)
	move.l	a2,-(sp)
	move.l	a0,-(sp)
	move.l	a1,-(sp)
	.xref	_namecheck_pathok
	bsr	_namecheck_pathok
	lea	(16,sp),sp
	movem.l	(sp)+,d2/a0-a2
	rts
*
***************************************************************************
*
*in	a2=namebf
*	a3=lock/fastopen等のバッファ
*out	zero_flg=1..cmp_ok
*break	d0/d1/d2
*
namecheck_namebf_cmp:
	movem.l	a0-a2,-(sp)
	move.l	a3,-(sp)
	move.l	a2,-(sp)
	.xref	_namecheck_namebf_cmp
	bsr	_namecheck_namebf_cmp
	addq.l	#8,sp
	tst.l	d0
	movem.l	(sp)+,a0-a2
	rts
*
***************************************************************************
*
*in	a5/a4
*out	d2/a5/zero_flg
*break	d0/d1/d2
*
namecheck_knj_case_cmp:
	movem.l	a0-a2,-(sp)
	addq.l	#1,d0
	move.l	d0,-(sp)
	.xref	_namecheck_knj_case_cmp
	bsr	_namecheck_knj_case_cmp
	moveq	#0,d2
	move.b	(-1,a5),d2
	move.l	d0,(sp)+
	movem.l	(sp)+,a0-a2
	rts
*
***************************************************************************
*
*	files/nfilesが成功した場合に、disk_bufferからpacked_nameを作成する
*	mkdir/chdirでnamebfのnamnm1からの２１バイトをpathの後に付ける。
*in	a2=dir_buff packed_name_buff top
*	d1=1(dir_buff_mode/files用) or 0(namebf.namnm1 mode/chdir用)
*	a1=disk dir buffer(name8+ext3+atr?+name10 top)
*out	d0.l=0..ok  /-1..error
*	a2=end pos(00)
*	sign_flg=1..error
*break	a3/d0/d1
*
namecheck_files_ent:
	movem.l	d2/a0/a1/a5,-(sp)
	move.l	a1,-(sp)
	move.l	d1,-(sp)
	move.l	a2,a5
	.xref	_namecheck_files_ent
	bsr	_namecheck_files_ent
	addq.l	#8,sp
	move.l	a5,a2
	tst.l	d0
	movem.l	(sp)+,d2/a0/a1/a5
	rts
*
***************************************************************************
*
*先頭と最後ＳＰは全てカット、［.］の前のＳＰもカット、それ以外のＳＰはエラー
*$00-$1f,の文字はエラー
*$80-$9f,$e0-$ffは２バイト文字の第一バイト（第二バイトは$20-$ff）
*(cmpdat の先頭が０ならば このルーチンが必要 )
*in     a1=path address(end=0)
*       a4=new path buffer(90bytes)
*break  d0.b/d1.l/a1.l/a0.l
*out    bmi     error
namecheck_yenslh:
	movem.l	d2/a2/a5,-(sp)
	move.l	a1,a5
	.xref	_namecheck_yenslh
	bsr	_namecheck_yenslh
	tst.l	d0
	movem.l	(sp)+,d2/a2/a5
	rts
*
***************************************************************************
*
*	指定の文字がファイル名として正しいかどうかのチェック
*in	d0=file_name_chr
*out	zero_flg=1...error
*break	flg
*
namecheck_errchr:
		.ifdef	C_ERRCHR
	movem.l	d0-d2/a0-a2,-(sp)
	clr.l	-(sp)
	move.b	d0,(3,sp)
	.xref	_namecheck_errchr
	bsr	_namecheck_errchr
	move.l	d0,(sp)+
	movem.l	(sp)+,d0-d2/a0-a2
		.else
		tst.b	d0			;常に d0 != 0
		.endif
	rts


***************************************************************************
*
*	引数解析
*
_path_buf::
check_option:
		movea.l	($12,a5),a0
skip_filename:
		tst.b	(a0)+			;ファイル名を飛ばす
		bne	skip_filename

		lea	(_flags,pc),a1
		move.l	(a1),d7
		bchg	#_TWON_T_BIT,d7
arg_loop:
		move.b	(a0)+,d0
		beq	arg_end
		cmpi.b	#'+',d0
		beq	check_opt_plus
		cmpi.b	#'-',d0
		beq	check_opt_minus
*arg_sysroot:
		movem.l	d1-d7/a0-a6,-(sp)
		pea	(-1,a0)
		lea	(sp),a6			;arg ptr
		.xref	_set_sysroot_direct
		bsr	_set_sysroot_direct	;SYSROOT を設定する
		move.l	d0,(sp)+
		beq	@f
		pea	(sysr_err_mes,pc)	;指定がおかしい
		DOS	_PRINT
		addq.l	#4,sp
@@:		movem.l	(sp)+,d1-d7/a0-a6
skip_arg:
		tst.b	(a0)+
		bne	skip_arg
		bra	arg_loop
arg_end:
		bchg	#_TWON_T_BIT,d7
		move.l	d7,(a1)
		rts

check_opt_plus:
		moveq	#-1,d6			;bit set
		bra	check_opt_next
check_opt_minus:
		moveq	#0,d6			;bit clear
check_opt_next:
		move.b	(a0)+,d0
		beq	arg_loop
		cmpi.b	#'+',d0
		beq	check_opt_plus
		cmpi.b	#'-',d0
		beq	check_opt_minus
		.ifdef	DEBUG
		cmpi.b	#'1',d0
		beq	check_opt_1
		cmpi.b	#'2',d0
		beq	check_opt_2
		cmpi.b	#'3',d0
		beq	check_opt_3
		cmpi.b	#'4',d0
		beq	check_opt_4
		cmpi.b	#'5',d0
		beq	check_opt_5
		.endif
		andi	#$df,d0
		cmpi.b	#'B',d0
		beq	check_opt_bufsize
		cmpi.b	#'X',d0
		beq	check_opt_expatch

CHKOPT:		.macro	char,num
		moveq	#num,d1
		cmpi.b	char,d0
		beq	@f
		.endm
		CHKOPT	#'V',_TWON_V_BIT
		CHKOPT	#'C',_TWON_C_BIT
		CHKOPT	#'S',_TWON_S_BIT
		CHKOPT	#'P',_TWON_P_BIT
		CHKOPT	#'T',_TWON_T_BIT
		CHKOPT	#'F',_TWON_F_BIT
		CHKOPT	#'R',_TWON_R_BIT
		CHKOPT	#'W',_TWON_W_BIT
		CHKOPT	#'Y',_TWON_Y_BIT
		CHKOPT	#'A',_TWON_A_BIT

		move.b	d0,(opt_err_mes_a-_flags,a1)
		pea	(opt_err_mes,pc)	;不正なオプション
		bra	check_opt_error
@@:
		tst	d6
		beq	@f
		bset	d1,d7
		bra	check_opt_next
@@:		bclr	d1,d7
		bra	check_opt_next

check_opt_expatch:
		move.b	d6,(expatch_flag-_flags,a1)
		bra	check_opt_next

check_opt_bufsize:
		cmpi.b	#'=',(a0)+
		beq	@f
		subq.l	#1,a0
@@:		bsr	getnum
		bhi	check_opt_b_err		;数字がない
		move.l	d0,d1			;先頭桁
check_opt_b_loop:
		bsr	getnum
		bhi	check_opt_b_end
		mulu	#10,d1
		add.l	d0,d1
		cmpi.l	#$ffff,d1		;オーバーフロー検査
		bls	check_opt_b_loop
check_opt_b_err:
		pea	(buf_err_mes,pc)
check_opt_error:
		DOS	_PRINT
		addq.l	#4,sp
		bra	skip_arg		;文字列の残りは無視する
check_opt_b_end:
		tst	d1
		beq	check_opt_b_err		;-b0
		move	d1,d7
		bra	check_opt_next

getnum:
		moveq	#0,d0
		move.b	(a0)+,d0
		subi.b	#'0',d0
		cmpi.b	#9,d0
		bls	@f
		subq.l	#1,a0
@@:		rts


		.ifdef	DEBUG
check_opt_1:	moveq	#_TWON_D1_BIT,d0
		bra	@f
check_opt_2:	moveq	#_TWON_D2_BIT,d0
		bra	@f
check_opt_3:	moveq	#_TWON_D3_BIT,d0
		bra	@f
check_opt_4:	moveq	#_TWON_D4_BIT,d0
		bra	@f
check_opt_5:	moveq	#_TWON_D5_BIT,d0
@@:
		andi.l	#.not.(%11111<<16),d7
		tst	d6
		beq	@f
		bset	d0,d7
		bra	check_opt_next
@@:		bclr	d0,d7
		bra	check_opt_next
		.endif


***************************************************************************
*
*	初期化ルーチン
*
* out	d0.l	0:OK !0:error
*
do_initialize:
		DOS	_VERNUM			;Human68k のバージョン検査
		cmpi	#$0302,d0
		beq	@f
		pea	(ver_err_mes,pc)
		bra	do_init_error
@@:
		moveq	#sizeof_EXBUF,d0
		mulu	(option_B,pc),d0
		addi.l	#_hexnamebufs+$40000,d0
		cmp.l	(HumanMemEnd),d0
		bls	@f
		pea	(mem_err_mes,pc)	;メモリが足りない
		bra	do_init_error
@@:
		pea	(namecheck_table,pc)
		move	#NAMECHK_ID,-(sp)
		DOS	_OS_PATCH
		addq.l	#6,sp
		move.l	d0,a0
		cmpi.l	#'?Twe',(-106,a0)	;二重組み込み検査
		bne	@f
		cmpi.l	#'nty?',(-102,a0)
		bne	@f

* 既に常駐していた場合
		move.l	a0,-(sp)		;テーブルを元に戻す
		move	#NAMECHK_ID,-(sp)
		DOS	_OS_PATCH
		addq.l	#6,sp
		pea	(already_mes,pc)
do_init_error:
		move.l	(sp)+,d0		;組み込み失敗
		rts
@@:
		moveq	#sizeof_EXBUF,d0
		mulu	(option_B,pc),d0
		lea	(_hexnamebufs),a0
		moveq	#0,d1
		lsr	#2,d0
		bcc	@f
		move	d1,(a0)+
		bra	@f
clr_buf_loop:
		move.l	d1,(a0)+
@@:		dbra	d0,clr_buf_loop
		clr	d0
		subq.l	#1,d0
		bcc	clr_buf_loop
		move.l	a0,($0e,a5)		;常駐末尾アドレス

		.xref	patch
		bsr	patch			;パッチをあてる

		.xref	 _dos_twon
		pea	(_dos_twon,pc)		;$ffb0 にファンクションコール
		move	#_TWON,-(sp)		;を割り当てる
		DOS	_INTVCS
		addq.l	#6,sp

		moveq	#0,d0			;組み込み成功
		rts


***************************************************************************
*
*	.x メインルーチン
*
execute:
		DOS	_EXIT


* Data Section -------------------------------- *

*		.data
		.even

expatch_flag::	.dc.b	0

title_mes:	.dc.b	CRLF
		.dc.b	'TwentyOne (とぅぇにぃわん)'
		.ifdef	DEBUG
		.dc.b	' Debug'
		.endif
		.dc.b	' version 1.36c (for Human68k version 3.02)',CRLF
		.dc.b	TAB,'Copyright 1991,92,93,94 by Ｅｘｔ(T.Kawamoto)',CRLF
		.dc.b	TAB,'modified +14 by GORRY.',CRLF
		.dc.b	TAB,'patchlevel ',PATCHLEVEL,' : ',PATCHDATE,' ',PATCHAUTHOR,'.',CRLF
		.dc.b	0
		.even

ver_err_mes:	.dc.b	'Human68k version 3.02 以外には対応していません.',CRLF,0
mem_err_mes:	.dc.b	'メモリが足りません.',CRLF,0
already_mes:	.dc.b	'既に組み込まれています.',CRLF,0
sysr_err_mes:	.dc.b	'SYSROOT の指定が正しくありません.',CRLF,0
buf_err_mes:	.dc.b	'バッファサイズの指定が正しくありません.',CRLF,0
opt_err_mes:	.dc.b	'未対応のオプションです('
opt_err_mes_a:	.dc.b	'?).',CRLF,0


* Stack Section ------------------------------- *

		.stack
		.even
_hexnamebufs::
*keep_end:

		.end	execute

* End of File --------------------------------- *
