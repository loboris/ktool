#!/bin/bash

TOOLCHAIN_PATH="${PWD}/../kendryte-toolchain/bin"

echo
echo " ======================================="
echo " === Building 2ns stage ISP for K210 ==="
echo " ======================================="

make clean > /dev/null 2>&1

rm -rf CMakeFiles/* > /dev/null 2>&1
rm -rf lib/* > /dev/null 2>&1
rm -f *.cmake > /dev/null 2>&1
rm -f *.txt > /dev/null 2>&1

echo
echo "=== Running 'cmake'"
cmake .. -DPROJ=isp -DTOOLCHAIN=${TOOLCHAIN_PATH} > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "cmake ERROR"
    exit 1
fi

echo "=== Running 'make'"
make -j8
if [ $? -ne 0 ]; then
    echo "make ERROR"
    exit 2
fi

if [ $? -eq 0 ]; then
    ${TOOLCHAIN_PATH}/riscv64-unknown-elf-size isp
    cp -f isp isp.tmp
    ${TOOLCHAIN_PATH}/riscv64-unknown-elf-objdump -S  --disassemble isp > isp.dump
    mv -f isp.tmp isp.elf
    ./isp_bintovar.py
fi

# === CLEAN ===
make clean > /dev/null 2>&1

rm -rf __pycache__/* > /dev/null 2>&1
rm -rf CMakeFiles/* > /dev/null 2>&1
rm -rf lib/* > /dev/null 2>&1
rmdir __pycache__ > /dev/null 2>&1
rmdir CMakeFiles > /dev/null 2>&1
rmdir lib > /dev/null 2>&1

rm -f *.cmake > /dev/null 2>&1
rm -f *.txt > /dev/null 2>&1
rm -f *.json > /dev/null 2>&1
rm -f *.bak > /dev/null 2>&1
rm -f Makefile > /dev/null 2>&1
