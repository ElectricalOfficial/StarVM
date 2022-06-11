#pragma once

#define OPCODE_OPSIZE 0x66

#define OPCODE_NOP 0x90

#define OPCODE_MOV_AL_DATA 0xb0
#define OPCODE_MOV_CL_DATA 0xb1
#define OPCODE_MOV_DL_DATA 0xb2
#define OPCODE_MOV_BL_DATA 0xb3
#define OPCODE_MOV_AH_DATA 0xb4
#define OPCODE_MOV_CH_DATA 0xb5
#define OPCODE_MOV_DH_DATA 0xb6
#define OPCODE_MOV_BH_DATA 0xb7

#define OPCODE_MOV_EAX_DATA 0xb8
#define OPCODE_MOV_ECX_DATA 0xb9
#define OPCODE_MOV_EDX_DATA 0xba
#define OPCODE_MOV_EBX_DATA 0xbb
#define OPCODE_MOV_ESP_DATA 0xbc
#define OPCODE_MOV_EBP_DATA 0xbd
#define OPCODE_MOV_ESI_DATA 0xbe
#define OPCODE_MOV_EDI_DATA 0xbf

#define OPCODE_INT 0xcd

#define OPCODE_JMP_REL8 0xeb

#define OPCODE_HLT 0xf4
