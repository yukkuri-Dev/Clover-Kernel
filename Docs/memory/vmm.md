# 仮想メモリ管理 (VMM)

`kernel/memmgr/vmm.c`

## 基本方針

ブート時の恒等マッピング上で動作するため、`buddy_alloc` が返す物理アドレスはそのまま仮想アドレスとして使える（物理 = 仮想）。

## API

```c
// 新しいアドレス空間を作成する
page_table_t vmm_create_address_space();

// 仮想アドレス virt を物理アドレス phys にマップする
void vmm_map_page(page_table_t pml4, uint64_t virt, uint64_t phys, uint64_t flags);

// CR3 を切り替える（アドレス空間を切り替える）
void vmm_switch(page_table_t pml4);
```

## vmm_create_address_space の動作

1. `buddy_alloc(0)` で 4KB ページを確保し PML4 として使う
2. PML4 を全てゼロクリア
3. カーネルの PML4（`0x10000`）の全 512 エントリをコピーする

エントリを全コピーすることで、新しいアドレス空間でも以下が維持される:
- **PML4[0]**: 恒等マッピング（カーネルコード・スタック・メモリ管理領域）
- **PML4[256+]**: Higher Half エイリアス

## ページフラグ

| マクロ | 値 | 意味 |
|--------|-----|------|
| `PAGE_PRESENT` | `0x01` | ページが存在する |
| `PAGE_WRITE` | `0x02` | 書き込み可能 |
| `PAGE_USER` | `0x04` | Ring3 からアクセス可能 |

## 注意

`vmm_switch` を呼んだあとは CR3 が変わるため、マップされていないアドレスへのアクセスは即座に #PF になる。カーネルタスクからは直接 `vmm_switch` を呼ばず、スケジューラに任せること。
