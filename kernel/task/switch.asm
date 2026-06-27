[BITS 64]
global context_switch
global task_wrapper
global task_launcher
extern task_exit

; task_wrapper: rdiにエントリ関数ポインタが入っている状態で呼ばれる
task_wrapper:
    call rdi
    call task_exit
    ud2

; task_launcher: 初回起動時にcontext_switchのretで飛んでくる
; この時点でrdiにエントリ関数が入っている（context_switchが[rsi+64]からロード済み）
; スタックにはiretqフレームが積まれている → iretqで実際のタスクへ
task_launcher:
    iretq

; context_switch(task_t* current, task_t* next)
; rdi = current,  rsi = next
;
; task_t offsets:
;   0:   rsp, 32: rbx, 104: r12, 112: r13, 120: r14, 128: r15, 136: rbp
;   160: ring (uint8_t)  0=Ring0, 3=Ring3

; context_switch(prev, next)
; pit_handlerから呼ばれる時点でスタックは:
;   [rsp+0]  = callのリターンアドレス（pit_handler内）
;   [rsp+8]  = iretqフレーム: rip
;   [rsp+16] = iretqフレーム: cs
;   [rsp+24] = iretqフレーム: rflags
;   [rsp+32] = iretqフレーム: rsp (割り込まれた側)
;   [rsp+40] = iretqフレーム: ss
context_switch:
    ; prevのcallee-savedを保存
    mov [rdi + 32],  rbx
    mov [rdi + 104], r12
    mov [rdi + 112], r13
    mov [rdi + 120], r14
    mov [rdi + 128], r15
    mov [rdi + 136], rbp

    ; prevのrspを保存（callのリターンアドレスを含む現在のrsp）
    mov [rdi + 0], rsp

    ; nextのcallee-savedを復元
    mov rbx, [rsi + 32]
    mov r12, [rsi + 104]
    mov r13, [rsi + 112]
    mov r14, [rsi + 120]
    mov r15, [rsi + 128]
    mov rbp, [rsi + 136]
    mov rdi, [rsi + 64]   ; rdi = task->rdi (初回起動用エントリ関数)
    mov rsp, [rsi + 0]

    ; nextが初回起動の場合: rspは初期iretqフレームを指している → iretqで起動
    ; nextが再開の場合: rspはcontext_switch呼び出し時のretアドレス位置 → retで戻る
    ; 両方を区別するためにスタック先頭を見る必要がある
    ; → シンプルに: 常にretで戻る。初回はスタックにretアドレスの代わりにtask_launcherを置く
    ret
