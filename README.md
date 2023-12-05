# (V)TwentyOne.sys
TwentyOne.X Ver 1.36c +14の改造版です。  
無保証につき各自の責任で使用して下さい。


## Build
コンパイラは真里子版gccを使用して下さい。
gcc2では正しく動作しないコードが出力されるようです。

PCやネット上での取り扱いを用意にするために、src/内のファイルはUTF-8で記述されています。
X68000上でビルドする際には、UTF-8からShift_JISへの変換が必要です。

### u8tosjを使用する方法

あらかじめ、[u8tosj](https://github.com/kg68k/u8tosj)をビルドしてインストールしておいてください。

トップディレクトリで`make`を実行してください。以下の処理が行われます。
1. build/ディレクトリの作成。
2. src/内の各ファイルをShift_JISに変換してbuild/へ保存。

次に、カレントディレクトリをbuild/に変更し、`make`を実行してください。
実行ファイルが作成されます。

### u8tosjを使用しない方法

ファイルを適当なツールで適宜Shift_JISに変換してから`make`を実行してください。
UTF-8のままでは正しくビルドできませんので注意してください。


## License
配布規定についてはTwentyOne.X Ver 1.36cに準じます。  
詳しくはアーカイブファイルTWON136C.LZH内のreadme.docならびに、
TW136C14.LZH内のVTwentyOne.docを参照してください。


## Author
TcbnErik / https://github.com/kg68k/twentyonesys

