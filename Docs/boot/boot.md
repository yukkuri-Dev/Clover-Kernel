# ブートシーケンス

## ディスクレイアウト

| セクタ | 内容 |
|--------|------|
| 0 | Stage1 MBR (`boot/boot.asm`) |
| 1〜34 | Stage2 (`boot/stage2.asm`、17408 バイトにパディング) |
| 35〜 | カーネルフラットバイナリ (`kernel.bin`) |

## Stage1 (`boot/boot.asm`)

- MBR としてアドレス `0x7C00` にロードされる
- Stage2 を `0x7E00` へロードして制御を渡す

## Stage2 (`boot/stage2.asm`)

実行順序:

1. A20 ライン有効化
2. E820 でメモリマップを取得 → `0x500` に書き込む（24 バイト × 最大 32 エントリ）
3. カーネルを `0x20000` へロード（LBA 35 から KERNEL_SECTORS セクタ）
4. ページテーブルを構築（下記参照）
5. CR3 / PAE / LME / PG を設定してロングモードへ移行
6. カーネルエントリポイント `0x20000` へジャンプ

## ブート時ページテーブル

構築場所: `0x10000`〜`0x12FFF`（Stage2 が `0x1000`〜`0x2FFF` をゼロクリアしてから使用）

| 物理アドレス | 構造 |
|------------|------|
| 0x10000 | PML4 |
| 0x11000 | PDPT |
| 0x12000 | PD（512 エントリ × 2MB ラージページ） |

- **PML4[0]** → PDPT → PD : 物理 `0x0`〜`0x3FFFFFFF`（1GB）を恒等マッピング
- **PML4[256]** → 同じ PDPT : Higher Half エイリアス（`0xFFFF800000000000`〜）

カーネルはこの恒等マッピング上で動作するため、`buddy_alloc` が返す物理アドレスはそのまま仮想アドレスとして使える。

## カーネルエントリ (`kernel/kernel.asm`)

`0x20000` に配置された `_start` から `kernel_main()` へ呼び出す。

## kernel_main の初期化順序

```
gdt_init()       GDT・TSS のロード
idt_init()       IDT のロード
pic_init()       8259A PIC の初期化
vga_init()       VGA テキスト画面クリア
pmm_init()       E820 を読んで buddy へ登録
slab_init()      Slab アロケータ初期化
scheduler_init() カーネルタスク (PID 0) を登録
pit_init()       PIT タイマー設定
sti              割り込み有効化
hlt ループ       以後はタイマー割り込みでスケジューリング
```
