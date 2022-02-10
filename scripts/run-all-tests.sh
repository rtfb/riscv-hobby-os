#!/bin/sh

rm -f out/test-output-u32.txt
rm -f out/test-output-u64.txt
rm -f out/smoke-test-output-u32.txt
rm -f out/smoke-test-output-u64.txt
make out/test-output-u32.txt
make out/test-output-u64.txt
make out/smoke-test-output-u32.txt
make out/smoke-test-output-u64.txt
