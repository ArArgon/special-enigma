# special-enigma

**This project contains unfixed bugs.**

---

## Description

SiFang (四方) compiler from UESTC.

Developers: [ArArgon](https://github.com/13927), [
uestc pb](https://github.com/13927), [TrancedYou](https://github.com/TrancedYou), [henry-bugfree](https://github.com/henry-bugfree).

## Specifications

Laguage: C++17\
Dependencies: CMake, Clang (or any compiler supporting C++17), Bison, Flex

## Build

```shell
cmake ./CMakeLists.txt
```

or

```shell
make
```

## Usage

Standard mode:

```
./compiler -S <source> -o <targetASM>
```

Optimized (performance) mode:

```
./compiler -S <source> -o <targetASM> -O2
```

If you would like to see debug info, append `-debug`.