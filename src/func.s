		.title	(V)TwentyOne.sys - func.s


* Include File -------------------------------- *

		.include	twoncall.mac
		.include	patchlevel.mac


* Global Symbol ------------------------------- *

		.xref	_flags
		.xref	_sysroot


* Text Section -------------------------------- *

		.cpu	68000

		.text
		.even

* 新設DOSコール$ffb0(_TWON)の処理.
* in	a6.l	パラメータポインタ
*	(a6).w	機能番号
*		0: ドライバ識別子の収得
*		1: バージョンの収得
*		2: フラグ/SYSROOTアドレスの収得
*		3: オプション/バッファサイズの収得
*		4: オプションの設定
*		5: SYSROOTの収得
*		6: SYSROOTの設定
*		7: UniCode から MS 漢字コードへの変換
* out	d0.l	エラーコード(機能番号がおかしければ-14)
* break	a6

_dos_twon::
		move	(a6)+,d0
		subq	#7,d0
		bhi	dos_twon_error
		add	d0,d0
		move	(@f+7*2,pc,d0.w),d0
		jmp	(@f,pc,d0.w)
@@:
		.dc	get_id-@b
		.dc	get_version-@b
		.dc	get_address-@b
		.dc	get_option-@b
		.dc	set_option-@b
		.dc	get_sysroot-@b
		.dc	set_sysroot-@b
		.dc	u2s-@b

dos_twon_error:
		moveq	#-14,d0
		rts


* DOS _TWON(0) : ドライバ識別子の収得
* out	d0.l	'TwOn'

get_id:
		move.l	#_TWON_ID,d0
		rts


* DOS _TWON(1) : バージョンの収得
* out	d0.l	バージョン
*		bit 31-16:Ｅｘｔ(T.Kawamoto)氏によるオリジナルバージョン
*		bit 15- 8:GORRY.氏による modified バージョン
*		bit  7- 0:立花えりりんによるパッチレベル
*		例)version 1.36c modified +14 patchlevel 1なら$136c_0e01

get_version:
		move.l	#N_VERSION<<16+N_MODIFIED<<8+N_PATCHLEVEL,d0
		rts


* DOS _TWON(2) : フラグ/SYSROOTアドレスの収得
* out	d0.l	バッファサイズ

get_address:
		pea	(_flags,pc)
		move.l	(sp)+,d0
		rts


* DOS _TWON(3) : オプションの収得
* out	d0.l	オプション/バッファサイズ
*		内容は flags の T ビットが逆になった値

get_option:
		move.l	(_flags,pc),d0
		bchg	#_TWON_T_BIT,d0		;+T/-T のみ値が逆
		rts


* DOS _TWON(4) : オプションの設定
* in	(a6).l	設定する値
* out	d0.l	変更前のオプション/バッファサイズ
* break	a0
* 備考:	下位ワードのバッファサイズは変更されない.

set_option:
		lea	(_flags,pc),a0
		move	#1<<(_TWON_T_BIT-16),d0
		move.l	(a0),-(sp)
		move	(a6),(a0)
		eor	d0,(sp)			;+T/-T のみ値が逆
		eor	d0,(a0)			;〃
		move.l	(sp)+,d0		;以前の値を返す
		rts


* DOS _TWON(5) : SYSROOTの収得
* in	(a6).l	バッファアドレス
* out	d0.l	エラーコード
* break	a0-a1

get_sysroot:
		lea	(_sysroot,pc),a0
		movea.l	(a6),a1
@@:		move.b	(a0)+,(a1)+
		bne	@b
		moveq	#0,d0
		rts


* DOS _TWON(6) : SYSROOTの設定
* in	(a6).l	文字列のアドレス
*		0または空文字列ならSYSROOTを空にする
* out	d0.l	エラーコード(-13ならパス名がおかしい)
* break	d1/a0-a1

_set_sysroot_direct::
set_sysroot:
		lea	(_sysroot,pc),a1
		movea.l	(a6),a0
		move.l	a0,d0
		beq	set_sysroot_null
		tst.b	(a0)
		beq	set_sysroot_null

		moveq	#$20,d0
		or.b	(a0)+,d0
		subi.b	#'a',d0
		cmpi.b	#'z'-'a',d0
		bhi	set_sysroot_error
		cmpi.b	#':',(a0)+
		bne	set_sysroot_error
		bsr	get_char
		bsr	is_slash
		bne	set_sysroot_error	;"d:/"の形でなければエラー
@@:
		move	d0,d1			;直前の文字
		bsr	get_char
		bne	@b
		subq.l	#1,a0			;NULのアドレス
		move	d1,d0
		bsr	is_slash
		bne	@f
		subq.l	#1,a0			;末尾のパスデリミタは削除
@@:
		move.l	a0,d0
		movea.l	(a6),a0
		sub.l	a0,d0
		moveq	#_SYSROOT_MAX,d1
		cmp.l	d1,d0
		bcc	set_sysroot_error	;パス名が長すぎる

		subq	#1,d0
@@:		move.b	(a0)+,(a1)+
		dbra	d0,@b
set_sysroot_null:
		clr.b	(a1)

		moveq	#0,d0
		rts
set_sysroot_error:
		moveq	#-13,d0
		rts

is_slash:
		cmpi	#'/',d0
		beq	@f
		cmpi	#'\',d0
@@:		rts


* in	a0.l	文字列のアドレス
* out	d0.l	文字コード
*		$0000_00xx なら1バイト文字
*		$0000_xxxx なら2バイト文字(bit15=1)
*	a0.l	次の文字を指す
*	ccr	tst d0の結果

get_char:
		moveq	#0,d0
		move.b	(a0)+,d0
		bpl	get_char_end
		cmpi.b	#$a0,d0
		bcs	get_char_mb
		cmpi.b	#$e0,d0
		bcs	get_char_end
get_char_mb:
		lsl	#8,d0
		move.b	(a0)+,d0
		bne	get_char_end
		lsr	#8,d0
		subq.l	#1,a0
get_char_end:
		tst	d0
		rts


* DOS _TWON(7) : UniCode から MS 漢字コードへの変換
* in	(a6).w	UniCode 文字
* out	d0.l	MS 漢字コード文字
*		-1: VTwentyOne.sys が組み込まれていない.
* break	d1-d2/a0-a2

u2s:
		.ifdef	USE_VFAT
		move	(a6),-(sp)
		clr	-(sp)
		.xref	_Uni2SJis
		bsr	_Uni2SJis
		addq.l	#4,sp
		rts
		.else
		moveq	#-1,d0
		rts
		.endif


* End of File --------------------------------- *
