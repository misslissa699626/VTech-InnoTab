@echo off
del /q ..\kernel_packed*.bin 2> nul
innotab_packtool.exe ..\kernel_rel.bin 255
make_upgradebin.exe -kb 255 ..\kernel_rel_packed_v255.bin -sb 255 ..\system_rel.bin
pause
