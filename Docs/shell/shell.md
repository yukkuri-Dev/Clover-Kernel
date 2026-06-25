# シェル仕様

`kernel/shell/shell.c`

## 概要

`shell_run()` がカーネルタスク（Ring0）として PID 1 で動作する。

## プロンプト

```
*
```

## コマンド一覧

| コマンド | 動作 |
|---------|------|
| `help` | コマンド一覧を表示 |
| `clear` | 画面をクリアする |
| `echo <text>` | `<text>` をそのまま表示する |
| `meminfo` | 総メモリ量・空きページ数を表示する |
| `tasks` | 実行中タスク一覧を表示する（PID / アドレス / 名前） |
| `ver` | バージョン・ライセンス情報を表示する |
| `whoami` | `kernel(PID: 0)` を表示する |
| `crash` | 2 回入力すると `int3` でカーネルパニックを発生させる |

## キー操作

| キー | 動作 |
|------|------|
| `Enter` | コマンドを確定・実行する |
| `Backspace` | 入力バッファの末尾 1 文字を削除する（プロンプトより前には戻れない） |

## tasks コマンドの出力形式

```
Task list:
PID  TASK_ADDR        NAME_ADDR        NAME
0    0xXXXXXXXXXXXX  0xXXXXXXXXXXXX  [kernel]
1    0xXXXXXXXXXXXX  0xXXXXXXXXXXXX  [blank]
2    0xXXXXXXXXXXXX  0xXXXXXXXXXXXX  [shell]
```

`TASK_ADDR` と `NAME_ADDR` の差は常に `0xA0`（160）になる（`task_t.name` のオフセット）。
