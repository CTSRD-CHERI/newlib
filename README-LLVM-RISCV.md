# Newlib with Clang/LLVM and RISC-V

This repo provides a version of newlib that is suitable for compilation with Clang/LLVM on RISC-V.
Since the compiler has incomplete support for floating point and the current newlib does not provide long double libraries, only libc can be compiled.
The compiled libc also does not support I/O floating point and long double functions.
The generated libc.a should be able to provide library support for common embedded targets.

## Build

Assuming the compiler binaries are already in PATH and symlinked with proper triples.

```sh
cd newlib/libc
mkdir build
cd build
CC=riscv32-unknown-freebsd-clang CFLAGS="-march=rv32im -mabi=ilp32 -ffreestanding -mno-relax -Werror -g -I$PWD/../include --sysroot=$SPAREFS/gfe/sysroot32" LD=riscv32-unknown-freebsd-ld LDFLAGS="-mno-relax" RANLIB=llvm-ranlib AR=llvm-ar ../configure --build=riscv32-unknown-freebsd
make
```
