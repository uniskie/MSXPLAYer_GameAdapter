#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>
#include <stdbool.h>

#define MAX_STRING   16
#define MAX_ARG      4

// コマンドバッファ構造体（Core0 が enqueue、Core1 が dequeue） // 元の構造をヘッダへ移動
typedef struct {
    char cmd[MAX_STRING];           // コマンド文字列 (例 "SMRD") // コマンド名格納
    char arg[MAX_STRING];           // 引数の生テキスト (必要に応じて保持) // 生引数の保持
    int arg_val[MAX_ARG];           // 第2,3,4,5パラメータを変換した値（不正時-1） // 16進パース結果格納
    bool valid;                     // エントリが有効かどうか // キュー有効フラグ
} Command_t;

// コマンド関数シグネチャ型 // コマンド実行関数の型
typedef int (*CmdFunc)(const Command_t*);

// コマンドテーブルエントリ // name と実行関数を紐づける
typedef struct {
    const char* name;               // コマンド名 // 文字列比較で同定
    CmdFunc func;                   // 実行関数ポインタ // コマンド処理関数
} CommandTableEntry;

// コマンドテーブルは別ファイル(commands.c)で定義される // extern 宣言
extern const CommandTableEntry cmd_table[]; // コマンドテーブル配列 // 実体は commands.c
extern const size_t cmd_table_size;         // テーブルサイズ // 実体は commands.c

#endif // COMMANDS_H