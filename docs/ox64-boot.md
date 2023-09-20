
1. Run picocom with gkermit
1. Interrupt U-Boot into its shell
1. Run `loadb <address> <baud>` in U-Boot shell:
    * `loadb 50200000 2000000`
1. When it says “Ready for binary (kermit) download to 0x50200000 at 2000000 bps…”, hit Ctrl-A, Ctrl-S to get it to prompt for a file, then type out the file name:
    * `*** file: out/user_ox64_u.bin`
1. When done loading, it will get back to U-Boot prompt, it’s time to boot:
    * `booti 0x50200000 - 0x51ff8000`

Repeated here for more convenient copy-pasting:
```
loadb 50200000 2000000
out/user_ox64_u.bin
booti 0x50200000 - 0x51ff8000
```
