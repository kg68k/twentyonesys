# 始めに

(V)TwentyOne.sysをC言語から簡単に扱えるようにするヘッダファイルです。
options.oの代わりに使って下さい。


# 使い方

src/ディレクトリにあるtwoncall.hを、環境変数includeの指すディレクトリ
(`/usr/local/include`など)にコピーします。

C言語のソースファイル(.c)の先頭で
```
#include <twoncall.h>
```
として取り込めば後述の関数が使えるようになります。

マクロとインライン関数で定義されているので、ライブラリやオブジェクトファイルを
リンクする必要はありません。


# 定義内容

詳しい構造はtwoncall.hを参照して下さい。

## TwonFlags

オプション、バッファサイズの値を持つ構造体が`TwonFlags`としてtypedefされます。

## ビット位置

`TWON_???_BIT`という名称で各オプションのビット番号が定義されます。

## 関数

(V)TwentyOne.sysの状態を収得、設定する為のインライン関数が定義されます。  
実際には、`_di_twon*()`という`__DOSCALL`関数を宣言し、それを呼び出す`_dos_twon*()`
というインライン関数に機能番号を渡すマクロ関数となっています。


### `int GetTwentyOneID (void);`

(V)TwentyOne.sysの識別子を返します。
返値が`TWON_ID`と等しければ同ドライバが組み込まれています。

### `int GetTwentyOneVersion (void);`

(V)TwentyOne.sysのバージョンを返します。

### `int GetTwentyOneAddress (void);`

(V)TwentyOne.sys のオプション/バッファサイズを保持する変数のアドレスを返します。
返値を`TwonFlags`型へのポインタに型変換して使います。

### `int GetTwentyOneOptions (void);`

(V)TwentyOne.sysのオプション、バッファサイズを返します。返値はoptions.oと同じ形式です。

各ビットを参照/変更する方法には二通りあります。

#### `TWON_???_BIT`を使う。
```
int flag = GetTwentyOneOptions ();
if (flag & (1 << TWON_SPECIAL_BIT)) {
  // ...
}
```

`(1 << TWON_???_BIT)`と論理積を取れば任意のビットの有効・無効を調べることが出来ます。
バッファサイズは`0xffff`との論理積を取ります。

#### `TwonFlags`型に変換してメンバ名でアクセスする。

```
union {
  int val;
  TwonFlags flags;
} ret;

ret.val = GetTwentyOneOptions ();
if (ret.flags.special_option) {
  // ...
}
```

`int`型で返値を受け取って、`TwonFlags`型で参照します
(厳密には最後に書き込んだ型以外で読むのは処理系依存の動作なので好ましくないのですが……)。

`???_option`というメンバがビットフィールドで定義されているので、それを使って調べます。

なお、定数`TWON_TWENTYONE_BIT`やメンバ`twentyone_option`で示されるビットは、他のビットと
同じように+Tなら`1`に、-Tなら`0`になります。options.oとは違うので注意して下さい。

### `int SetTwentyOneOptions (int val);`

(V)TwentyOne.sysのオプションを設定します。引数はoptions.oと同じ形式です。
変更前のオプション、バッファサイズを返します。

この関数も+Tにするときビットを`1`にして下さい。

### `int GetTwentyOneSysroot (char* buf);`

(V)TwentyOne.sysが内部に保持する`SYSROOT`を`buf`で指定したバッファに収得します。
`buf`は`SYSROOT_MAX`バイト以上の領域を指していなければなりません。
収得したパス名の最後にパスデリミタは付きません。
`SYSROOT`が設定されていない場合は空文字列が書き込まれます。

### `int SetTwentyOneSysroot (char* buf);`

`buf`で指定したパスを`SYSROOT`に設定します。
パス名は`D:/`の形で始まり、パスデリミタで終わっていなければなりません。
また、最後のデリミタを取り除いた文字列の長さが`SYSROOT_MAX`バイト未満でなければなりません。

正常終了した場合は0を、エラーが発生した時は負数を返します。

### `int Unicode2Sjis (int c);`

`c`で指定したUnicode文字をShift JISコードに変換します。この関数はVTwentyOne.sys
が組み込まれていなければエラーを返します
(TwentyOne.sysは文字コード変換表が組み込まれていません)。

正常終了した場合は`0～0xffff`の値を返します。エラーが発生した場合は負数を返します。


# 著作権

著作権の扱いや配布規定は GNU GENERAL PUBLIC LICENSE Version 2
かそれ以降の任意の版に従います。

src/twoncall.h のみ、下記ライセンスとなります。

> all-permissive license
> 
> Copying and distribution of this file, with or without modification,
> are permitted in any medium without royalty provided the copyright
> notice and this notice are preserved.  This file is offered as-is,
> without any warranty.

