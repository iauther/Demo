@echo off
if "%1" == "0" (
copy /y  %2  ..\..\..\..\output\
..\mkupg %1 ..\..\..\..\output\boot.bin
) else (
copy /y  %2  ..\..\..\..\output\
..\mkupg %1 ..\..\..\..\output\app.bin
)

