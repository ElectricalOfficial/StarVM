#include <stdio.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "vga.h"
#include "opcodes.h"

#define BOOTSECTOR_ADDR 0x7c000

typedef struct psf_header {
    
    uint8_t magic[2];
    uint8_t filemode;
    uint8_t height;
    
}__attribute__((packed)) psf_header_t;

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Event event;
SDL_Texture* framebuffer_tx;
uint32_t* framebuffer;

char* font;

uint8_t* memory;
FILE* drives[0xff];

uint32_t eax, ebx, ecx, edx;
uint32_t cs, ds, es, fs, fs, ss;
uint32_t esi, edi, ebp, eip, esp;
uint32_t eflags;

void setup_sdl() {
    
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        
        printf("SDL_Init failed!\n%s\n", SDL_GetError());
        exit(1);
        
    }
    
    window = SDL_CreateWindow("StarVM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 80*8, 25*16, SDL_WINDOW_SHOWN);
    if(!window) {
        
        printf("SDL_CreateWindow failed!\n%s\n", SDL_GetError());
        SDL_Quit();
        exit(1);
        
    }
    
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(!renderer) {
        
        printf("SDL_CreateRenderer failed!\n%s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
        
    }
    
    framebuffer_tx = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 80*8, 25*16);
    if(!framebuffer_tx) {
        
        printf("SDL_CreateTexture failed!\n%s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        exit(1);
        
    }
    
    framebuffer = malloc(80 * 8 * 25 * 16 * 4);
    
}

void load_font() {
    
    FILE* fontf = fopen("font.psf", "rb");
    
    if(!fontf) {
        
        printf("Failed to open file \"font.psf\"!\n");
        exit(1);
        
    }
    
    fseek(fontf, 0, SEEK_END);
    int size = ftell(fontf);
    fseek(fontf, 0, SEEK_SET);
    
    font = malloc(size);
    
    if(fread(font, size, 1, fontf) < 1) {
        
        printf("Failed to read file \"font.psf\"!\n");
        exit(1);
        
    }
    
    fclose(fontf);
    
    psf_header_t* header = (psf_header_t*)font;
    
    if(header->magic[0] != 0x36 || header->magic[1] != 0x04 || header->height != 16) {
        
        printf("Font \"font.psf\" is unsupported!\n");
        exit(1);
        
    }
    
}

void end() {
    
    SDL_DestroyTexture(framebuffer_tx);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    for(int i = 0; i < 0xff; i++) {
        
        if(drives[i]) fclose(drives[i]);
        
    }
    
    exit(0);
    
}

void render() {
    
    SDL_RenderClear(renderer);
    
    uint8_t* ch = memory + VIDEO_MEMORY;
    uint8_t* color = memory + VIDEO_MEMORY + 1;
    
    for(int y = 0; y < 25; y++) {
        
        for(int x = 0; x < 80; x++) {
            
            for(int font_y = 0; font_y < 16; font_y++) {
                
                for(int font_x = 0; font_x < 8; font_x++) {
                    
                    if(*(font + sizeof(psf_header_t) + font_y + *ch * 16) & (0b10000000 >> font_x)) {
                        
                        framebuffer[x * 8 + font_x + (y * 16 + font_y) * 80 * 8] = vga_colors[*color & 0xf];
                        
                    } else framebuffer[x * 8 + font_x + (y * 16 + font_y) * 80 * 8] = vga_colors[(*color & 0xf0) >> 4];
                    
                }
                
            }
            
            ch += 2;
            color += 2;
            
        }
        
    }
    
    SDL_UpdateTexture(framebuffer_tx, NULL, framebuffer, 80 * 8 * 4);
    SDL_RenderCopy(renderer, framebuffer_tx, NULL , NULL);
    SDL_RenderPresent(renderer);
    
}

int cur_x = 0;
int cur_y = 0;

void int_10h() {
    
    switch((eax & 0x0000ff00) >> 8) {
        
        case 0xe:
            memory[VIDEO_MEMORY + (cur_x + cur_y * 80) * 2] = eax & 0x000000ff;
            memory[VIDEO_MEMORY + (cur_x + cur_y * 80) * 2 + 1] = 0x07;
            cur_x++;
            if(cur_y >= 80) {
                
                cur_x = 0;
                cur_y++;
                
            }
            break;
        
    }
    
}

int main(int argc, char* argv[]) {
    
    memory = malloc(1024*1024);
    memset(memory, 0, 1024*1024);
    
    int hard_disks = 0;
    int floppy_disks = 0;
    
    int boot_device = -1;
    
    for(int i = 1; i < argc; i++) {
        
        if(strcmp(argv[i], "-hdd") == 0) {
            
            i++;
            if(i >= argc) break;
            
            drives[0x80 + hard_disks] = fopen(argv[i], "rb+");
            if(!drives[0x80 + hard_disks]) {
                
                printf("Failed to open %s!\n", argv[i]);
                end();
                
            }
            
            hard_disks++;
            
            if(boot_device < 0) {
                
                if(fread(memory + BOOTSECTOR_ADDR, 512, 1, drives[0x80 + hard_disks - 1]) < 1) continue;
                if(*(uint16_t*)&memory[BOOTSECTOR_ADDR + 510] == 0xaa55) boot_device = 0x80 + hard_disks - 1;
                
            }
            
        }
        
    }
    
    if(boot_device < 0) {
        
        printf("There is no bootable device!\n");
        exit(0);
        
    }
    
    load_font();
    setup_sdl();
    
    eip = BOOTSECTOR_ADDR;
    
    uint8_t size = 0;
    
    int hlt = 0;
    
    while(1) {
        
        while(SDL_PollEvent(&event)) {
            
            if(event.type == SDL_QUIT) {
                
                end();
                
            }
            
        }
        
        if(!hlt) switch(memory[eip]) {
            
            case OPCODE_NOP:
                printf("NOP\n");
                eip++;
                break;
            
            case OPCODE_OPSIZE:
                size = 1;
                eip++;
                break;
            
            case OPCODE_MOV_AL_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                eax &= 0xffffff00;
                eax |= val;
                printf("MOV AL, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_CL_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                ecx &= 0xffffff00;
                ecx |= val;
                printf("MOV CL, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_DL_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                edx &= 0xffffff00;
                edx |= val;
                printf("MOV DL, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_BL_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                ebx &= 0xffffff00;
                ebx |= val;
                printf("MOV BL, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_AH_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                eax &= 0xffff00ff;
                eax |= val << 8;
                printf("MOV AH, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_CH_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                ecx &= 0xffff00ff;
                ecx |= val << 8;
                printf("MOV CH, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_DH_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                edx &= 0xffff00ff;
                edx |= val << 8;
                printf("MOV DH, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_BH_DATA: {
                uint8_t val = *(uint8_t*)(memory + eip + 1);
                ebx &= 0xffff00ff;
                ebx |= val << 8;
                printf("MOV BH, %d\n", val);
                eip += 2;
                break;
            }
            
            case OPCODE_MOV_EAX_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    eax &= 0xffff0000;
                    eax |= val;
                    printf("MOV AX, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    eax = *(uint32_t*)(memory + eip + 1);
                    printf("MOV EAX, %d\n", eax);
                    eip += 5;
                    size = 0;
                    
                }
                break;
            
            case OPCODE_MOV_ECX_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    ecx &= 0xffff0000;
                    ecx |= val;
                    printf("MOV CX, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    ecx = *(uint32_t*)(memory + eip + 1);
                    printf("MOV ECX, %d\n", ecx);
                    eip += 5;
                    size = 0;
                    
                }
                break;
                
            case OPCODE_MOV_EDX_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    edx &= 0xffff0000;
                    edx |= val;
                    printf("MOV DX, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    edx = *(uint32_t*)(memory + eip + 1);
                    printf("MOV EDX, %d\n", edx);
                    eip += 5;
                    size = 0;
                    
                }
                break;
                
            case OPCODE_MOV_EBX_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    ebx &= 0xffff0000;
                    ebx |= val;
                    printf("MOV BX, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    ebx = *(uint32_t*)(memory + eip + 1);
                    printf("MOV EBX, %d\n", ebx);
                    eip += 5;
                    size = 0;
                    
                }
                break;
                
            case OPCODE_MOV_ESP_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    esp &= 0xffff0000;
                    esp |= val;
                    printf("MOV SP, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    esp = *(uint32_t*)(memory + eip + 1);
                    printf("MOV ESP, %d\n", esp);
                    eip += 5;
                    size = 0;
                    
                }
                break;
                
            case OPCODE_MOV_EBP_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    ebp &= 0xffff0000;
                    ebp |= val;
                    printf("MOV BP, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    ebp = *(uint32_t*)(memory + eip + 1);
                    printf("MOV EBP, %d\n", ebp);
                    eip += 5;
                    size = 0;
                    
                }
                break;
                
            case OPCODE_MOV_ESI_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    esi &= 0xffff0000;
                    esi |= val;
                    printf("MOV SI, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    esi = *(uint32_t*)(memory + eip + 1);
                    printf("MOV ESI, %d\n", esi);
                    eip += 5;
                    size = 0;
                    
                }
                break;
                
            case OPCODE_MOV_EDI_DATA:
                if(!size) {
                    
                    uint16_t val = *(uint16_t*)(memory + eip + 1);
                    edi &= 0xffff0000;
                    edi |= val;
                    printf("MOV DI, %d\n", val);
                    eip += 3;
                    
                } else {
                    
                    edi = *(uint32_t*)(memory + eip + 1);
                    printf("MOV EDI, %d\n", edi);
                    eip += 5;
                    size = 0;
                    
                }
                break;
            
            case OPCODE_INT: {
                uint8_t interrupt = memory[eip + 1];
                
                switch(interrupt) {
                    
                    case 0x10:
                        int_10h();
                        break;
                    
                }
                
                printf("INT %XH\n", interrupt);
                
                eip += 2;
                break;
            }
            
            case OPCODE_JMP_REL8:
                if((int8_t)(memory[eip+1]) + 2) printf("JMP %d\n", (int8_t)(memory[eip+1]) + 2);
                else printf("JMP $\n");
                eip += (int8_t)(memory[eip+1]) + 2;
                break;
            
            case OPCODE_HLT:
                hlt = 1;
                printf("HLT\n");
                break;
            
            default:
                printf("Unsupported/Invalid opcode!\n");
                end();
            
        }
        
        render();
        
    }
    
    end();
    
}
