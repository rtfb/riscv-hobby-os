
Clone freedom-e-sdk:
https://github.com/sifive/freedom-e-sdk.git

Download the toolchain and OpenOCD from https://www.sifive.com/software. It
won't work with the toolchain installed via apt (excerpt from freedom-e-sdk's
README):

```
cp openocd-<date>-<platform>.tar.gz /my/desired/location/
cp riscv64-unknown-elf-gcc-<date>-<platform>.tar.gz /my/desired/location
cd /my/desired/location
tar -xvf openocd-<date>-<platform>.tar.gz
tar -xvf riscv64-unknown-elf-gcc-<date>-<platform>.tar.gz
export RISCV_OPENOCD_PATH=/my/desired/location/openocd
export RISCV_PATH=/my/desired/location/riscv64-unknown-elf-gcc-<date>-<version>
```

(add the export statements to source'able script).

After that, the example command works fine:

`$ make BSP=metal PROGRAM=hello TARGET=sifive-hifive1-revb software`

produces output:

```
$ ls software/hello/debug/
hello.elf  hello.hex  hello.lst  hello.map
```
