# CPU Emulator and Assembler

This tries to emulate a 64bit Motorola 68k look-alike CPU.
I wanted to generate machine code for my toy-language.
So I naturally invented a CPU since I thought x86 was too complicated.
It also presented an opportunity to actually generate code but from
an assembler instead of directly from the language. So instead
of finishing the language I started to write an assembler for the
emulated CPU...  Good riddance!

# TO-DO
```pre
! basic infrastructure (cpu, assembler, etc..)
! stack handling in CPU (needed for call routines)
! call/ret instructions
! global labels and variables in assembler
- local labels/variables (like asmone; .var)?    
! add call/ret routines 
- add jmp    
+ basic addressing and referencing; 
    + move d0, (a0), and friends
    + lea a0, <label>, and friends
! variable/array declarations (variable is just an array with one element)
Support for the following directives:
    - struct definitions; struct <name> ...  endstruct      ?? not sure
        - rs.<opsize>   <num>       <- reserve, alt.
    - struct instances; istruct <typename> at <member> <value> iend     <- this is the nasm way... (asmone just have macros to do this)
    - alignment options (align <num>)
    - block/buffer declarations; dcb.<opsize> <num>{,<initial value>}
    - include; include "lib/mylib.asm" or include "defs/structs.inc"
    - incbin, for binary inclusion; incbin "assets/somebinary.bin"
    - section <code/text/data/bss>  -> long form of '.code','.data', etc.. -> asmone

! syscall, through sys instruction
- Separate linker step in compiler (refactor it away from the compiler)
- compare instructions
- branching
+ disassembling (i.e. reconstructing an instruction from op-codes)     
- bit instructions (lsr, asr, rol, ror, and, or, xor, etc..)
- mul/div instructions
- assembler
    - output elf files
    - make a linker step
    - Ability to output a proper memory layout
- VCPU
    - Properly define the memory layout
    - ROM/RAM area, Int-Vector table, etc...
    - Raise exceptions
        - Within the CPU using the Int-Vector table
        - To the emulator
        
- emulator
    - accept elf files as executables
        
- consider adding 'elf' support in compiler (see: https://github.com/serge1/ELFIO
- write an OS
+ have fun...       <- don't forget...
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

# External sources
* libfmt - part of source tree.
* testrunner - https://github.com/gnilk/testrunner - for unit testing
* elfio - https://github.com/serge1/ELFIO

