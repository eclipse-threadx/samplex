set confirm off
set remotetimeout 60
file /home/fdesbiens/core-v/samplex-fd/OpenHW/CORE-V-MCU/slideshow/build/core_v_mcu_slideshow.elf
target extended-remote localhost:3333
load
monitor resume
disconnect
quit
