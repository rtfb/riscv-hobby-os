# Booting Ox64 from UART

When developing, it's much more convenient to load the binary directly from the
host machine instead of fiddling with the SD card after every build. Here's how
to do that:

1. Run picocom with gkermit (`scripts/ox64-term.sh` does that)
1. Interrupt U-Boot into its shell
    * i.e. hit any key when it does the countdown: `Hit any key to stop autoboot:  5`
1. Run `loadb <address> <baud>` in U-Boot shell:
    * `loadb 50200000 2000000`
1. When it says `Ready for binary (kermit) download to 0x50200000 at 2000000 bps...`,
   hit Ctrl-A, Ctrl-S to get it to prompt for a file, then type out the file name:
    * `*** file: out/os_ox64.bin`
1. When done loading, it will get back to U-Boot prompt, it’s time to boot:
    * `booti 0x50200000 - 0x51ff8000`

Commands repeated here for more convenient copy-pasting:
```
loadb 50200000 2000000
out/os_ox64.bin
booti 0x50200000 - 0x51ff8000
```
