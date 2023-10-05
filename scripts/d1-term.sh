#!/bin/bash

sudo picocom -b 115200 --imap lfcrlf --send-cmd 'gkermit -iXvs' /dev/ttyUSB0
