Getting GDB
===========

The regular apt-gettable gdb/gdb-multiarch somewhat works, but fails to work in
some cases, at least on my Ubuntu installation. E.g. I observed gdb-multiarch
treating an `sret` instruction as no-op, simply stepping over to the next
instruction in memory.

There are at least two options to get a fully working gdb. The Makefile is aware
of both options and will pick up whichever option you use.

SiFive prebuilt toolchain
-------------------------

`make download-sifive-toolchain` target will download [SiFive version of
toolchain][sifive-toolchain]. The upside of this approach is the speed and ease.
The downside is that you will get somewhat outdated gdb, compiled with minimal
extra features.

Build your own gdb
------------------

This wasn't as intimidating as I feared it would be. [These][1] instructions
have detailed steps to build the entire toolchain, but since I only needed gdb,
I followed them loosely and built only gdb:

```
git clone https://sourceware.org/git/binutils-gdb.git
cd binutils-gdb
git checkout gdb-14.1-release
mkdir $HOME/binutils-gdb
./configure --prefix=$HOME/binutils-gdb --target=riscv
make install
```

This approach is a little more heavyweight, but you get the latest gdb, with all
the perks like the TUI, colored output, Python integration and whatnot.

[1]: https://pdos.csail.mit.edu/6.S081/2020/tools.html
[sifive-toolchain]: https://www.sifive.com/software
