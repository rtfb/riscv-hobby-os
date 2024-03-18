Virtual Memory
==============

Virtual memory is implemented on the following targets:
* Qemu virt
* Qemu sifive_u 64-bit
* Pine64 Star64 board

The kernel implements RISC-V Sv39 virtual memory scheme. All memory is
identity-mapped in the kernel pagetable, meaning that from the kernel's point of
view, every virtual address is equal to its physical address.

Each user process is assigned its own virtual memory page table, which has three
distinct mapping ranges.

### Trampoline page

A single page mapped with identity mapping like all the kernel space. It's also
marked to be only Superuser-accessible. This page contains return-to-userland
code. It must be mapped and be accessible to the Superuser in order to be able
to continue fetching instructions after setting the user page table, but before
actually jumping to the user code.

### User code and data memory

This range contains the regular user program memory: the code pages and the
allocated heap pages. This range is mapped with semi-identity: virtual
address is equal to the physical address with the top 11 bits masked out.

### User stack page

The stack page is mapped differently than the rest of user memory. Its virtual
address is always the topmost page in the virtual address space (and it's the
same address for all processes), and the corresponding physical address is
whichever address gets allocated by `kalloc()` for process stack. One page below
is a guard page - an entry in the page table mapping the virtual address to
a NULL physical address, with no user-accessible permissions. This page acts as
a sentinel guarding against stack overflows.

## Unsupported targets

The following targets do support virtual memory, but it's not implemented:
* Qemu sifive_u 32-bit
* Ox64
* Nezha D1
