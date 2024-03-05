OUTPUT_ARCH( "riscv" )
ENTRY( _start )
SECTIONS
{
  RAM_START = DEFINED(RAM_START) ? RAM_START : 0x80000000;
  /* Default to 16MiB of RAM if not specified otherwise: */
  RAM_SIZE = DEFINED(RAM_SIZE) ? RAM_SIZE : 0x1000000;
  STACK_SIZE = DEFINED(STACK_SIZE) ? STACK_SIZE : 0x1000;
  PAGE_SIZE = 0x1000;

  /* text: kernel code section */
  . = DEFINED(ROM_START) ? ROM_START : RAM_START;
  .text : { *(.text) }

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

  . = DEFINED(ROM_START) ? RAM_START : .;
  . = ALIGN(PAGE_SIZE);
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

  /* align stack to 16 bytes as required by the ABI. It could be relaxed to 4
   * bytes on RV32E, but not worth the effort.
   */
  . = ALIGN(16);
  stack_bottom_addr = .;
  . += STACK_SIZE;
  stack_top_addr = .;

  /* End of uninitialized data segment */
  _end = .;
}
