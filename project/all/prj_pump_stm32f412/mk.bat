@echo off
if "%1" == "1" (
copy /y  %2  ..\..\..\..\output\
..\mkfw ..\..\..\..\output\boot.bin
) else (
copy /y  %2  ..\..\..\..\output\
..\mkfw ..\..\..\..\output\app.bin
)

