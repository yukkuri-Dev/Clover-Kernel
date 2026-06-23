# ============================================================
#  OS Build System
#  Target: BIOS x86_64
#  Disk layout:
#    sector 0        (512B) : boot.bin   (Stage1 MBR)
#    sector 1-2      (1KB)  : stage2.bin (Stage2)
#    sector 3以降           : kernel.bin (Kernel)
# ============================================================

# --- Tools ---
ASM       := nasm
CC        := gcc
LD        := ld
OBJCOPY   := objcopy

# --- Flags ---
ASMFLAGS_BIN  := -f bin
ASMFLAGS_ELF  := -f elf64
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
BUILD_DIR  := build

# --- Sources ---
BOOT_SRC    := $(BOOT_DIR)/boot.asm
STAGE2_SRC  := $(BOOT_DIR)/stage2.asm

KERNEL_C_SRCS := $(shell find $(KERNEL_DIR) -name "*.c")
KERNEL_ASM_SRCS := $(shell find $(KERNEL_DIR) -name "*.asm")

# --- Objects ---
KERNEL_ASM_OBJS := $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/%.asm.o, $(KERNEL_ASM_SRCS))
KERNEL_C_OBJS   := $(patsubst $(KERNEL_DIR)/%.c,   $(BUILD_DIR)/%.c.o,   $(KERNEL_C_SRCS))
KERNEL_OBJS     := $(KERNEL_ASM_OBJS) $(KERNEL_C_OBJS)

# --- Output ---
BOOT_BIN    := $(BUILD_DIR)/boot.bin
STAGE2_BIN  := $(BUILD_DIR)/stage2.bin
KERNEL_BIN  := $(BUILD_DIR)/kernel.bin
OS_IMG      := os.img

# Stage2のセクタ数 (1セクタ=512B, 2セクタ=1024B)
STAGE2_SECTORS := 34
STAGE2_SIZE    := $(shell echo $$(($(STAGE2_SECTORS) * 512)))

# Kernelのセクタ数 (kernel.binが存在すれば計算、なければ1)
KERNEL_SIZE    := $(shell test -f $(KERNEL_BIN) && wc -c < $(KERNEL_BIN) || echo 512)
KERNEL_SECTORS := $(shell echo $$(( ($(KERNEL_SIZE) + 511) / 512 )))

# ============================================================
#  Targets
# ============================================================

.PHONY: all clean run debug

all: $(OS_IMG)

# --- Final disk image ---
# boot.bin(512B) + stage2.bin(パディング済み) + kernel.bin(セクタ境界にパディング)
$(OS_IMG): $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(STAGE2_BIN) $(KERNEL_BIN) > $@
	@SZ=$$(wc -c < $@); truncate -s $$(( ($$SZ + 511) / 512 * 512 )) $@
	@echo "[IMG] $@ created ($$(wc -c < $@) bytes, kernel=$(KERNEL_SECTORS) sectors)"

# --- Stage1 (MBR) ---
$(BOOT_BIN): $(BOOT_SRC) | $(BUILD_DIR)
	$(ASM) $(ASMFLAGS_BIN) $< -o $@
	@echo "[ASM] $< -> $@ ($(shell wc -c < $@) bytes)"

# --- Stage2 (kernel.binのサイズを確定してからビルド) ---
$(STAGE2_BIN): $(STAGE2_SRC) $(KERNEL_BIN) | $(BUILD_DIR)
	$(eval KSECTORS := $(shell echo $$(( ($(shell wc -c < $(KERNEL_BIN)) + 511) / 512 ))))
	$(ASM) $(ASMFLAGS_BIN) -DKERNEL_SECTORS=$(KSECTORS) $< -o $@
	truncate -s $(STAGE2_SIZE) $@
	@echo "[ASM] $< -> $@ (padded to $(STAGE2_SIZE) bytes, KERNEL_SECTORS=$(KSECTORS))"

# --- Kernel ELF -> flat binary ---
$(KERNEL_BIN): $(BUILD_DIR)/kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo "[BIN] $< -> $@"

# --- Kernel ELF (link) ---
$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJS) linker.ld | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(KERNEL_OBJS) -o $@
	@echo "[LD]  $@"

# --- Kernel ASM objects ---
$(BUILD_DIR)/%.asm.o: $(KERNEL_DIR)/%.asm
	mkdir -p $(dir $@)
	$(ASM) $(ASMFLAGS_ELF) $< -o $@
	@echo "[ASM] $< -> $@"

# --- Kernel C objects ---
$(BUILD_DIR)/%.c.o: $(KERNEL_DIR)/%.c
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
