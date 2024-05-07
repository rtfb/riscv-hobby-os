OUTPUT_ARCH( "riscv" )
ENTRY( _start )
SECTIONS
{
  RAM_START = DEFINED(RAM_START) ? RAM_START : 0x80000000;
  /* Default to 16MiB of RAM if not specified otherwise: */
  RAM_SIZE = DEFINED(RAM_SIZE) ? RAM_SIZE : 0x1000000;
  PAGE_SIZE = 0x1000;

  /* text: kernel code section */
  . = DEFINED(ROM_START) ? ROM_START : RAM_START;
  .text : {
      *(.initstack .trampoline .text)
      ASSERT((end_of_trampoline - _start_kernel < 0x1000), "Trampoline overflows one page!");
  }

  . = ALIGN(PAGE_SIZE);
  user_code_start = .;
  /* user_text: userland code section */
  .user_text : {
      *(.user_text)
      *(.user_rodata)
  }

  /* data: Initialized data segment */
  . = ALIGN(PAGE_SIZE);
  rodata_start = .;
  .rodata : { *(.rodata) }
  /* build-id: Store after readonly data */
  build_id = .;
  .gnu_build_id : { *(.note.gnu.build-id) }
  rodata_end = .;

  /*
   * If our code lives in ROM, reserve the bottom 512 bytes of RAM for kernel
   * stack and place all RAM sections above it. Otherwise just align data
   * section to the page boundary.
   */
  . = DEFINED(ROM_START) ? RAM_START + 512 : ALIGN(PAGE_SIZE);
  data_start = .;
  .data : { *(.data) }
  bss_start = .;
  .bss : {
    *(.sbss)
    *(.bss)
  }
  bss_end = .;
  .sdata : { *(.sdata) }
  .debug : { *(.debug) }

  heap_start = .;
}
