		.title	(V)TwentyOne.sys - patch.s


* Include File -------------------------------- *

		.include	doscall.mac
		.include	iocscall.mac
		.include	twoncall.mac


* Global Symbol ------------------------------- *

		.xref	_flags
		.xref	expatch_flag


* Constant ------------------------------------ *

NOP_CODE:	.equ	$4e71
JMP_ABSL:	.equ	$4ef9
BEQ_S:		.equ	$67
BHI_S:		.equ	$62

TAB:		.equ	$09

;file attribute
*EXEC:		.equ	7
*LINK:		.equ	6
ARCHIVE:	.equ	5
DIRECTORY:	.equ	4
VOLUME:		.equ	3
SYSTEM:		.equ	2
HIDDEN:		.equ	1
READONLY:	.equ	0

NAMESTS_Path:	.equ	2
NAMESTS_SIZE:	.equ	88


* IOCS Work ----------------------------------- *

MPUTYPE:	.equ	$cbc


* Macro --------------------------------------- *

STRLEN:		.macro	areg,dreg
		.local	loop
		move.l	areg,dreg
loop:		tst.b	(areg)+
		bne	loop
		subq.l	#1,areg
		exg	dreg,areg
		sub.l	areg,dreg
		.endm


* Text Section -------------------------------- *

		.cpu	68000

		.text
		.even

* メモリ上の Human68k に以下のパッチをあてる.
*	・directory が延びないバグのフィックス(原作:Ｅｘｔ氏)
*	・VFAT 対策拡張 EXPAND patch(原作:GORRY 氏)
*	・18+'.'+3 文字のディレクトリに chdir できない不具合の修正
*	・実行ファイルのパスデリミタに '/' が使えない不具合の修正
*	・仮想ディレクトリ/ドライブの不具合の修正
*	・実行ファイルの拡張子収得の不具合の修正
*	・実行ファイル名の不具合の修正
*	・DOS _NAMECK でスペースを正しく処理できない不具合の修正
*	・DOS _RMDIR でファイルがあっても削除する不具合の修正
*	・DOS _RENAME の不具合の修正
*	・その他の不具合の修正
*	・拡張パッチ
* スーパーバイザモードで呼び出すこと.
* Human68k version 3.02 専用.
* in	なし
* out	d0.l	0:正常終了 -1:Human68k のバージョンが違う.

patch::
		DOS	_VERNUM
		cmpi	#$302,d0
		bne	human_version_error

		bsr	patch302_increase_dir
		.ifdef	_TWON_F_BIT
		bsr	patch302_files
		.else
		bsr	patch302_vfat_expand
		.endif
		bsr	patch302_chdir_dir
		bsr	patch302_subst
		bsr	patch302_exec_ext
		bsr	patch302_exec_name
		bsr	patch302_pathchk_slash
		bsr	patch302_pathchk_ext
		bsr	patch302_nameck
		bsr	patch302_rename
		bsr	patch302_rmdir
		bsr	patch302_others
		move.b	(expatch_flag,pc),d0
		beq	@f
		bsr	patch302_expatch
@@:
		cmpi.b	#1,(MPUTYPE)
		bls	cache_flush_skip
		move.l	d1,-(sp)
		moveq	#3,d1			;cache flush
		IOCS	_SYS_STAT
		move.l	(sp)+,d1
cache_flush_skip:
		moveq	#0,d0
		rts
human_version_error:
		moveq	#-1,d0
		rts


* directory が延びないバグのフィックス -------- *
*        by NCD02474 高橋雅雄（Ｇａｏ）

patch302_increase_dir:
		pea	(patch302_increase_dir_patch_b8e2,pc)
		move	#JMP_ABSL,($b8e2)
		move.l	(sp)+,($b8e2+2)
		rts

patch302_increase_dir_patch_b8e2:
		moveq	#0,d0
		move	($14,a0),d0		;DPB_DataTopSec
		cmp.l	d0,d1
		jmp	($b8e8)


**L00b8e2:	move	(DPB_DataTopSec,a0),d0	;jmp
**		cmp	d0,d1			;  (patch302_increase_dir_patch).l
**L00b8e8:	bcc	L00b8f0			;<- jmp ($b8e8)


* DOS _FILES 属性補正(+F) --------------------- *
* +F 指定時に、検索対象にアーカイブ属性が含まれていたら
* $01(読み込み専用属性のみ)のファイルを一致させる.
* EXPAND patch を含む.

		.ifdef	_TWON_F_BIT
patch302_files:
		pea	(patch302_files_patch_be1a,pc)
		move	#JMP_ABSL,($be1a)
		move.l	(sp)+,($be1a+2)
		rts

patch302_files_patch_be1a:
		move.b	($0b,a1),d5		;DIR_Atr
		beq	patch302_files_patch_archive
		cmpi.b	#$0f,d5
		beq	patch302_files_patch_match
						;VFAT エントリなら確定
		btst	#_TWON_F_BIT-24,(_flags,pc)
		beq	patch302_files_patch_skip
		cmpi.b	#1<<READONLY,d5
		bne	patch302_files_patch_skip
		btst	#ARCHIVE,d7
		beq	patch302_files_patch_skip
patch302_files_patch_match:
		swap	d7			;+F、属性 $01、Archive 検索なら確定
		jmp	($be2a)

patch302_files_patch_archive:
		moveq	#1<<ARCHIVE,d5		;$00 は Archive 属性と見なす
patch302_files_patch_skip:
		jmp	($be22)

**		swap	d7
**L00be1a:	move.b	(DIR_Atr,a1),d5		;jmp
**		bne	L00be22			; (patch302_files_patch_be1a).l
**		moveq	#1<<ARCHIVE,d5
**L00be22:	and.b	d7,d5			;<- jmp ($be22)
**		swap	d7
**		tst.b	d5
**		beq	L00be3c
**L00be2a:	cmp.b	#$05,d0			;<- jmp ($be2a)

		.endif


* VFAT 対策拡張 EXPAND patch ------------------ *

		.ifndef	_TWON_F_BIT
patch302_vfat_expand:
		pea	(patch302_vfat_expand_patch_be18,pc)
		move	#JMP_ABSL,($be18)
		move.l	(sp)+,($be18+2)
		rts

patch302_vfat_expand_patch_be18:
		move.b	($0b,a1),d5		;DIR_Atr
		cmpi.b	#$0f,d5
		beq	@f			;VFAT エントリなら確定
		swap	d7
		tst.b	d5
		jmp	($be1e)
@@:		jmp	($be2a)


**L00be18:	swap	d7			;jmp
**		move.b	(DIR_Atr,a1),d5		; (patch302_vfat_expand_patch).l
**L00be1e:	bne	L00be22			;<- jmp ($be1e)
**		moveq	#1<<ARCHIVE,d5
**L00be22:	and.b	d7,d5
**		swap	d7
**		tst.b	d5
**		beq	L00be3c
**L00be2a:	cmp.b	#$05,d0			;<- jmp ($be2a)

		.endif


* chdir出来ない不具合の修正 ------------------- *
* 18+3文字(ピリオドを入れると22文字)のディレクトリ
* に入れない不具合の修正.

patch302_chdir_dir:
		pea	(patch302_chdir_dir_b420,pc)
		move	#JMP_ABSL,($b420)
		move.l	(sp)+,($b420+2)
		rts

patch302_chdir_dir_b420:
		move.b	(a5)+,d0		;ループ内は同じ
		cmpi.b	#TAB,d0
		beq	@f
		move.b	d0,(a1)+
		dbra	d3,patch302_chdir_dir_b420
		cmpi.b	#TAB,(a5)+		;18+'.'+3文字の次がパスデリミタなら
		beq	@f			;正しいディレクトリ名
		jmp	($b42e)
@@:		jmp	($b432)


**L00b420:	move.b	(a5)+,d0		;jmp
**		cmp.b	#TAB,d0			; (patch302_chdir_dir_b420).l
**		beq	L00b432
**		move.b	d0,(a1)+
**		dbra	d3,L00b420
**L00b42e:	moveq	#-13,d0			;<- jmp ($b42e)
**		bra	L00b46a
**L00b432:	clr.b	(a1)			;<- jmp ($b432)


* 仮想ディレクトリ/ドライブの不具合の修正 ----- *
* mkdir foo.bar || subst z: foo がエラーになら
* ない不具合の修正.
* ".." "." エントリのないサブディレクトリに割り
* 当てられない不具合の修正(通常は有り得ない).

patch302_subst:
		pea	(patch302_subst_d73c,pc)
		move	#JMP_ABSL,($d73c)
		move.l	(sp)+,($d73c+2)
		rts

patch302_subst_d73c:
		tst.b	(a2)+			;NAMESTS_Wild
		beq	patch302_subst_d73c_skip

* サブディレクトリの指定("d:\foo\")
* バッファ確保を手抜きして header.s の不要なコード部分を
* 使用しているので再入不可能である.
		.xref	 _path_buf
		lea	(_path_buf,pc),a1	;パス名バッファ
		moveq	#0,d0
		move.b	(a2)+,d0		;NAMESTS_Drive
		jsr	($a728)			;物理ドライブ番号を論理番号に変換
		move	#'A:',(a1)+
		add.b	d0,(-2,a1)
patch302_subst_loop:
		move.b	(a2)+,d0		;パス名を内部表現から戻す
		cmpi.b	#$09,d0
		bne	@f
		moveq	#'\',d0
@@:		move.b	d0,(a1)+
		bne	patch302_subst_loop
		clr.b	(-2,a1)			;最後の'\'を取り除く

		lea	(_path_buf,pc),a1
patch302_subst_d73c_skip:
		movea.l	sp,a2
		move	#1<<DIRECTORY,-(sp)	;補完モードにしない
		jmp	($d73c+6)

**L00d73c:	movea.l	sp,a2			;jmp
**		move	#1<<DIRECTORY+1<<8,-(sp); (patch302_subst_d73c).l
**		move.l	a1,-(sp)		;<- jmp ($d73c+6)


* 実行ファイルの拡張子収得の不具合の修正 ------ *
* ".x" といったファイル名に拡張子があると見なされる
* 不具合の修正. DOS _EXEC 呼び出し時に d1.hw =! 0
* だと場合によっては "x" でも見なされる.

patch302_exec_ext:
		pea	(patch302_exec_ext_997c,pc)
		move	#JMP_ABSL,($997c)
		move.l	(sp)+,($997c+2)
		rts

patch302_exec_ext_997c:
		move.l	a1,-(sp)
		lea	(-92,sp),sp
		pea	(sp)
		pea	(a1)
		DOS	_NAMECK
		tst.l	d0
		bmi	@f

		lea	(86+8,sp),a1
		tst.b	(a1)+			;cmpi.b #'.',(a1)+
		beq	@f			;bne @f
		moveq	#$20,d0
		or.b	(a1)+,d0
		tst.b	(a1)
		beq	patch302_exec_ext_end
@@:
		moveq	#0,d0
patch302_exec_ext_end:
		lea	(92+8,sp),sp
		movea.l	(sp)+,a1
		rts

.if 0
* "d:.x" で駄目
		move.l	a1,-(sp)
@@:
		tst.b	(a1)+
		bne	@b
		subq.l	#1,a1			;ファイル名末尾(EOS)
@@:
		cmpa.l	(sp),a1
		beq	patch302_exec_ext_err
		cmpi.b	#' ',-(a1)
		beq	@b			;末尾の空白は無視

		subq.l	#1,a1			;abc.x
		cmpa.l	(sp),a1			;   ^
		bls	patch302_exec_ext_err	;拡張子なし
		cmpi.b	#'.',(a1)+
		bne	patch302_exec_ext_err	;拡張子なし/一文字ではない

		moveq	#$20,d0
		or.b	(a1)+,d0		;拡張子を小文字にして返す
patch302_exec_ext_end:
		movea.l	(sp)+,a1
		rts

patch302_exec_ext_err:
		moveq	#0,d0
		bra	patch302_exec_ext_end
.endif

**L00997c:	PUSH	d1/a1			;jmp
**		clr	d1			; (patch302_exec_ext_997c).l


* 実行ファイル名の不具合の修正 ---------------- *
* 拡張子のない実行ファイルをロードした時、PSP 内
* のバッファに格納されるファイル名の末尾に '.'
* が付く不具合の修正.
* +S による空白を含むファイル名に対応.

patch302_exec_name:
		pea	(patch302_exec_name_96c6,pc)
		move	#JMP_ABSL,($96c6)
		move.l	(sp)+,($96c6+2)
		rts

patch302_exec_name_96c6:
		lea	(2+65,a2),a1		;NAMESTS_Name1
		lea	($c4,a0),a2		;PSP_Filename
		moveq	#0,d1			;chdir/mkdir モード
		jsr	($f292)			;call namecheck_files_ent
		clr.b	(a2)			;念の為…
		jmp	($970e)

**L0096c6:	lea	(PSP_Filename,a0),a1	;jmp
**		moveq	#8-1,d1			; (patch302_exec_name_96c6).l
**		...
**L00970e:	lea	(PSP_Drive,a0),a1	;<- jmp ($970e)


* スラッシュが使えない不具合の修正 ------------ *
* DOS _EXEC(2)による実行ファイルの検索で、パスデリミタ
* に'/'が使われているとエラーになる不具合の修正.
* namecheck_errchr()で'/'が許可されていなければ不要.

patch302_pathchk_slash:
		pea	(patch302_pathchk_slash_9c8a,pc)
		move	#JMP_ABSL,($9c8a)
		move.l	(sp)+,($9c8a+2)
		rts

patch302_pathchk_slash_9c8a:
		cmpi.b	#'\',d0
		beq	1f
		cmpi.b	#'/',d0
		bne	2f
1:		move.l	a2,(sp)
2:		jmp	($9c92)


**L009c8a:	cmpi.b	#'\',d0			;jmp
**		bne	L009c92			; (patch302_pathchk_slash_9c8a).l
**		move.l	a2,(sp)
**L009c92:	addq.b	#1,d2			;<- jmp ($9c92)


* 実行ファイル検索の不具合の修正 -------------- *
* DOS _EXEC(2)による実行ファイルの検索で、拡張子が
* 大文字のファイルが正しく扱えない不具合の修正.

patch302_pathchk_ext:
		addq	#8,($9dc8+2)

		addq	#3,($9e74+2)
		pea	(exec_ext_list,pc)
		move.l	(sp)+,($9e78+2)

		move.b	#'?',($11204+1)
		rts

exec_ext_list:	.dc.b	'r',0
		.dc.b	'R',0
		.dc.b	'z',0
		.dc.b	'Z',0
		.dc.b	'x',0
		.dc.b	'X',0
		.dc.b	0
		.even

**L009dc8:	bsr	L009e56			;bsr L009e5e

**L009e74:	move.b	#3,d1			;move.b #6,d1
**L009e78:	lea	(L0111fd),a1		;lea (exec_ext_list),a1

**L011204::	.dc.b	'.*',0			;'.?',0


* DOS _NAMECK の不具合の修正 ------------------ *
* DOS _NAMECK でファイル名にスペースが含まれている場合に
* スペース以降の文字が消えてしまう不具合の修正.

patch302_nameck:
		pea	(patch302_nameck_adc8,pc)
		move	#JMP_ABSL,($adc8)
		move.l	(sp)+,($adc8+2)
		rts

patch302_nameck_adc8:
		lea	(-3,a3),a4		;name1 末尾
		tst.b	(a3)			;name2 先頭
		bne	patch302_nameck_fn2

		moveq	#8-1,d2
@@:		cmpi.b	#' ',-(a4)		;末尾のスペースを削る
		dbne	d2,@b
		beq	patch302_nameck_nul
		addq.l	#1,a4
		bra	patch302_nameck_nul

patch302_nameck_fn2:
		moveq	#10-1,d2
@@:		move.b	(a3)+,(a4)+		;name1 に name2 を繋げる
		dbeq	d2,@b
patch302_nameck_nul:
		clr.b	(a4)

		lea	(86,a2),a3
		tst.b	($f1b2)
		beq	patch302_nameck_end	;拡張子なし

		move.b	#'.',(a3)+
		moveq	#3-1,d2
@@:		rol.l	#8,d1			;拡張子を格納
		move.b	d1,(a3)+
		dbra	d2,@b

		moveq	#4-1,d2
@@:		cmpi.b	#' ',-(a3)		;末尾のスペースを削る
		dbne	d2,@b
		addq.l	#1,a3
patch302_nameck_end:
		jmp	($ae12)


**L00adc8:	lea	(NAMESTS_Name1,a2),a3	;jmp
**		moveq	#8-1,d2			; (patch302_nameck_adc8).l
**L00adce:

**		dbra	d2,L00ae04
**L00ae12:	clr.b	(a3)			;<- jmp ($ae12)


* DOS _RMDIR の不具合の修正 ------------------- *
* DOS _RMDIR で ..foo などのファイルがあるディレクトリ
* を削除できてしまう不具合の修正.

patch302_rmdir:
		pea	(patch302_rmdir_d2fc,pc)
		move	#JMP_ABSL,($d2fc)
		move.l	(sp)+,($d2fc+2)
		rts

patch302_rmdir_d2fc:
		cmpi	#'..',d0
		bne	@f
		tst.b	(30+2,a2)
		bne	@f
		jmp	($d302)
@@:		jmp	($d30e)


**L00d2fc:	cmp	#'..',d0		;jmp
**		bne	L00d30e			; (patch302_rmdir_d2fc).l
**L00d302:
**		move.l	a2,-(sp)		;<- jmp ($d302)

**L00d30e:	movea.l	(sp)+,a6		;<- jmp ($d30e)


* DOS _RENAME の不具合の修正 ------------------ *

patch302_rename:
		pea	(patch302_rename_d000,pc)
		move	#JMP_ABSL,($d000)
		move.l	(sp)+,($d000+2)
		pea	(patch302_rename_d05a,pc)
		move	#JMP_ABSL,($d05a)
		move.l	(sp)+,($d05a+2)
		rts

patch302_rename_d000:
		move.l	a1,d7
		jsr	($bcec)
		tst.l	d0
		bpl	patch302_rename_exist

		moveq	#-2,d1
		cmp.l	d0,d1
		bne	patch302_rename_return

		movea.l	a2,a1
		movea.l	sp,a2
		lea	(NAMESTS_Path,a2),a4
		lea	(NAMESTS_Path,a1),a5
		STRLEN	a4,d0
		jsr	($f27c)			;call namecheck_knj_case_cmp
		movea.l	d7,a1
		bne	@f
		jmp	($d070)
@@:
		jsr	($bcec)
		tst.l	d0
		bmi	patch302_rename_return

		moveq	#0,d5
		move	d0,d5			;オフセット
		move.l	d1,d6			;セクタ番号
		move.b	(a1),d4
		jmp	($d034)

patch302_rename_exist:
		move	d0,d5			;オフセット
		move.l	d1,d6			;セクタ番号
		movea.l	d7,a1
		movea.l	sp,a2
		jsr	($bcec)
		tst.l	d0
		bmi	patch302_rename_return

		cmp	d0,d5
		bne	@f
		cmp.l	d1,d6
@@:		jmp	($d0d0)

patch302_rename_d05a:
		movea.l	d7,a1
		jsr	($b848)
		tst.l	d0
		bmi	@f
		jmp	($d062)
@@:
		move.l	d0,-(sp)
		move.l	d6,d1
		jsr	($b7d2)			;call diskio_readfile/diskio_setflag
		move.b	d4,(a1,d5.l)
		move.l	(sp)+,d0
patch302_rename_return:
		lea	(NAMESTS_SIZE*2,sp),sp
		rts


**		bsr_	L00cbe2
**L00d000:	bsr_	L00bcec			;jmp
**		tst.l	d0			; (patch302_rename_d000).l
**		bpl	L00d0be

**L00d034:	move.b	(DIR_Atr,a1),d0		;<- jmp ($d034)

**		lea	(NAMESTS_SIZE,sp),a2
**L00d05a:	bsr_	L00b848			;jmp
**		tst.l	d0			; (patch302_rename_d05a).l
**		bmi	L00d0dc

**L00d062:	movea.l	sp,a4			;<- jmp ($d062)

**L00d070:	bsr_	L00bcec			;<- jmp ($d070)

**L00d0d0:	beq	L00d078			;<- jmp ($d0d0)


* その他の不具合の修正 ------------------------ *

patch302_others:

* VFAT ファイル名によるファイルのオープンに失敗する不具合の修正.
* VTwentyOne の内部動作に原因があるような気がするが、対策が
* 分からないので、Human68k の動作を変更することで対応する.
		subq.b	#BEQ_S-BHI_S,($bdb4)

**L00bdb0:	cmp	(DPB_DirSchFind_Offset,a0),d4
**		beq	L00bdc2			;bhi L00bdc2


* 実行ファイルのロードモード「最小ブロックから確保」が
* 無視される不具合の修正.
		clr	($99c4+2)

**L0099c4:	btst	#1,d1			;btst #0,d1


* 排他制御の互換モードが非互換な不具合の修正(LockCompatible.x).
		move.l	a0,-(sp)
		lea	($ddb8),a0
		moveq	#$f,d0
		andi	#$efff,(44+2,a0)	;読込 create
		or.l	d0,(a0)+		;互換 read
		or.l	d0,(a0)+		;     write
		or.l	d0,(a0)+		;     r/w
		or.l	d0,(a0)+		;     create
		movea.l	(sp)+,a0


* 環境変数 path がなく、実行ファイルがカレントディレクトリから
* 見つからなかった場合に path= ディレクトリの実行ファイルを
* 検索する不具合の修正。
		move	#$41f9,($9d6c)
**L009d6c:	lea	(L0111fc),a3		;lea (L0111fc),a0


* DOS _GETDPB でリモートドライブのメディアバイト
* を渡せるようにする.
		movem.l	a0-a1,-(sp)
		lea	(getdpb_pat_tbl,pc),a0
		lea	($b63c),a1
		moveq	#(getdpb_pat_tbl_end-getdpb_pat_tbl)/2-1,d0
@@:		move	(a0)+,(a1)+
		dbra	d0,@b
		movem.l	(sp)+,a0-a1

		rts


getdpb_pat_tbl:
		.dc	$1001,$d03c,$0061,$1540,$0016
		.dc	$6100,$22e2-$0a,$4a80,$6be4-$0a
getdpb_pat_tbl_end:

**L00b63c:	bsr	L00d920			;move.b d1,d0
**		tst.l	d0			;add.b #$61,d0
**		bmi	L00b628			;move.b d0,(22,a2)
**		move.b	d1,d0			;
**		add.b	#$61,d0			;bsr_ L00d920
**		move.b	d0,(22,a2)		;tst.l d0 / bmi L00b628


* 拡張パッチ ---------------------------------- *
* expatch_flag が $00 の時は呼び出さないこと.

patch302_expatch:

* DOS _EXEC(2)による実行ファイルの検索で、カレント
* ディレクトリからは探さないようにする.
		move	#NOP_CODE,($9d80)

**		bsr	L009ed4
**L009d80:	bra	L009d90			;nop

* 実行ファイルロード時の BSS クリア高速化.
		pea	(patch302_expatch_98aa,pc)
		move	#JMP_ABSL,($98aa)
		move.l	(sp)+,($98aa+2)

		rts


patch302_expatch_98aa:
		adda.l	d6,a1			;BSS 末尾
		movem.l	d1-d6/a1-a5,-(sp)
		moveq	#0,d0

		move	a1,d1
		andi	#3,d1
		bra	@f
clear_bss_align_loop:
		move.b	d0,-(a1)		;ロングワード境界に合わせる
		subq.l	#1,d6
		beq	clear_bss_end
@@:		dbra	d1,clear_bss_align_loop

		moveq	#$1f,d4
		and	d6,d4			;端数
		lsr.l	#3,d6
		moveq	#%11100,d5
		and	d6,d5
		lsr.l	#5,d6
		neg	d5
		moveq	#0,d1
		moveq	#0,d2
		moveq	#0,d3
		movea.l	d0,a2
		movea.l	d0,a3
		movea.l	d0,a4
		movea.l	d0,a5
		jmp	(clear_bss_loop_end,pc,d5.w)
clear_bss_loop:
	.rept	8
		movem.l	d0-d3/a2-a5,-(a1)	;32*8 = 256
	.endm
clear_bss_loop_end:
		dbra	d6,clear_bss_loop
		bra	@f
clear_bss_loop2:
		move.b	d0,-(a1)		;残りをクリア
@@:		dbra	d4,clear_bss_loop2
clear_bss_end:
		movem.l	(sp)+,d1-d6/a1-a5
		rts


**L0098aa:	move.l	d6,d0			;jmp
**		subq.l	#1,d0			; (patch302_expatch_98aa).l
**		swap	d0			;


		.end

* End of File --------------------------------- *
