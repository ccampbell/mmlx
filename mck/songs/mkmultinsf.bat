@echo off

call set_envs.bat

if not exist effect.h goto compile
del effect.h

:compile


if "%OS%" == "Windows_NT" goto WinNT
%PPMCK_BASEDIR%\bin\ppmckc -i -u %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofppmckc
:WinNT
%PPMCK_BASEDIR%\bin\ppmckc -i -u %*
:endofppmckc


if not exist effect.h goto err

if not exist ppmck.nes goto assemble
del ppmck.nes
:assemble
%PPMCK_BASEDIR%\bin\nesasm -s -raw ppmck.asm
if not exist ppmck.nes goto err

if not exist multisong.nsf goto renam
del multisong.nsf
:renam
ren ppmck.nes multisong.nsf
rem start multisong.nsf
goto finally
:err
:finally