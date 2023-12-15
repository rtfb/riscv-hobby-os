# Booting Nezha D1

### First-time boot, to enable bootdelay

1. Connect UART
1. Boot the preinstalled Tina Linux from internal NAND (i.e. with SD card *removed*)
1. Run `fw_setenv bootdelay 3` there to get the bootdelay prompt of U-Boot
1. Next time you boot (no need to pull the plug, just do `# reboot` in Tina Linux),
   U-Boot will prompt you with this countdown:
   `Hit any key to stop autoboot:  3`

### Regular boot

1. Run picocom with gkermit (`scripts/d1-term.sh` does that)
1. Interrupt U-Boot into its shell
    * i.e. hit any key when it does the countdown: `Hit any key to stop autoboot:  3`
1. Run `loadb <address> <baud>` in U-Boot shell:
    * `loadb 40200000 115200`
1. When it says `Ready for binary (kermit) download to 0x40200000 at 115200 bps...`,
   hit Ctrl-A, Ctrl-S to get it to prompt for a file, then type out the file name:
    * `*** file: out/os_d1_u.bin`
1. When done loading, it will get back to U-Boot prompt, it’s time to boot:
    * `booti 0x40200000`

Commands repeated here for more convenient copy-pasting:
```
loadb 40200000 115200
out/os_d1_u.bin
booti 0x40200000
```
