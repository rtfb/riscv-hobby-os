#!/bin/bash

# sudo apt install gkermit
sudo picocom -b 2000000 --imap lfcrlf --send-cmd 'gkermit -iXvs' /dev/ttyUSB0
