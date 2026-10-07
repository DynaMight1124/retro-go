/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2009-2011 Ari64
 * Shared compiler code extracted unchanged from new_dynarec.c.
 */
#define LOREG 32 // lo
#define HIREG 33 // hi
//#define FSREG 34 // FPU status (FCSR)
//#define CSREG 35 // Coprocessor status
#define CCREG 36 // Cycle count
#define INVCP 37 // Pointer to invalid_code
//#define MMREG 38 // Pointer to memory_map
#define ROREG 39 // ram offset (if psxM != 0x80000000)
#define TEMPREG 40
#define FTEMP 40 // Load/store temporary register (was fpu)
#define PTEMP 41 // Prefetch temporary register
//#define TLREG 42 // TLB mapping offset
#define RHASH 43 // Return address hash
#define RHTBL 44 // Return address hash table address
#define RTEMP 45 // JR/JALR address register
#define MAXREG 45
#define AGEN1 46 // Address generation temporary register (pass5b_preallocate2)
//#define AGEN2 47 // Address generation temporary register

  /* instruction types */
#define NOP 0     // No operation
#define LOAD 1    // Load
#define STORE 2   // Store
#define LOADLR 3  // Unaligned load
#define STORELR 4 // Unaligned store
#define MOV 5     // Move (hi/lo only)
#define ALU 6     // Arithmetic/logic
#define MULTDIV 7 // Multiply/divide
#define SHIFT 8   // Shift by register
#define SHIFTIMM 9// Shift by immediate
#define IMM16 10  // 16-bit immediate
#define RJUMP 11  // Unconditional jump to register
#define UJUMP 12  // Unconditional jump
#define CJUMP 13  // Conditional branch (BEQ/BNE/BGTZ/BLEZ)
#define SJUMP 14  // Conditional branch (regimm format)
#define COP0 15   // Coprocessor 0
#define RFE 16
#define SYSCALL 22// SYSCALL,BREAK
#define OTHER 23  // Other/unknown - do nothing
#define HLECALL 26// PCSX fake opcodes for HLE
#define COP2 27   // Coprocessor 2 move
#define C2LS 28   // Coprocessor 2 load/store
#define C2OP 29   // Coprocessor 2 operation
#define INTCALL 30// Call interpreter to handle rare corner cases
