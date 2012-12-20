alc-ctl
=======

auxiliary tool for alc5632 sound codec registers manipulation from userspace.
alc-ctl reads and writes device registers over i2c bus.


NOTE about sources
==================

alc-ctl is based on http://people.canonical.com/~diwic/ac100/alc-dump.c (local copy is present in this repo)


USAGE
=====

% cat full-dump.sh
# Load i2c-dev kernel module (if it is not loaded yet)
lsmod | grep "i2c-dev" > /dev/null || sudo modprobe i2c-dev
 
# Dump all registers
for reg in `seq 0 2 96 | xargs -L1 -I% echo "obase=16; %" | bc | tr '[:upper:]' '[:lower:]'` ; do
    sudo ./alc-ctl /dev/i2c-0 r $reg
done

% ./full-dump.sh > before-playback.txt
