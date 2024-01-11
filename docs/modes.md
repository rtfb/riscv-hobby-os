Targets and Privilege modes
===========================

This matrix describes which targets operate in which mode. Some explanation goes
below.

| Target | Boot Mode | Operation Mode |
| ------ | --------- | -------------- |
| qemu sifive_u | M-Mode | M-Mode |
| qemu sifive_e | M-Mode | M-Mode |
| qemu sifive_u32 | M-Mode | M-Mode |
| qemu sifive_e32 | M-Mode | M-Mode |
| qemu virt | M-Mode | S-Mode |
| hifive1_revb | M-Mode | M-Mode |
| ox64_u | S-Mode | S-Mode |
| d1_u | S-Mode | S-Mode |

### Boot Mode

This column specifies which privileged mode the kernel boots in. On some targets
we have opportunity to boot in the most privileged mode, the M-Mode; on others,
the M-Mode is handled by the bootloader (U-Boot), and by the time it jumps to
the kernel, it sets the processor into the S-Mode, and thus, we boot in S-Mode.

This is important to know to avoid touching the M-Mode registers and features if
we were bootstrapped directly into the S-Mode.

### Operation Mode

This column specifies which privileged mode the kernel operates in. On some
targets, like HiFive1-RevB, there's no S-Mode, so the only option is to operate
in M-Mode. On others, we boot in S-Mode, so we can't switch back to M-Mode. On
some targets, however, we have a luxury to boot in M-Mode, but as soon as
possible we want to switch to S-Mode.

### Flags

The above matrix is reflected in these respective flags. The flags have to be
known at compile time because some of the generated code has to be different -
specifically, the names of the CSRs (`mstatus` vs `sstatus`, etc).

* `BOOT_MODE_M` - when true, the kernel boots in M-Mode on this target
* `HAS_S_MODE` - when true, the target can switch to S-Mode (or already is in
  it, depending on `BOOT_MODE_M`)
