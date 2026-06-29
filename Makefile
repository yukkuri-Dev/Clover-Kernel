# ============================================================
#  OS Build System
#  Target: BIOS x86_64
#  Disk layout:
#    sector 0        (512B) : MBR (boot.bin のコード部分、BPBはFAT32が管理)
#    FAT32ボリューム        : STAGE2.BIN, KERNEL.BIN を格納
# ============================================================

# --- Architecture ---
# x86_64, aarch64, riscv64
# ============================
ARCH ?= x86_64

# --- Tools ---
ASM       := nasm
CC        := gcc
LD        := ld
OBJCOPY   := objcopy

# --- Flags ---
ASMFLAGS_BIN  := -f bin
ifeq ($(ARCH), x86_64)
ASMFLAGS_ELF  := -f elf64
else ifeq ($(ARCH), aarch64)
ASMFLAGS_ELF  := -f elf64
else ifeq ($(ARCH), riscv64)
ASMFLAGS_ELF  := -f elf64
endif
CFLAGS        := -ffreestanding \
                 -fno-stack-protector \
                 -fno-pic \
                 -mno-red-zone \
                 -nostdlib \
                 -nostdinc \
                 -m64 \
                 -O0 \
                 -Wall \
				 -mcmodel=large \
                 -Wextra

LDFLAGS       := -nostdlib \
                 -T linker.ld \
                 -m elf_x86_64

# --- Directories ---
BOOT_DIR   := boot
KERNEL_DIR := kernel
ARCH_DIR   := arch/$(ARCH)
BUILD_DIR  := build

# --- Sources ---
BOOT_SRC    := $(BOOT_DIR)/boot.asm
STAGE2_SRC  := $(BOOT_DIR)/stage2.asm

KERNEL_C_SRCS   := $(shell find $(KERNEL_DIR) -name "*.c")
KERNEL_ASM_SRCS := $(shell find $(KERNEL_DIR) -name "*.asm")
ARCH_C_SRCS     := $(shell find $(ARCH_DIR) -name "*.c"   2>/dev/null)
ARCH_ASM_SRCS   := $(shell find $(ARCH_DIR) -name "*.asm" 2>/dev/null)

# --- Objects ---
KERNEL_ASM_OBJS := $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/kernel/%.asm.o, $(KERNEL_ASM_SRCS))
KERNEL_C_OBJS   := $(patsubst $(KERNEL_DIR)/%.c,   $(BUILD_DIR)/kernel/%.c.o,   $(KERNEL_C_SRCS))
ARCH_ASM_OBJS   := $(patsubst $(ARCH_DIR)/%.asm,   $(BUILD_DIR)/arch/%.asm.o,   $(ARCH_ASM_SRCS))
ARCH_C_OBJS     := $(patsubst $(ARCH_DIR)/%.c,     $(BUILD_DIR)/arch/%.c.o,     $(ARCH_C_SRCS))
KERNEL_OBJS     := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS) $(ARCH_ASM_OBJS) $(ARCH_C_OBJS)

# --- Output ---
BOOT_BIN    := $(BUILD_DIR)/boot.bin
STAGE2_BIN  := $(BUILD_DIR)/stage2.bin
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
OS_IMG      := os.img

# FAT32イメージサイズ (32MB)
IMG_SIZE_MB := 32

# ============================================================
#  Targets
# ============================================================

.PHONY: all clean run debug

all: $(OS_IMG)

# --- Final disk image (FAT32) ---
# 1. 空のFAT32イメージを作成
# 2. boot.binのコード部分(0x5A〜509バイト)をMBRに書き込む
#    - FAT32 BPB(オフセット3〜89)はmkfs.fatが書いたものを保持
#    - JMP命令(先頭3バイト)とブートコード(0x5A〜)だけ上書き
# 3. stage2.binとkernel.binをFAT32上にコピー
$(OS_IMG): $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	dd if=/dev/zero of=$@ bs=1M count=$(IMG_SIZE_MB) status=none
	mkfs.fat -F 32 -n "CLOVERBOOT" $@
	@# MBRのJMP(3B) + ブートコード(オフセット90〜509) + シグネチャ(510〜511) を上書き
	dd if=$(BOOT_BIN) of=$@ bs=1 count=3 conv=notrunc status=none
	dd if=$(BOOT_BIN) of=$@ bs=1 skip=90 seek=90 count=420 conv=notrunc status=none
	dd if=$(BOOT_BIN) of=$@ bs=1 skip=510 seek=510 count=2 conv=notrunc status=none
	@# STAGE2.BINをFAT32予約領域(LBA=2)に書き込む
	@# LBA=0:MBR, LBA=1:FSInfo, LBA=6:BkBoot → LBA=2〜5が安全な空き
	dd if=$(STAGE2_BIN) of=$@ bs=512 seek=2 conv=notrunc status=none
	@# KERNEL.BINをFAT32ファイルシステム経由でコピー
	mcopy -i $@ $(KERNEL_BIN) ::KERNEL.BIN
	@echo "[IMG] $@ created (FAT32 $(IMG_SIZE_MB)MB)"

# --- Stage1 (MBR) ---
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS_BIN) $< -o $@
	@echo "[ASM] $< -> $@ ($(shell wc -c < $@) bytes)"

# --- Stage2 ---
$(STAGE2_BIN): $(STAGE2_SRC) | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS_BIN) $< -o $@
	@echo "[ASM] $< -> $@ ($(shell wc -c < $@) bytes)"

# --- Kernel ELF -> flat binary ---
$(KERNEL_BIN): $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo "[BIN] $< -> $@"

# --- Kernel ELF (link) ---
$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@
	@echo "[LD]  $@"

# --- Kernel ASM objects ---
$(BUILD_DIR)/kernel/%.asm.o: $(KERNEL_DIR)/%.asm
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_ELF) $< -o $@
	@echo "[ASM] $< -> $@"

# --- Kernel C objects ---
$(BUILD_DIR)/kernel/%.c.o: $(KERNEL_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC]  $< -> $@"

# --- Arch ASM objects ---
$(BUILD_DIR)/arch/%.asm.o: $(ARCH_DIR)/%.asm
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_ELF) $< -o $@
	@echo "[ASM] $< -> $@"

# --- Arch C objects ---
$(BUILD_DIR)/arch/%.c.o: $(ARCH_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "[CC]  $< -> $@"
# --- Build dir ---
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)


# --- Run on QEMU ---
run: $(OS_IMG)
	qemu-system-x86_64 \
		-drive format=raw,file=$(OS_IMG) \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-vga std
run-no-shutdown: $(OS_IMG)
	qemu-system-x86_64 \
		-drive format=raw,file=$(OS_IMG) \
		-m 512M \
		-vga std
# --- Debug (QEMU + GDB) ---
debug: $(OS_IMG)
	qemu-system-x86_64 \
		-drive format=raw,file=$(OS_IMG) \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-s -S &
	gdb $(BUILD_DIR)/kernel.elf \
		-ex "target remote localhost:1234" \
		-ex "set architecture i386:x86-64"

# --- Clean ---
clean:
	rm -rf $(BUILD_DIR) $(OS_IMG)
	@echo "[CLN] done"
