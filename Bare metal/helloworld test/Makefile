CC=arm-none-eabi-gcc
MACH=cortex-m3
CCFLAGS= -c -mcpu=$(MACH)  -mthumb -std=gnu11 -Wall -o0
LDFLAGS= -nostdlib -T f103_ls.ld -Wl,-Map=final.map
all:main.o f103_startup.o led.o final.elf

main.o:main.c
	$(CC) $(CCFLAGS) $^ -o $@
f103_startup.o:f103_startup.c
	$(CC) $(CCFLAGS) $^ -o $@
led.o:led.c
	$(CC)	$(CCFLAGS) $^ -o $@
final.elf:main.o led.o f103_startup.o
	$(CC)	$(LDFLAGS) $^ -o $@
	arm-none-eabi-objcopy -O binary final.elf final.bin
clean:
	rm -rf *.o *.elf *.map *.bin
load:
	openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg
upload:
	st-flash write final.bin 0x08000000
