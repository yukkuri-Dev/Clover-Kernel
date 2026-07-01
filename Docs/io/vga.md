# VGA テキスト出力

`kernel/io/vga.c`

## 仕様

| 項目 | 値 |
|------|-----|
| モード | VGA テキスト 80×25 |
| バッファアドレス | `0xB8000` |
| デフォルト文字色 | `0x0F`（白文字 / 黒背景） |

## API

```c
void vga_init();                           // 画面クリアして初期化
void vga_clear();                          // 画面クリア・カーソルを (0,0) へ
void vga_color_fill(char color);           // 指定色で全画面を塗りつぶす

void vga_putchar(char c);                  // 1 文字出力（\n / \b に対応）
void vga_putchar_color(char c, char color);// 色指定付き 1 文字出力
void vga_print(const char* str);           // 文字列出力
void vga_color_print(const char* str, char color); // 色指定付き文字列出力

void vga_print_hex(uint64_t val);          // "0x" + 16 桁 hex で出力
void vga_print_dec(uint64_t val);          // 10 進数で出力

void vga_backspace();                      // 1 文字消去・カーソルを戻す
void vga_set_cursor(int x, int y);         // CRTC でハードウェアカーソルを移動
```

## 動作

- 80 列を超えると自動で次の行へ折り返す
- `\n` で行頭へ移動し、必要なら 1 行スクロール
- 25 行を超えると全行を 1 行上へコピーし、最終行をクリアする（スクロール）
- `\b` はカーソルを 1 つ戻してスペースで上書きする（行頭を超えると前の行末へ移動）
