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
- labels and variables in assembler
    - consider scoping!
    - how to fix local variables (like asmone; .var)?    
- add jump routines (call, goto, etc..) and return
    this is mostly an assembler problem...
- basic addressing and referencing; 
    - move d0, (a0), and friends
    - lea a0, <label>, and friends
- compare instructions
- branching     
- bit instructions (lsr, asr, rol, ror, and, or, xor, etc..)
- mul/div instructions
- syscall handling
- write an OS
+ have fun...       <- don't forget...
```


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

