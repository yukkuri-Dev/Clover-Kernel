# 物理メモリ管理

## PMM (`kernel/memmgr/pmm.c`)

E820 マップ（`0x500`）を読んで使用可能領域を列挙し、Buddy アロケータへ登録する。

- `base >= 0x100000` の領域のみ登録
- 1 領域あたり最大 3MB に制限（バッファオーバーフロー防止）

```c
void     pmm_init();
uint64_t pmm_get_total_memory();
```

---

## Buddy アロケータ (`kernel/memmgr/buddy.c`)

2 の冪乗サイズのブロックを管理するアロケータ。

| パラメータ | 値 |
|-----------|-----|
| ページサイズ | 4096 バイト |
| 最大オーダー | 9（2^9 × 4KB = 2MB） |
| 最大管理ページ数 | 131072（512MB 分） |

### API

```c
void  buddy_init(uint64_t base, uint64_t length);
void* buddy_alloc(int order);  // order 範囲外は NULL を返す
void  buddy_free(void* ptr, int order);
```

- `buddy_alloc(0)` → 4KB ページを 1 枚確保
- 空き領域が要求 order になければ上位 order を分割して返す
- `buddy_free` はバディ同士をマージして上位 order に戻す

---

## Slab アロケータ (`kernel/memmgr/slab.c`)

小さなオブジェクトを効率的に確保するアロケータ。Buddy の上に乗っている。

### サイズクラス

8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096 バイト

### 構造

各サイズクラスごとに 4KB ページを確保し、ページ先頭に `slab_header_t` を置く。残りの領域をオブジェクトの配列として使い、空きオブジェクトはフリーリストでつなぐ。

### API

```c
void  slab_init();
void* kmalloc(uint64_t size);  // 4KB 超は非対応（NULL を返す）
void  kfree(void* ptr);
```

> **注意:** `kmalloc` はゼロクリアしない。確保直後のメモリはゴミ値を含む。
