CMFStateModuleLoader Rev.1 by 173210
------------------------------------
　このプログラムはPSPでCMFに含まれるstate.prxを読み込み、32MBのみのメモリでステートセーブやロードをするプログラムです。

・免責事項
　このプログラムは単なるローダーにすぎません。
　ステートセーブ時のstate.prxの挙動については一切責任を負いかねます。
　このプログラムを使用する際にはCMFの使用許諾(んなもんあったけな?)を読んでから使用してください。
　state.prxに関してはその許諾に従います。

・既知の問題
　・スリープすると次回の動作に不具合が出る。
　・セーブデータのないスロットを読み込むとフリーズする。

・導入方法
　ZIP内のms0/CheatMasterフォルダをms0:にコピーし、CMFStateModuleLoader.prxをプラグインに追加する。

・使用方法
　R+SELECTと十字キー↑・↓・←・→（全方向）/○ボタン/×ボタン/□ボタン/△ボタン/STARTボタンのどれかでセーブし、
　L+SELECTとセーブ時に押したボタンを押してでロードします。

----------------
開発者の方へ
----------------
　このプログラムはPspStatesと同じように動作するはずです。
しかし一部異なる部分があるので説明します。

・セーブデータの保存先
　セーブデータはms0:/SAVESTATES_CMF/$(UMDID)に保存されます。
　UMDIDはハイフンなしです。
　セーブデータは2分割されて保存されています。
　ファイル名は"%c_0.BIN"と"%c_1.BIN"(%cはPspStatesの****_%c.binと対応)です。

・グローバルセーブ未対応
　そのためLoadStates関数とSaveStates関数に渡される2番目の引数は無視されます。

・sceCtrlPeekBufferPositivePatched関数に未対応

--------------------------------
このプログラムは
koro氏によって作成されたCMFのソースコードと
Team Otwibacoによって作成されたPrx Common Librariesのソースコード、
ABCanG (阿部)によって作成されたCustomHOMEのソースコード、
plum氏によってリバースされたPspStatesのソースコード
を参考にしています。
koro氏とTeam Otwibacoのみなさん、ありがとうございます。

このプログラムのソースコードはhttps://github.com/173210/CMFStateModuleLoaderで公開されています。
GPLv3にてライセンスされています。詳細はLICENSE.TXTを参照してください。
