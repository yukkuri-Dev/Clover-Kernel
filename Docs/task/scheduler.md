# タスク / スケジューラ

## task_t 構造体 (`kernel/task/task.h`)

| フィールド | オフセット | 説明 |
|-----------|----------|------|
| `rsp` | 0 | iretq フレームトップへのポインタ |
| `rip` | 8 | （未使用、将来用） |
| `rflags` | 16 | （未使用、将来用） |
| `rax`〜`rdx` | 24〜56 | （未使用、将来用） |
| `rsi`, `rdi` | 64〜72 | （未使用、将来用） |
| `r8`〜`r11` | 80〜112 | （未使用、将来用） |
| `r12`〜`r15` | 104〜128 | コンテキストスイッチで保存・復元される |
| `rbx` | 32 | コンテキストスイッチで保存・復元される |
| `rbp` | 136 | コンテキストスイッチで保存・復元される |
| `page_table` | 144 | PML4 物理アドレス（0 = カーネルの CR3 をそのまま使う） |
| `kstack_top` | 152 | Ring0 スタック先頭（TSS RSP0 更新用、Ring0 タスクは 0） |
| `name` | 160 | タスク名（最大 31 文字 + NUL） |
| `stack` | 192 | スタック領域の先頭ポインタ |
| `state` | 200 | 状態（1 = RUNNING/READY） |
| `next` | 208 | 未使用（将来のリンクリスト用） |

`sizeof(task_t)` = 216 バイト → `kmalloc` は 256 バイトスラブから確保する。

---

## スケジューラ API (`kernel/task/scheduler.h`)

```c
void    scheduler_init();

// カーネル (Ring0) タスクを追加
void    scheduler_add_task(const char* name, void (*entry)(), uint64_t stack_size);

// ユーザー (Ring3) タスクを追加
void    scheduler_add_task_user(const char* name, void (*entry)(), uint64_t stack_size);

void    schedule();                    // PIT ハンドラから呼ばれる
int     scheduler_get_task_count();
task_t* scheduler_get_task(int idx);
```

| 定数 | 値 |
|------|-----|
| `MAX_TASKS` | 32 |
| スケジューリング方式 | ラウンドロビン |
| タイムスライス | PIT 割り込み間隔（約 1ms） |

---

## タスクの追加

### Ring0 タスク (`scheduler_add_task`)

- スタック: `buddy_alloc(0)` で 4KB 確保
- `kstack_top = 0`（TSS 更新しない）
- iretq フレームの CS = `0x08`、SS = `0x10`

### Ring3 タスク (`scheduler_add_task_user`)

- カーネルスタック (`kstack`): `buddy_alloc(0)` で 4KB 確保 → `kstack_top` に保存
- ユーザースタック (`ustack`): `buddy_alloc(0)` で 4KB 確保
- iretq フレームの CS = `0x1B`、SS = `0x23`

---

## iretq フレーム構造

スタックに積む順（下から上 = push 順）:

```
[高アドレス]
  SS       ← Ring0: 0x10 / Ring3: 0x23
  RSP      ← スタックトップ
  RFLAGS   ← 0x202 (IF=1)
  CS       ← Ring0: 0x08 / Ring3: 0x1B
  RIP      ← エントリポイント
[低アドレス]  ← RSP はここを指す
```

---

## コンテキストスイッチ (`kernel/task/switch.asm`)

`context_switch(task_t* prev, task_t* next)` の処理:

```
1. prev のスタックに iretq フレームを積む
     SS    = 0x10
     RSP   = context_switch 呼び出し前の RSP
     RFLAGS = 現在の RFLAGS | IF
     CS    = 0x08
     RIP   = schedule() のリターンアドレス

2. callee-saved レジスタを prev に保存
     prev->rbx = rbx
     prev->r12〜r15, rbp = 各レジスタ
     prev->rsp = 現在の RSP（iretq フレームトップ）

3. callee-saved レジスタを next から復元

4. RSP = next->rsp

5. iretq で next のフレームへジャンプ
```

`schedule()` 内で `context_switch` を呼ぶ前に:
- `next->page_table != 0` なら `vmm_switch(next->page_table)` で CR3 を切り替える
- `next->kstack_top != 0` なら `tss_set_rsp0(next->kstack_top)` で TSS を更新する

---

## scheduler_init

- `kmalloc` でカーネルタスク（PID 0）を作成し `task_list[0]` へ登録
- `page_table = 0`、`kstack_top = 0`、`name = "kernel"`
- `rsp` は最初の `context_switch` 呼び出し時に書き込まれる（初期値は未使用）
