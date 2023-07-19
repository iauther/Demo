@ECHO OFF

SET FILES=XXX.c

CALL :myTouch %FILES%


:myTouch
FOR %%A IN (%*) DO (
    IF EXIST "%%A" (
        COPY /B "%%A" +,, > nul
    ) else (
        TYPE NUL > "%%A"       
    )
)
EXIT /B


