# GDT / セグメント / TSS

`kernel/gdt/gdt.c`

## GDT エントリ一覧

GDT はバイト配列として `0x00`〜`0x37`（56 バイト）に配置される。

| オフセット | セレクタ | 説明 | アクセスバイト | Granularity |
|-----------|---------|------|-------------|-------------|
| `0x00` | — | Null | `0x00` | `0x00` |
| `0x08` | `0x08` | Kernel Code (64bit) | `0x9A` | `0xA0` |
| `0x10` | `0x10` | Kernel Data | `0x92` | `0xC0` |
| `0x18` | `0x1B`* | User Code (64bit) | `0xFA` | `0xA0` |
| `0x20` | `0x23`* | User Data | `0xF2` | `0xC0` |
| `0x28` | `0x28` | TSS（16 バイト） | `0x89` | — |

\* Ring3 で使う場合は RPL ビットを OR した値を使う（`0x18 | 3 = 0x1B`）

## Ring3 セレクタ

| 用途 | セレクタ値 |
|------|----------|
| CS（コードセグメント） | `0x1B` |
| DS / SS（データ・スタック） | `0x23` |

## TSS (Task State Segment)

`kernel/gdt/gdt.h` の `tss_t` 構造体。

| フィールド | 説明 |
|-----------|------|
| `rsp0` | Ring3 実行中に割り込みが来たときの Ring0 スタックアドレス |
| `rsp1`, `rsp2` | 未使用（0） |
| `ist[7]` | 未使用（0） |
| `iopb_offset` | `sizeof(tss_t)`（全 I/O ポートをカーネルのみ許可） |

コンテキストスイッチ時に Ring3 タスクへ切り替える場合は `tss_set_rsp0(next->kstack_top)` で RSP0 を更新する。

```c
void tss_set_rsp0(uint64_t rsp0);
```

## gdt_init の処理順序

```
1. GDT エントリ構築（バイト配列へ直接書き込み）
2. TSS を初期化して GDT[0x28] にセット
3. lgdt でロード
4. far return trick で CS を 0x08 に更新
5. DS/ES/FS/GS/SS を 0x10 に更新
6. ltr 0x28 で TSS をロード
```
