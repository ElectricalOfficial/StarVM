
StarVM:
	gcc -o $@ -no-pie main.c vga.c `sdl2-config --libs`

boot.bin: boot.asm
	nasm -f bin $< -o $@

run: StarVM boot.bin
	./StarVM -hdd boot.bin
	
clean:
	rm -rf StarVM boot.bin

all: clean run
