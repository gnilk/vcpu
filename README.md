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
+ labels and variables in assembler
    - consider scoping!
    - how to fix local variables (like asmone; .var)?    
+ add jump routines (call, goto, etc..) and return
    this is mostly an assembler problem...
- basic addressing and referencing; 
    - move d0, (a0), and friends
    - lea a0, <label>, and friends
- compare instructions
- branching
+ disassembling (i.e. reconstructing an instruction from op-codes)     
- bit instructions (lsr, asr, rol, ror, and, or, xor, etc..)
- mul/div instructions
- assembler
    - output elf files
    - make a linker step
- emulator
    - accept elf files as executeables
        
- syscall handling
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

