# CPU Emulator and Assembler

Virtual/Emulated CPU (m68k look-alike) with Assembler/Linker.

# Building
This has so far only been tested with Clang on Linux. Anyway...

CMake driven build, clone and run:
```shell
git clone https://github.com/gnilk/vcpu
cd vcpu
mkdir bld
cd bld
cmake ..
cmake --build . --target asm -j
cmake --build . --target emu -j
```
There are 3 targets
* `asm`, the assembler and linker (only outputs files similar to `a.out`)
* `emu`, the emulator (only accepts `a.out` files - no check is made)
* `test`, code compiled as shared library for unit-testing (see dependencies)

## assembler
Use like (no help is provided):
```shell
./asm -o firmware.bin file.asm
```

## emulator
The emulator is equally good documented, you basically just run:
```shell
./emu firmware.bin
```
There is however one flag you can specify `-d <addr>` which specifies the address of where the firmware should be loaded in the emulated ram.
This is to support `.org <offset>` type of statements..

# Dependencies
* libfmt - part of source tree.
* testrunner - https://github.com/gnilk/testrunner - for unit testing
* elfio - https://github.com/serge1/ELFIO - clone this into src/ext/ELFIO

# Details
* Instruction set is documented in the file `InstructionSet.cpp` and `InstructionSet.h`
* Any kind of meta-statements for the compiler is not documented
* The following sections are recognized; `.code` / `.text`, `.data`, `.bss` (not used)
* Declaration of data is like: `dc.b 0x44, 0x11` (can also handle strings)
* Operand sizes supported
  * `.b` - byte (uint8_t), example: `move.b d0, #43`
  * `.w` - word (uint16_t), example: `move.w d0, #43`
  * `.d` - dword (uint32_t), example: `move.d d0, #43`
  * `.l` - long (uint64_t), example: `move.l d0, #43`
* Only one namespace right now (no locals), labels must terminate with ':'
* Error messages are sparse (at best)
* Only a handful of instructions implemented

# Why?
This tries to emulate a 64bit Motorola 68k look-alike CPU.
I wanted to generate machine code for my toy-language.
So I naturally invented a CPU since I thought x86 was too complicated.
It also presented an opportunity to actually generate code but from
an assembler instead of directly from the language. So instead
of finishing the language I started to write an assembler for the
emulated CPU...  Good riddance!

# TO-DO
```pre
! Lexer should NOT produce EOL tokens!!!!
! basic infrastructure (cpu, assembler, etc..)
! stack handling in CPU (needed for call routines)
! call/ret instructions
! global labels and variables in assembler
! const declarations
- local labels/variables (like asmone; .var)?    
! add call/ret routines 
- add jmp    
! basic addressing and referencing; 
    ! move d0, (a0), and friends
    ! lea a0, <label>, and friends
! Advanced addressing (note; the contents within '()' is just a compile-time expression where a0 is a predefined variable...
    ! move d0, (a0 + <reg>)
    ! move d0, (a0 + <num>)
    - move d0, (a0)++       ;; There are a few bits left in the op-size byte...
! variable/array declarations (variable is just an array with one element)
! Struct definintions and declarations
    ! struct definitions; struct typename { ... }
        !  <member>  rs.<opsize>  <num>    <- reserve number of opsize elements
    ! struct instances;
        dc.struct <typename> {  dc.<opsize> <data> }    <- more work can be done here!! 
        
    - alignment options (align <num>)
        This is actually pretty easy - just insert zero's until alignment matches..
        
Support for the following directives:
    - block/buffer declarations; dcb.<opsize> <num>{,<initial value>}
    - include; include "lib/mylib.asm" or include "defs/structs.inc"
    - incbin, for binary inclusion; incbin "assets/somebinary.bin"
    - section <code/text/data/bss>  -> long form of '.code','.data', etc.. -> asmone

! syscall, through sys instruction
! disassembling (i.e. reconstructing an instruction from op-codes)     
! Separate linker step in compiler (refactor it away from the compiler)
+ refactor segments and their handling...
+ compare instructions
+ branching
+ bit instructions (lsr, asr, rol, ror, and, or, xor, etc..)
- mul/div instructions
- assembler
    ! output elf files
    ! make a linker step
    - Ability to output a proper memory layout
- VCPU
    - Properly define the memory layout
      + MMU (emulate an MMU)
    - ROM/RAM area, Int-Vector table, etc...
    - Raise exceptions
        - Within the CPU using the Int-Vector table
        - To the emulator
- Interrupt drivers
  - Consider how this should be done, directly or via an 'external' chip
  - If external - how to communicate?
           
- emulator
    ! accept elf files as executables
        
! consider adding 'elf' support in compiler (see: https://github.com/serge1/ELFIO
- write an OS
+ have fun...       <- don't forget...
```

From https://github.com/kstenerud/Musashi/blob/master/m68k_in.c
This is how Musashi handles flags - specifically interested in the FLAG_V (overflow)
```c++
// example instr. m68k_in.c
M68KMAKE_OP(add, 8, er, d)
{
	uint* r_dst = &DX;
	uint src = MASK_OUT_ABOVE_8(DY);
	uint dst = MASK_OUT_ABOVE_8(*r_dst);
	uint res = src + dst;

	FLAG_N = NFLAG_8(res);
	FLAG_V = VFLAG_ADD_8(src, dst, res);
	FLAG_X = FLAG_C = CFLAG_8(res);
	FLAG_Z = MASK_OUT_ABOVE_8(res);

	*r_dst = MASK_OUT_BELOW_8(*r_dst) | FLAG_Z;
}

// macros as follows (m68kcpu.h)
/* Flag Calculation Macros */
#define CFLAG_8(A) (A)
#define CFLAG_16(A) ((A)>>8)

#if M68K_INT_GT_32_BIT
	#define CFLAG_ADD_32(S, D, R) ((R)>>24)
	#define CFLAG_SUB_32(S, D, R) ((R)>>24)
#else
	#define CFLAG_ADD_32(S, D, R) (((S & D) | (~R & (S | D)))>>23)
	#define CFLAG_SUB_32(S, D, R) (((S & R) | (~D & (S | R)))>>23)
#endif /* M68K_INT_GT_32_BIT */

#define VFLAG_ADD_8(S, D, R) ((S^R) & (D^R))
#define VFLAG_ADD_16(S, D, R) (((S^R) & (D^R))>>8)
#define VFLAG_ADD_32(S, D, R) (((S^R) & (D^R))>>24)

#define VFLAG_SUB_8(S, D, R) ((S^D) & (R^D))
#define VFLAG_SUB_16(S, D, R) (((S^D) & (R^D))>>8)
#define VFLAG_SUB_32(S, D, R) (((S^D) & (R^D))>>24)

#define NFLAG_8(A) (A)
#define NFLAG_16(A) ((A)>>8)
#define NFLAG_32(A) ((A)>>24)
#define NFLAG_64(A) ((A)>>56)

#define ZFLAG_8(A) MASK_OUT_ABOVE_8(A)
#define ZFLAG_16(A) MASK_OUT_ABOVE_16(A)
#define ZFLAG_32(A) MASK_OUT_ABOVE_32(A)


/* Turn flag values into 1 or 0 */
#define XFLAG_AS_1() ((FLAG_X>>8)&1)
#define NFLAG_AS_1() ((FLAG_N>>7)&1)
#define VFLAG_AS_1() ((FLAG_V>>7)&1)
#define ZFLAG_AS_1() (!FLAG_Z)
#define CFLAG_AS_1() ((FLAG_C>>8)&1)


/* Conditions */
#define COND_CS() (FLAG_C&0x100)
#define COND_CC() (!COND_CS())
#define COND_VS() (FLAG_V&0x80)
#define COND_VC() (!COND_VS())
#define COND_NE() FLAG_Z
#define COND_EQ() (!COND_NE())
#define COND_MI() (FLAG_N&0x80)
#define COND_PL() (!COND_MI())
#define COND_LT() ((FLAG_N^FLAG_V)&0x80)
#define COND_GE() (!COND_LT())
#define COND_HI() (COND_CC() && COND_NE())
#define COND_LS() (COND_CS() || COND_EQ())
#define COND_GT() (COND_GE() && COND_NE())
#define COND_LE() (COND_LT() || COND_EQ())

```
wef

# Compile and build
From a build directory:
```shell
mkdir bld
cd bld
cmake ..
make -j
```
Will give you a few libraries and executables..
Nothing except the unit-tests will work right now...


