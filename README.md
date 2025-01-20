# CPU Emulator and Assembler

Started out as: Virtual/Emulated CPU (m68k look-alike) with Assembler/Linker.

Now: An embryo to a fully multi-core (multi-ISA) superscalar CPU with an MMU doing proper memory translation.
CPU architecture has support for:
* Dynamic instruction set extensions (up to 16 sub-instr. set)
  * Basic instruction set is M68k look-alike
  * SIMD extension (in progress) instruction set - using instr.set prefix-coding
* Separation between Decoding and Execution - through single Dispatcher -> i.e. 'hyper-threading' possible
* Can handle different number of execution engines per ISA (i.e. you may have 4 x Integer and 1 x SIMD)
* Multi staged in-order pipeline execution (SuperScalar) - this is currently only optimistic (i.e. no register allocation handling and such)
* MMU with L1/L2 caching using MESI bus protocols
* Memory mapped HW (currently a Timer)
* Interrupts
* Exceptions

Note: The CPU Emulator is a tad bit more than a regular 'byte interpreter'.

Supportive tools:
* Assembler (two-pass assembler and linker, can produce ELF binaries)
* Emulator (to load and execute ELF binaries)

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
* debugbreak - https://github.com/scottt/debugbreak/blob/master/debugbreak.h - single file header, added to src/common

# Details
* Instruction set is documented in the file `InstructionSet.cpp` and `InstructionSet.h`
* Any kind of meta-statements for the compiler is not documented
* The following sections are recognized; `.code` / `.text`, `.data`, `.bss` (not used)
* Declaration of data is like: `dc.b 0x44, 0x11` (can also handle strings)
* DecodedOperand sizes supported
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

# Stupidity or proceed?
I made the initial op-code decoding step to always consume 32 bits of data. Was pondering to make any CPU
access 32 bit aligned. However, that would waste quite a bit of space - so I decided not to.
But I will for now be keeping the 32 bit OP-code structure..  Perhaps reverting it later..

Also, changing the format to be strict about the first 32 bits and then additional information in the
coming X number of bytes is causing a bit of headache in the compiler..

For instance, relative addressing was previously solved as when ADDR mode was decoded the REL.ADDR followed.
However, this broke the strict 32 bit alignment. Hence, any relative addr. indicators will now come afterwards.
This made code-emitters in the compiler more complicated as they need to commit some extra bytes AFTER the initial
32 bits have been emitted... And since I allow this on both src/dst operands I might have trailing oper.data from
both operands... 

Therefore there now is a 'postEmitter' array - where all post-emitters are placed. They are executed in placement order...

# TO-DO
```pre
- Refactor instruction decoder (and order of details) to enable pipelineing
  I want to be able to calculate the complete instr.size from just the 32 bits (4 bytes) of op. instr. data
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
    ! section <code/text/data/bss>  -> long form of '.code','.data', etc.. -> asmone

! syscall, through sys instruction
! disassembling (i.e. reconstructing an instruction from op-codes)     
! Separate linker step in compiler (refactor it away from the compiler)
! refactor segments and their handling => in context now!
+ compare instructions
+ branching
+ bit instructions (lsr, asr, rol, ror, and, or, xor, etc..)
- mul/div instructions
- assembler
    - support for include directive
    + output elf files (works - but can be refactored to take advantage of ELF symbol handling)
    ! make a linker step (works, it's good for now)
    ! Ability to output a proper memory layout(??)
    - Perhaps output emitter statement as some kind of 'language' (text) - JSON/XML/Other?
      This can later be fed into the linker step. Which allows for 'streaming' data between these steps.
    ! add 'export' token to explicitly put publically visible labels in a separate section (should be handled by context)
    - Consider using name-mangling for private symbols..      
    - Relative jumps (BNE/BEQ don't work - something is wrong with computation of addresses)
      - It doesn't work when we jump forward since the identifier statement hasn't be processed yet and thus we can't compute the distance and we bial
      - What we need for such statements is to 'Defer' them to either the finalize of a pre-finalize state (i.e. between process and finalize)
        this would allow us to properly compute (and chose the right instr. op) for the jump.
        Either we do this via callback's or we have to change the return value from bool to an enum (which probably is better anyway)
x- linker
    ! Support for multiple compile units
    ! Support for static (within a compile-unit) and exported functions and variables
    - Dummy linker don't compute segment start-addresses properly when multiple segments and chunks
- VCPU
    ! Refactor the InstructionSet/InstructionImpl/InstructionSetV1Decoder so that it works with InstructionExtensions      
    ! Separate the instruction handler from the CPUBase, instead make that happen through composition
      Not sure how though - as it requires a much more well defined interface to avoid refactoring-hell..
    - Try to remove arithmetic flags
      RISC-V don't define them - they are deduced through other means => makes parallel execution much more efficient
    + CPU kFeature_Control kFeature_AnyRegister - interrupt mask register (1-on, 0 - off), exception mask (1-on, 0-off)
      Currently we fire interrupts even if no handlers are enabled (not good)
      All Interrupts should be disabled on reset...
    + Properly define the memory layout
      + ROM/RAM area, Int-Vector table, etc...
    + Raise exceptions
        - Within the CPU using the Int-Vector table
        - To the emulator
    - Try to remove FLAGS - they create problems when pipelining and doing out-of-order exec.
      Instead add X flag registers (8?) which can be used with special instructions (add_with_flag, sub_with_flags <- obviously other names)
    - Change branching to three operand instructions
      beq d0,d1,<label> ; read like; if d0 == d1 goto label
      bne d0,d1,<label>
      bflg d0, #<8 bit mask>, <label>   ; read like: if (d0 & mask) goto label
      bcc/bcs/bez/bnz/etc..  are all macros for bflg d0, <mask>
      
      This removes the need for "cmp" flags.
    - Branch prediction and flushing in case prediction errors
    - add 'fence' (or similar) instructions
    - add 'flush_instr_cache' / 'flush_instr_pipeline'
    - move registers to it's own 'register_file' which is more throughly defined..
    
    - Pipelining requires synchronization of register usage and ability to 'stall' in order to wait for something to execute      
      
- Interrupt drivers
  - Consider how this should be done, directly or via an 'external' chip
  - If external - how to communicate?
- Peripherals
  + Timers, proof-of-concept in place
  - GPIO, this is the basic - need for all others
  - DMA, ability to emulate 'Direct Memory Access'
  - Serial
  - I2C Bus
  - SPI Bus
  - Other?
- Piplining
  + Work on pipelining in 'SuperScalarCPU' - it currently decodes in parallel but executes in-order.

+ MMU (emulate an MMU)
  - Refactor the MMU, just define the table layout
  - Address Translation
  ! Integrate the Cache mechanism
    ! The MMU should replace the 'RamMemory' class in the Cache impl.
  - Functions to bypass (i.e. priviledge/supervisor mode -> CPU Cntrl Reg)

- Bios, based on syscall's (for the time beeing)
  - Alloc/Free, routines - that integrate with the MMU
    - Need this to start testing MMU and Address Translation

! Cache handling
  ! Integrate memory cache
  ! Work on L1/L2 caches (not sure I need L2 at this stage) 
 
        
- emulator
    ! accept elf files as executables
        
! consider adding 'elf' support in compiler (see: https://github.com/serge1/ELFIO
  Could be refactored to take advantage of the ELF symbol handling. Currently using my own linker and just wraps in an ELF binary
  
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

# Thoughts

Note to self: Skip this!

Tossing around the idea to break down the ISA to microcode and build the front/backend more like a proper microarchitecture.
From my scratch-pad...

Some abbreviations
* Execution Unit - generic for any type
* AGU/ACU - Address Generation/Computation Unit
* ALU - Arithmetic Logic Unit (Integer operation)
* FPU - Floating point unit
* SIMD - Single Inst. Multiple Data
* SME - Streaming Matrix Execution (not sure - but the idea is that it operates on larger areas registers broken down to something)

Branching is also interesting - in the first case - we need to flush the backend Dispatcher cache and reset decoding from the new IP.

Not sure how this should look like - I really want different numbers for each type..

Not sure how I should handle AGU/ACU (Address Generation/Computation Unit) - since also ALU operations  can have AGU dependencies..

Some paths look like:   `AGU -> ALU -> Write | AGU -> FPU -> Write`
While others look like: `ALU -> Write | FPU -> Write`

Things would become simpler if this is restricted on the ISA side...
with specific load/store operations.  i.e. no ALU operations allowed by the actually ALU operation...
Which we could define if we break down the ISA to micro-code first..

## Registers

The CPU registers would be translated to an internal map of many more registers allowing for reordering and out-of-order
execution. 

The backend would then have a registermap of 128 registers from which the allocation takes place
each register is 64bit - which gives a 1kb register map file..

Would be defined in two banks and allocation is done with a bitmask of 64bit each...
Assuming we can have a true 64bit core across the board...

The microcode would have to allocate a register. Each microcode block would be given a register mask in a bank.

## Microcode thoughts
There are multiple ways to achieve this. But there is a big design decision IF the AGU/ACU should be integrated with
execution units. 
* Integrated - The microcode block can be handed over as-is to the ALU
* Outside - The microcode must be divided up (either through specific code's that signal an end and handover)

If Integrated the various ALU's will blow up in size - if outside the backend must toss the code around between different
execution units.

```c++
// Basic layout of a microcode block
struct Microcode {
    uint8_t euFetureBitMask;     // What features we want from the execution unit     
    uint8_t instrPtr;     
    uint8_t code[MAX_BYTES];    // Question is if this should be 'allocated' from a generic array or a static block
};
```
Assuming each micro-code operation is 16 bit fixed. Size of the op-code poses (a minor) problem dealing with immediate values.
Will have to be split into multiple load operations Or an exception is made to allow loading of 64bit immediate values. Which means
loading a 'byte' into a register like `move.b d0, 0x42` would still be translated to `load_immediate <64bit>`

One way of doing this would be to break-down the micro-code in 
* Common part - all ISA must translate to this
* EU/ALU specific part - the ALU handover will be very small

Also - the register allocation could be hinted in the microcode cntrl block, like:
```c++
struct PreFetchControlBlock {
    uint8_t   reg_x_flags;    // flags - size, expansion, memory access, etc...
    uint8_t   reg_x_src;      // the source register
    uint8_t   reg_y_flags;    // flags?
    uint8_t   reg_y_src;     // which register to map to first allocation
};
```
This would make the prefetcher work on the control-block instead of having micro-code for this.


```asm
add d0, (a0) ; load (a0), add d0, store d0  - the ALU would have a scratch register in this case
add (a0),d0  ; load (a0), add d0, store d0  - the ALU would have a scratch register in this case
```

basically break down 'add d0, (a0)' to:
  ALU:
  ```asm
     load alu.x, (a0)       ; alu.x = ram[a0]        <- this should not happen in the ALU
     load alu.y  d0         ; alu.y = regmap[d0]
     add                    ; alu.result = x + y         <- the arithmetic operation always operates on X,y and stores in result
     store d0               ; regmap[d0] = alu.result
 ```
While this microcode is ALU centric is can be fairly easily translated from the ISA level
However, it is not deterministic => or is it?  assuming there would be a max size for this...
                                                     
```asm
 move d0, (a0+d1)    <- relative addressing requires and 'add'
```

Translates to:
```asm
 load alu.x, a0
 load alu.y, d1
 add
 store a0
```

However?
 `[read memory here]`      -> where to?

```asm
 load alu.x, (a0)        ; should this happen in the ALU (-> requires L1 access)
 zero alu.y
 or
 store d0
```
Another problem - we don't want the alu to read-memory, so memory reads should be resolved before...

## Operations requires by the ALU

* load  <alu register>,<regmapsrc> ; alu has two registers (1 bit)
* store <regmap target>            ; always store 'alu.result'
* zero <alu register>
* brk                 ; break ALU...
* add
* sub
* xor
* or
* and
* asr
* asl
* lsr
* asl
* ones    - ?

Assuming an extremely limited instr.set for the ALU we could perhaps encode the microcode like:
```text
BYTE
  0      ALT 1: 6 bit instr, 1 bit alu reg, 1 bit if memory access       <- no prefetcher
         ALT 2: 7 bit instr, 1 bit alu reg                               <- w. prefetcher
  1      7 bit registry, 1 bit carry/borrow
```


there must be some micro-code (common) for the pre-fetcher as well, i.e. to read from memory..
This must also be emitted by the ext. instr. sets

assuming:
```asm
     move    d0, (a0)
```

microcode:
```asm
     allocate        ; allocate a register
     load    (a0)    ; fetch
     store   d0      ; store and release
```

Either this happens in the BEFORE we hit the ALU - OR there are some ALU+AGU units - a block anyway
needs to 'tag' which type of execution unit is targetted.

With a prefetcher we need to execute the microcode-stream across different execution units - we might be
tricky...  Without the pre-fetcher we need some (or all) ALU's to be extended with AGU and L1 Cache access.

## Instruction dependencies (in/out of order execution)
Quite a few instruction pairs can be executed out-of-order and/or mixed, depending on how you schedule things.
```asm
  add   d0,d1
  add   d2,d3
  move   d4,(a0)
  move  (a1),d5
```

All of these can execute without dependencies. The dependencies are encoded already in the `pre-fetch-control-block`.
The control-block should take the `reg_x` and `reg_y` and mask it with the `active_mask`, like:
```c++
    // pseude code - but in principle - this is what it would like (naive first thought)
    
    // Step 1. assign micro code to the various execution units...
    // Assuming the control-block only holds a reg mask
    if (((cntrlBlock.reg_x & active_mask) == 0) && ((cntrlBlock.reg_y & active_mask) == 0)) {
        // This micro_code block can be executed perfectly fine - with no collisions
        active_mask |= cntrlBlock.reg_x;
        active_mask |= cntrlBlock.reg_y;
        eu = GetExecutionBlockFromFeatures(cntrlBlock.featureMask);
        eu.Schedule(microCodeBlock);
    }
    
    // This should be 'ok' in HW - design, Update/Finished are just signals and bit/mask operations should be ok...
    
    // Step 2. update the execution units
    for(eu : executionUnits) {
        eu.Update();
        // Finished - remove the active mask flags - so we can run again...
        if (eu.IsFinished()) {
            active_mask &= ^eu.cntrlBlock.reg_x;
            active_mask &= ^eu.cntrlBlock.reg_y;
        }
    }
    // repeat...
```

## Branching
If I use 'combined' EU's - where an execution unit is tagged with features (see: Coffee Lake and others) I can let 
the decoder add the requested features in 'ExecutionUnitFeatureMask' bit-field during decoding. The Dispatcher will 
then put that microcode block on the queue for the specific execution unit.


There is however a special combination - ALU+BRANCH - which requires some extra consideration. Since one design goal is
to get rid of the ALU flags [carry, borrow, zero, etc..]
```asm
  bne   d0,d1,<label>
```
If using a `pre-fetch-control-block` we would map; `reg_x = d0`, `reg_y = d1` the ALU microcode would simply be:
```asm
  cmp          ;; compares reg.x with reg.y and sets the ALU flags
  brk          ;; break ALU execution
  bz <label>   ;; branch on zero to label   <- this is however, not so easy - as this is not part of the ALU..
               ;; <- no break needed as the pre-fetcher will terminate on the branch   
```

Not sure how to handle the label... Is it a fixed value following the microcode?
Or do I introduce a specific 'ldimm' (load immediate) which loads a number of bytes from the microcode stream.
like:
```asm
  ldimm     1,2,4,8     ; load 1,2,4, or 8 bytes from the microcode
  bz                    ; compare the ALU output and issue a branch if needed (branch is simply 'add to IP' <- which is a special IP ALU)
```

```asm
  call <label>
```

Branching is also interesting - in the first case - we need to flush the backend Dispatcher cache and reset decoding from the new IP.


If using a `pre-fetch-control-block` we would map; `reg_x = d0`, `reg_y = d1` the ALU microcode would simply be:
```asm
  jl           ;; jump and link
               ;; <- no break needed as the pre-fetcher will terminate on the branch   
```

```asm
  call (a0)
```


## Other things
* Microcode can be cached - and the instr. decoder bypassed. 
  * Keeping a circular buffer of X microcode buffers indicated by the IP
  * This would speed up tight loops by quite a bit..
  * How big can such a cache be?
