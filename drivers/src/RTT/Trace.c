/**
 ****************************************************************************************
 *
 * @file Trace.c
 *
 * @brief Trace HardFault or Assert
 *
 ****************************************************************************************
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/* print line, must config by user */
#ifndef trace_print
#define trace_print(...)            printf(__VA_ARGS__)
#endif

/* supported function call stack max depth, default is 16 */
#ifndef CSTACK_MAX_DEPTH
#define CSTACK_MAX_DEPTH            (4)
#endif

#ifndef DUMP_STACK_INFO
#define DUMP_STACK_INFO             (1)
#endif

/*
 * DEFINES
 ****************************************************************************************
 */

#if __STDC_VERSION__ < 199901L
    #error "must be C99 or higher. try to add '-std=c99' to compile parameters"
#endif

#if defined(__CC_ARM)
    /* C stack block name, default is STACK */
    #ifndef CSTACK_BLOCK_START
    #define CSTACK_BLOCK_START      STACK$$Base
    #endif
    #ifndef CSTACK_BLOCK_END
    #define CSTACK_BLOCK_END        STACK$$Limit
    #endif
    /* code section name, default is ER_IROM1 */
    #ifndef CODE_SECTION_START
    #define CODE_SECTION_START      Image$$ER_IROM1$$Base
    #endif
    #ifndef CODE_SECTION_END
    #define CODE_SECTION_END        Image$$ER_IROM1$$Limit
    #endif
    
    #define ELF_FILE_EXT            ".axf"
    
    extern const int CSTACK_BLOCK_START;
    extern const int CSTACK_BLOCK_END;
    extern const int CODE_SECTION_START;
    extern const int CODE_SECTION_END;
    
#elif defined(__ICCARM__)
    /* C stack block name, default is 'CSTACK' */
    #ifndef CSTACK_BLOCK_NAME
    #define CSTACK_BLOCK_NAME       "CSTACK"
    #endif
    /* code section name, default is '.text' */
    #ifndef CODE_SECTION_NAME
    #define CODE_SECTION_NAME       ".text"
    #endif
    #define ELF_FILE_EXT            ".out"
    
    #pragma section=CSTACK_BLOCK_NAME
    #pragma section=CODE_SECTION_NAME
#elif defined(__GNUC__)
    /* C stack block start address, defined on linker script file, default is _sstack */
    #ifndef CSTACK_BLOCK_START
    #define CSTACK_BLOCK_START      _sstack
    #endif
    /* C stack block end address, defined on linker script file, default is _estack */
    #ifndef CSTACK_BLOCK_END
    #define CSTACK_BLOCK_END        _estack
    #endif
    /* code section start address, defined on linker script file, default is _stext */
    #ifndef CODE_SECTION_START
    #define CODE_SECTION_START      _stext
    #endif
    /* code section end address, defined on linker script file, default is _etext */
    #ifndef CODE_SECTION_END
    #define CODE_SECTION_END        _etext
    #endif
    #define ELF_FILE_EXT            ".elf"
    
    extern const int CSTACK_BLOCK_START;
    extern const int CSTACK_BLOCK_END;
    extern const int CODE_SECTION_START;
    extern const int CODE_SECTION_END;
#else
    #error "not supported compiler"
#endif

/**
 * Cortex-M fault registers
 */
typedef struct cm_fault_regs {
    unsigned int r0;                     // Register R0
    unsigned int r1;                     // Register R1
    unsigned int r2;                     // Register R2
    unsigned int r3;                     // Register R3
    unsigned int r12;                    // Register R12
    unsigned int lr;                     // Link register
    unsigned int pc;                     // Program counter
    unsigned int psr;                    // Program status                 
} fault_regs_t;

static uint32_t main_stack_base = 0;
static uint16_t main_stack_size = 0;
static uint32_t code_image_base = 0;
static uint32_t code_image_size = 0;

static bool stack_is_overflow = false;


/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

/**
 * library initialize
 */
void trace_init(void) {
#if defined(__CC_ARM) || defined(__GNUC__)
    main_stack_base = (uint32_t)(&CSTACK_BLOCK_START);
    main_stack_size = (uint32_t)(&CSTACK_BLOCK_END) - main_stack_base;
    code_image_base = (uint32_t)(&CODE_SECTION_START);
    code_image_size = (uint32_t)(&CODE_SECTION_END) - code_image_base;
#elif defined(__ICCARM__)
    main_stack_base = (uint32_t)__section_begin(CSTACK_BLOCK_NAME);
    main_stack_size = (uint32_t)__section_end(CSTACK_BLOCK_NAME) - main_stack_base;
    code_image_base = (uint32_t)__section_begin(CODE_SECTION_NAME);
    code_image_size = (uint32_t)__section_end(CODE_SECTION_NAME) - code_image_base;
#else
    #error "not supported compiler"
#endif
    stack_is_overflow = false;
    
    if (main_stack_size == 0) {
        trace_print("Trace: Unable to get STACK info\r\n");
        return;
    }

}

#ifdef DUMP_STACK_INFO
/**
 * dump current stack information
 */
static void dump_stack(uint32_t stack_start_addr, size_t stack_size, uint32_t *stack_pointer) {
    if (stack_is_overflow) {
        trace_print("Trace: stack(%08x) was overflow\r\n", (uint32_t)stack_pointer);
        
        if ((uint32_t) stack_pointer < stack_start_addr) {
            stack_pointer = (uint32_t *) stack_start_addr;
        } else if ((uint32_t) stack_pointer > stack_start_addr + stack_size) {
            stack_pointer = (uint32_t *) (stack_start_addr + stack_size);
        }
    }
    
    trace_print("[StackInfo]\r\n");
    for (; (uint32_t) stack_pointer < stack_start_addr + stack_size; stack_pointer++) {
        trace_print("  addr:%08x  data:%08x\r\n", (uint32_t)stack_pointer, *stack_pointer);
    }
}
#endif /* DUMP_STACK_INFO */

/* check the disassembly instruction is 'BL' or 'BLX' */
static bool disassembly_ins_is_bl_blx(uint32_t addr) {
    uint16_t ins1 = *((uint16_t *)addr);
    uint16_t ins2 = *((uint16_t *)(addr + 2));

#define BL_INS_MASK         0xF800
#define BL_INS_HIGH         0xF800
#define BL_INS_LOW          0xF000
#define BLX_INX_MASK        0xFF00
#define BLX_INX             0x4700

    if ((ins2 & BL_INS_MASK) == BL_INS_HIGH && (ins1 & BL_INS_MASK) == BL_INS_LOW) {
        return true;
    } else if ((ins2 & BLX_INX_MASK) == BLX_INX) {
        return true;
    } else {
        return false;
    }
}

/**
 * backtrace function call stack
 *
 * @param buffer call stack buffer
 * @param size buffer size
 * @param sp stack pointer
 *
 * @return depth
 */
static size_t trace_call_stack(uint32_t sp, const fault_regs_t *regs, uint32_t *buffer) {
    uint32_t stack_start_addr = main_stack_base, pc;
    size_t depth = 0, stack_size = main_stack_size;
    bool regs_saved_lr_is_valid = false;

    if (regs) {
        if (!stack_is_overflow) {
            /* first depth is PC */
            buffer[depth++] = regs->pc;
            /* fix the LR address in thumb mode */
            pc = regs->lr - 1;
            if ((pc >= code_image_base) && (pc <= code_image_base + code_image_size) && (depth < CSTACK_MAX_DEPTH)) {
                buffer[depth++] = pc;
                regs_saved_lr_is_valid = true;
            }
        }
    }

    if (stack_is_overflow) {
        if (sp < stack_start_addr) {
            sp = stack_start_addr;
        } else if (sp > stack_start_addr + stack_size) {
            sp = stack_start_addr + stack_size;
        }
    }

    /* copy called function address */
    for (; sp < stack_start_addr + stack_size; sp += sizeof(size_t)) {
        /* the *sp value may be LR, so need decrease a word to PC */
        pc = *((uint32_t *) sp) - sizeof(size_t);
        /* the Cortex-M using thumb instruction, so the pc must be an odd number */
        if (pc % 2 == 0) {
            continue;
        }
        /* fix the PC address in thumb mode */
        pc = *((uint32_t *) sp) - 1;
        if ((pc >= code_image_base) && (pc <= code_image_base + code_image_size) && (depth < CSTACK_MAX_DEPTH)
                /* check the the instruction before PC address is 'BL' or 'BLX' */
                && disassembly_ins_is_bl_blx(pc - sizeof(size_t))) {
            /* the second depth function may be already saved, so need ignore repeat */
            if ((depth == 2) && regs_saved_lr_is_valid && (pc == buffer[1])) {
                continue;
            }
            buffer[depth++] = pc;
        }
    }

    return depth;
}

/**
 * dump function call stack
 *
 * @param sp stack pointer
 */
static void dump_call_stack(uint32_t sp, const fault_regs_t *regs) {
    size_t i, cur_depth = 0;
    uint32_t call_stack_buf[CSTACK_MAX_DEPTH];// = {0};

    cur_depth = trace_call_stack(sp, regs, call_stack_buf);

    if (cur_depth) {
        trace_print("More ELF Info to run:\r\n ..\\..\\..\\..\\tools\\addr2line -e *%s -a -f -C -p", ELF_FILE_EXT);
        for (i = 0; i < cur_depth; i++) {
            trace_print(" %08x", call_stack_buf[i]);
        }
        trace_print("\r\n");
    } else {
        trace_print("Trace: call stack error\r\n");
    }
}

/**
 * backtrace for assert
 *
 * @param sp the stack pointer when on assert occurred
 */
void trace_assert(uint32_t sp) {
    trace_print("\r\nAssert Occured (SP:%08X)\r\n", sp);
    
    #ifdef DUMP_STACK_INFO
    dump_stack(main_stack_base, main_stack_size, (uint32_t *)sp);
    #endif /* DUMP_STACK_INFO */

    dump_call_stack(sp, NULL);
}

/**
 * @brief trace for HardFault, only call once in HardFault_Handler.
 *
 * @param fault_lr  the LR register value on fault handler
 * @param fault_sp  the stack pointer on fault handler
 */
void trace_hardfault(uint32_t fault_lr, uint32_t fault_sp) {
    uint32_t sp = fault_sp;
    const fault_regs_t* regs = (const fault_regs_t*)fault_sp;

    trace_print("\r\nHardFault Occured (LR:%08x, SP:%08x)\r\n", fault_lr, fault_sp);
    trace_print("------------------------------------------------------------\r\n");

    /* delete saved R0~R3, R12, LR,PC,xPSR registers space */
    sp += sizeof(size_t) * 8;

    #ifdef DUMP_STACK_INFO
    /* check stack overflow */
    if (sp < main_stack_base || sp > main_stack_base + main_stack_size) {
        stack_is_overflow = true;
    }
    /* dump stack information */
    dump_stack(main_stack_base, main_stack_size, (uint32_t *)sp);
    #endif /* DUMP_STACK_INFO */

    /* the stack frame may be get failed when it is overflow  */
    if (!stack_is_overflow) {
        /* dump register */
        trace_print("[Registers]\r\n");
        trace_print("  R0 : %08x  R1 : %08x  R2 : %08x  R3 : %08x\r\n", 
                        regs->r0, regs->r1, regs->r2, regs->r3);
        trace_print("  P12: %08x  LR : %08x  PC : %08x  PSR: %08x\r\n", 
                        regs->r12, regs->lr, regs->pc, regs->psr);
        trace_print("------------------------------------------------------------\r\n");
    }

    dump_call_stack(sp, regs);
    while(1); // only call once
}

#if defined(__ASM)
__ASM void HardFault_Handler(void)
{
    IMPORT  trace_hardfault
    MOV     r0, lr
    MOV     r1, sp
    BL      trace_hardfault
}
#else
void HardFault_Handler(void)
{
    uint32_t fault_lr = __return_address();
    uint32_t fault_sp = __current_sp();
    
    trace_hardfault(fault_lr, fault_sp);
}
#endif
