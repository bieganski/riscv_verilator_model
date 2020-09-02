# PREFIX	?=	riscv-none-embed-
PREFIX  ?=	riscv32-unknown-elf-
CC			:=	$(PREFIX)gcc
XD 	?=	$(CC)
# $(PREFIX)ld
OBJDUMP	:=	$(PREFIX)objdump
OBJCOPY	:=	$(PREFIX)objcopy
GDB			:=	$(PREFIX)gdb
SIZE		:=	$(PREFIX)size
VERILAT	?=	0


ARCH 	= rv32imfcxpulpv2
ABI	= elf32lriscv
# ilp32
#rv32imfcxpulpv2
# -march=rv32imc


CFLAGS	:= -march=$(ARCH) -mabi=ilp32 -Wall -Wno-unused -ffreestanding -Wall -Wno-main -O0 -g
CFLAGS += --specs=nosys.specs
						# -mstrict-align = Add this to avoid misaligned access


# mabi=ilp32
LDFLAGS	:= -g  -Wl,-Map=output/image.map -nostartfiles -lm -lc -lgcc --entry main 
# --verbose
LDFLAGS += --specs=nosys.specs
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -march=$(ARCH)
#LDFLAGS += -mabi=$(ABI)


LD_SCRIPT := -Tsections.ld


TARGET_NAME	:= $(notdir $(shell pwd))
ODIR				:=	output
TEMP				:=	output_temp
TARGET			:=	$(ODIR)/$(TARGET_NAME)



OBJ := main.c

$(shell   mkdir -p $(ODIR)_temp)
$(shell   mkdir -p $(ODIR))

.PHONY: all clean default

default: all



all: $(TARGET).asm
	@echo	"$(TARGET_NAME) builded"
	$(SIZE) -t $(TARGET).elf

$(TARGET).asm:	$(TARGET).elf
	@echo "Generating disassembly of the code..."
	$(OBJDUMP) -t -D -h $< > $@

$(TARGET).bin:	$(TARGET).elf
	@echo "Generating binary format of the elf executable..."
	$(OBJCOPY) -O binary $< $@

$(TARGET).elf:
	$(CC) -c $(CFLAGS) main.c -o main
	@echo "Linking $@"
	$(XD) $(LD_SCRIPT) -I$(INC_API) -I$(INC) $(OBJ) -o $@ $(LDFLAGS)

test:
	@echo
clean:
	@echo "Cleaning all..."
	rm -rf $(ODIR)
	rm -rf $(TEMP)
