# IDT / PIC / 例外処理

## PIC (`kernel/pic.c`)

8259A PIC（マスター・スレーブ）を初期化する。

| 設定 | 値 |
|------|-----|
| IRQ0〜7 のベクタオフセット | 32（0x20） |
| IRQ8〜15 のベクタオフセット | 40（0x28） |
| 有効な IRQ | IRQ0（PIT）、IRQ1（キーボード） |

```c
void pic_init();
void pic_send_eoi(uint8_t irq);  // EOI を送信（irq >= 8 はスレーブにも送る）
```

## IDT (`kernel/idt.c`)

全ハンドラの CS セレクタ = `0x08`（Kernel Code）。

| ベクタ | 原因 | ハンドラ | 動作 |
|--------|------|---------|------|
| 8 | #DF Double Fault | `df_handler` | `kernel_panic` |
| 13 | #GP General Protection Fault | `gp_fault_handler` | `kernel_panic` |
| 14 | #PF Page Fault | `pf_handler` | CR2 を表示して `kernel_panic` |
| 32 | IRQ0 PIT タイマー | `pit_handler` | `schedule()` を呼ぶ |
| 33 | IRQ1 キーボード | `keyboard_handler` | キーバッファへ書き込む |

```c
void idt_init();
void idt_set_entry(int vector, void* handler, uint8_t type_attr);
```

## PIT (`kernel/timer/pit.c`)

| 設定 | 値 |
|------|-----|
| モード | Mode 3（Square Wave） |
| Divisor | 1193（約 1ms 間隔） |
| 基準クロック | 1,193,182 Hz |

```c
void     pit_init();
uint64_t pit_get_ticks();   // 起動からのタイマー割り込み回数
```

PIT 割り込みハンドラ (`pit_handler`) は EOI 送信後に `schedule()` を呼んでタスクを切り替える。

## Kernel Panic (`kernel/panic.c`)

```c
void kernel_panic(const char* msg, int color);
```

- `cli` で割り込みを無効化
- `color` で VGA 画面背景を塗りつぶす
- メッセージ表示後 `hlt` ループで停止

## 例外ハンドラ ASM (`kernel/exceptions.asm`)

エラーコードを持つ例外（#GP, #PF, #DF）はスタック上のエラーコードを `pop rax` で捨ててから C ハンドラを呼ぶ。

PIT / キーボードハンドラはスクラッチレジスタ（rax, rcx, rdx, rsi, rdi, r8〜r11）を保存・復元してから `iretq` で戻る。
