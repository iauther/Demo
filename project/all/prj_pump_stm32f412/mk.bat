@echo off
if "%1" == "1" (
copy /y boot\boot.bin ..\..\..\output\
mkfw ..\..\..\output\boot.bin
) else (
copy /y app\app.bin ..\..\..\output\
mkfw ..\..\..\output\app.bin
)

