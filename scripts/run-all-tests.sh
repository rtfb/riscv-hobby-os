#!/bin/sh

rm -f out/test-output*.txt
rm -f out/smoke-test-output*.txt
rm -f out/daemon-test*.txt
make out/test-output-u32.txt
make out/test-output-u64.txt
make out/smoke-test-output-u32.txt
make out/smoke-test-output-u64.txt
make out/smoke-test-output-virt.txt
make out/smoke-test-output-tiny-stack.txt
make out/daemon-test-output-virt.txt
make out/daemon-test-output-u64.txt
make out/daemon-test-output-tiny-stack.txt
