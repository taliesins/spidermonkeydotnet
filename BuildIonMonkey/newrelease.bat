@echo off

cmd /c start-msvc9
if errorlevel 1 goto error
cmd /c start-msvc9-x64
if errorlevel 1 goto error


cmd /c start-msvc10
if errorlevel 1 goto error
cmd /c start-msvc10-x64
if errorlevel 1 goto error
cmd /c start-msvc11
if errorlevel 1 goto error
cmd /c start-msvc11-x64
if errorlevel 1 goto error

goto end

:error
echo Build aborted
goto end
:usage
:end
