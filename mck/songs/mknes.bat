@echo off

call set_envs.bat

if not exist effect.h goto compile
del effect.h

:compile
%PPMCK_BASEDIR%\bin\ppmckc -i %1.mml
if not exist effect.h goto err

echo MAKE_NES	.equ	1 >> define.inc

if not exist ppmck.nes goto assemble
del ppmck.nes
:assemble
%PPMCK_BASEDIR%\bin\nesasm -s ppmck.asm
if not exist ppmck.nes goto err

if not exist %1.nes goto renam
del %1.nes
:renam
ren ppmck.nes %1.nes
rem start %1.nes
goto finally
:err
:finally