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

  /* user_text: userland code section */
  .user_text : {
      *(.user_text)
      *(.user_rodata)
  }
  . = ALIGN(PAGE_SIZE);

  /* data: Initialized data segment */
  .rodata : { *(.rodata) }
  /* build-id: Store after readonly data */
  build_id = .;
  .gnu_build_id : { *(.note.gnu.build-id) }

  . = DEFINED(ROM_START) ? RAM_START : .;
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
  stack_bottom = .;
  . += STACK_SIZE;
  stack_top = .;

  /* End of uninitialized data segment */
  _end = .;
}
