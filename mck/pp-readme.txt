ppmck
http://takamatsu.cool.ne.jp/dutycycle/ppmck.html
Author: h7 (h7mailmail at yahoo co jp)

■これは
Izumi.氏が開発したファミリーコンピュータ/NES用サウンドドライバmckと、
Manbow-J氏が開発したMMLコンパイラmckcに、バグフィックスや機能追加などをしたバージョンです。 
改造にあたって、某吉氏によるmck/mckcの改造版であるpmck/pmckcでの変更点を取り込んでいます。
また、2chのmckスレで公開されたパッチも取り込んでいます。さらにVRC6, MMC5, FME-7の
同時使用を可能にしています。 
なお、アーカイブにはMagicKitの一部であるnesasmを含めてあります。

mck        by Izumi.   http://www.geocities.co.jp/Playtown-Denei/9628/
mckc       by Manbow-J http://manbowj.hp.infoseek.co.jp/
pmck/pmckc by BKC      http://www.emucamp.com/boukichi/
mck/mckc patches   by [OK], 4-46, 4-356, 5-17, 5-95, 5-313, 5-658
                      (4-46, 4-356, 5-95は私です)
mmc5.h     by 5-317?  (あぷろだ491.lzhに含まれていたものを使用しました)
MagicKit assembler by J. H. Van Ornum, David Michel, Dave Shadoff, Charles Doty
                       http://www.magicengine.com/mkit/

私が書いた部分のソースについては自由に利用してください。
他の部分については各作者の指示に従ってください。

■ファイル構成
mck
│  pp-changes.txt              更新履歴
│  pp-readme.txt               このファイル
│  ppmck-ja.txt                ppmck/ppmckcでのオリジナルからの変更点の説明
│  ppmck.txt                   
│  
├─bin
│      nesasm.exe              MagicKit v2.51付属のNESASMのソースを再コンパイルしたバイナリ
│      ppmckc.exe              MMLからmckデータへのコンバータ
│      ppmckc_e.exe            English version
│      
├─doc
│  │  mck.txt                 オリジナルmckのドキュメント
│  │  mckc.txt                オリジナルmckcのドキュメント
│  │  mckc_p040719.txt        5-658氏パッチのドキュメント
│  │  pmck.txt                pmckのドキュメント
│  │  pmckc.txt               pmckcのドキュメント
│  │  
│  └─nesasm                  nesasmのドキュメント
│          CPU_INST.TXT
│          HISTORY.TXT
│          INDEX.TXT
│          NESHDR20.TXT
│          USAGE.TXT
│          
├─nes_include
│  │  ppmck.asm               ドライバのアセンブラソース
│  │  
│  └─ppmck                   ドライバのアセンブラソース
│          dpcm.h
│          fds.h
│          fme7.h
│          freqdata.h
│          internal.h
│          mmc5.h
│          n106.h
│          sounddrv.h
│          vrc6.h
│          vrc7.h
│          
├─scripts
│      mml2nsf.pl              一発コンパイル用perlスクリプト
│      
├─songs
│      00startcmd.bat          コマンドプロンプトを起動
│      mknsf.bat               一発コンパイル用バッチ
│      
└─src
    └─ppmckc                  ppmckcソース
            datamake.c
            file.c
            Makefile
            mckc.c
            mckc.h
            strings.c
            version.c

