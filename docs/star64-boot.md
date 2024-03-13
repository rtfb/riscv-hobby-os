# Booting Pine64 Star64

### Regular boot

1. Run picocom with gkermit (`scripts/star64-term.sh` does that)
1. Interrupt U-Boot into its shell
    * i.e. hit any key when it does the countdown: `Hit any key to stop autoboot:  2`
1. Run `loadb <address> <baud>` in U-Boot shell:
    * `loadb 80200000 115200`
1. When it says `Ready for binary (kermit) download to 0x80200000 at 115200 bps...`,
   hit Ctrl-A, Ctrl-S to get it to prompt for a file, then type out the file name:
    * `*** file: out/os_star64.bin`
1. When done loading, it will get back to U-Boot prompt, it’s time to boot:
    * `booti 0x80200000 - fffc6aa0

Commands repeated here for more convenient copy-pasting:
```
loadb 80200000 115200
out/os_star64.bin
booti 0x80200000 - fffc6aa0
```
