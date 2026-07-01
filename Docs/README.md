# Clover Kernel ドキュメント

バージョン: alpha 0.1.0 / アーキテクチャ: x86_64 (BIOS)

## ドキュメント一覧

```
Docs/
├── README.md              ← このファイル（インデックス）
│
├── boot/
│   └── boot.md            ブートシーケンス・ディスクレイアウト・ページテーブル初期構成
│
├── memory/
│   ├── layout.md          物理・仮想メモリレイアウト
│   ├── pmm.md             物理メモリ管理 (PMM / Buddy / Slab)
│   └── vmm.md             仮想メモリ管理 (ページング・VMM API)
│
├── interrupt/
│   ├── gdt.md             GDT・セグメントセレクタ・TSS
│   └── idt.md             IDT・PIC・例外ハンドラ・KernelPanic
│
├── task/
│   └── scheduler.md       タスク構造体・スケジューラ・コンテキストスイッチ
│
├── io/
│   ├── vga.md             VGA テキスト出力 API
│   └── keyboard.md        キーボード入力 API
│
└── shell/
    └── shell.md           シェル仕様・コマンド一覧
```

## ビルド

```sh
make        # os.img を生成
make run    # QEMU で起動
make debug  # QEMU + GDB (ポート 1234)
make clean  # ビルド成果物を削除
```

### 主要ビルドフラグ

| フラグ | 意味 |
|--------|------|
| `-ffreestanding` | 標準ライブラリなし |
| `-fno-stack-protector` | スタックカナリア無効 |
| `-mno-red-zone` | Red Zone 無効（割り込みハンドラで必須） |
| `-mcmodel=large` | 64bit アドレス空間全域参照可能 |
| `-O0` | 最適化なし |
