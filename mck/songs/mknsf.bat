@echo off

call set_envs.bat

if not exist effect.h goto compile
del effect.h

:compile
%PPMCK_BASEDIR%\bin\ppmckc -i %1.mml
if not exist effect.h goto err

if not exist ppmck.nes goto assemble
del ppmck.nes
:assemble
%PPMCK_BASEDIR%\bin\nesasm -s -raw ppmck.asm
if not exist ppmck.nes goto err

if not exist %1.nsf goto renam
del %1.nsf
:renam
ren ppmck.nes %1.nsf
rem start %1.nsf
goto finally
:err
:finally