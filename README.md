
# int_calc_compiler

WebAssembly を中間言語に採用した、32 ビット整数式の x64 JIT コンパイラです。C 言語で書かれていて、Visual Studio の標準 C ライブラリと Windows API に依存しています。

## 開発環境

* Visual Studio Community 2017
https://docs.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk
* Node.js
https://nodejs.org/ja/
* 64 ビット版 Windows 10

## コンパイル対象のソースコードの例と文法

文字コードは UTF-8 である必要があり、変数名に漢字や仮名も使えます。NUL 文字は、空白に置き換えてコンパイルされます。

```
Example:
int32_t value1 = (1 + 2) * 3;
int32_t value2 = 2 + (3 * value1);
value1 = value2 + 100;

Grammer:
Program -> Statement+
Statement -> Type? variable '=' Expression ';'
Expression -> Term (('+' | '-') Term)*
Term -> Factor (('*' | '/') Factor)*
Factor -> '(' Expression ')' | ('+' | '-')? (variable | constant)
Type -> int32_t

Note:
  (1) Nesting level of expression maximum 63.
```

## bin フォルダのコマンドの実行方法

* 実行用コマンド・ファイル(int_calc.cmd)を実行すると、ソースコード(source.txt)がコンパイルされ、x64 の機械語と WebAssembly の実行結果が出力されます。

![screen shot](https://raw.githubusercontent.com/tenpoku1000/int_calc_compiler/master/images/README.PNG)

* テスト実行用コマンド・ファイル(test_int_calc.cmd)を実行すると自動テストが実行され、スナップショット格納フォルダ(test_YYYY-MM-DD_nnn)にテスト結果が出力されます。
* プログラム本体(int_calc_compiler.exe)を実行すると、コマンドライン引数の説明が表示されます。

## bin フォルダに出力されるファイルの説明

* int_calc_compiler.exe    : 実行可能ファイル(プログラム本体)
* source.txt               : コンパイル対象のソースコード(拡張子は任意で、無くても問題ありません)
* int_calc.wasm            : WebAssembly バイナリ・ファイル
* int_calc.bin             : x64 機械語イメージ・ファイル
* int_calc.cmd             : 実行用コマンド・ファイル
* int_calc.js              : 実行用 Node.js ファイル
* int_calc_log.log         : 実行時のエラーなどが格納されている log ファイル
* int_calc_token.log       : 字句解析時のトークン列を記録したファイル
* int_calc_parse_tree.log  : 構文解析で得られた構文木を記録したファイル
* int_calc_object_hash.log : 意味解析のハッシュ表に登録されている内容のファイル
* test_int_calc.cmd        : テスト実行用コマンド・ファイル
* test_YYYY-MM-DD_nnn      : テスト実行時のスナップショット格納フォルダ(nnn: 001 ～ 999)

テスト実行時のスナップショット格納フォルダには、テストケース毎に以下のファイルが出力されます。

* int_calc.wasm            : WebAssembly バイナリ・ファイル
* int_calc.bin             : x64 機械語イメージ・ファイル
* int_calc_log.log         : 実行時のエラーなどが格納されている log ファイル
* int_calc_token.log       : 字句解析時のトークン列を記録したファイル
* int_calc_parse_tree.log  : 構文解析で得られた構文木を記録したファイル
* int_calc_object_hash.log : 意味解析のハッシュ表に登録されている内容のファイル

## ビルド方法

* int_calc_compiler.sln ファイルをダブルクリックします。
* Visual Studio のセキュリティ警告を回避してプロジェクトを開きます。  
![warning](https://raw.githubusercontent.com/tenpoku1000/int_calc_compiler/master/images/MSVC.PNG)
* F7 キーを押下します。

ビルド後に tools フォルダに存在するファイルが、bin フォルダにコピーされます。

## ライセンス

[MIT license](https://raw.githubusercontent.com/tenpoku1000/int_calc_compiler/master/LICENSE)

## 作者

市川 真一 <suigun1000@gmail.com>

