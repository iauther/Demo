@ECHO OFF

SET FWDIR=..\..\..
SET BLDIR=%FWDIR%\build
SET OUTDIR=%FWDIR%\output
SET MU=mu.exe
SET MB=mb.exe
SET BURN=D:\programs\keil5\UV4\UV4.exe

if not exist %OUTDIR% (
    MKDIR %OUTDIR%
)

ECHO ---%BLDIR%\%1\%1.bin
COPY /Y %BLDIR%\%1\%1.bin %OUTDIR%\

if "%1"=="app" (
    %MU% %OUTDIR%\%1.bin
)

if exist %OUTDIR%\boot.bin (
   if exist %OUTDIR%\app.bin (
      DEL /Q %OUTDIR%\image*.bin
      %MB% %OUTDIR%\boot.bin %OUTDIR%\app.upg
   )
)


