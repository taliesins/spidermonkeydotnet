@echo off


SET python=%~dp0%third_party\python_274\python.exe


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

set ionmonkeydir="C:\Data\ionmonkey"
set reldir="Binaries"
if exist %reldir% rmdir /s /q "%reldir%"
mkdir "%reldir%"
if errorlevel 1 goto error


if exist %reldir% rmdir /s /q "ionmonkey"

xcopy /e "%ionmonkeydir%\nsprpub\build-release-net35-amd64\dist\*.*" "vs2008\Net35\x64\nsprpub\"
xcopy /e "%ionmonkeydir%\nsprpub\build-release-net35-x86\dist\bin\*.*" "vs2008\Net35\win32\nsprpub\"
xcopy /e "%ionmonkeydir%\nsprpub\build-release-net40-amd64\dist\bin\*.*" "vs2010\Net40\x64\nsprpub\"
xcopy /e "%ionmonkeydir%\nsprpub\build-release-net40-x86\dist\bin\*.*" "vs2010\Net40\win32\nsprpub\"
xcopy /e "%ionmonkeydir%\nsprpub\build-release-net45-amd64\dist\bin\*.*" "vs2012\Net45\x64\nsprpub\"
xcopy /e "%ionmonkeydir%\nsprpub\build-release-net45-x86\dist\bin\*.*" "vs2012\Net45\win32\nsprpub\"

xcopy /e "%ionmonkeydir%\js\src\build-release-net35-amd64\dist\*.*" "vs2008\Net35\x64\js\"
xcopy /e "%ionmonkeydir%\js\src\build-release-net35-x86\dist\*.*" "vs2008\Net35\win32\js\"
xcopy /e "%ionmonkeydir%\js\src\build-release-net40-amd64\dist\*.*" "vs2010\Net40\x64\js\"
xcopy /e "%ionmonkeydir%\js\src\build-release-net40-x86\dist\*.*" "vs2010\Net40\win32\js\"
xcopy /e "%ionmonkeydir%\js\src\build-release-net45-amd64\dist\*.*" "vs2012\Net45\x64\js\"
xcopy /e "%ionmonkeydir%\js\src\build-release-net45-x86\dist\*.*" "vs2012\Net45\win32\js\"

mkdir "%reldir%\lib"
mkdir "%reldir%\lib\Net35"
mkdir "%reldir%\lib\Net40"
mkdir "%reldir%\lib\Net45"
mkdir "%reldir%\lib\Net35\x86"
mkdir "%reldir%\lib\Net35\amd64"
mkdir "%reldir%\lib\Net40\x86"
mkdir "%reldir%\lib\Net40\amd64"
mkdir "%reldir%\lib\Net45\x86"
mkdir "%reldir%\lib\Net45\amd64"

copy "%ionmonkeydir%\nsprpub\build-release-net35-amd64\dist\bin\libnspr4.dll" "%reldir%\NativeBinaries\Net35\amd64\"
copy "%ionmonkeydir%\nsprpub\build-release-net35-x86\dist\bin\libnspr4.dll" "%reldir%\NativeBinaries\Net35\x86\"
copy "%ionmonkeydir%\nsprpub\build-release-net40-amd64\dist\bin\libnspr4.dll" "%reldir%\NativeBinaries\Net40\amd64\"
copy "%ionmonkeydir%\nsprpub\build-release-net40-x86\dist\bin\libnspr4.dll" "%reldir%\NativeBinaries\Net40\x86\"
copy "%ionmonkeydir%\nsprpub\build-release-net45-amd64\dist\bin\libnspr4.dll" "%reldir%\NativeBinaries\Net45\amd64\"
copy "%ionmonkeydir%\nsprpub\build-release-net45-x86\dist\bin\libnspr4.dll" "%reldir%\NativeBinaries\Net45\x86\"

copy "%ionmonkeydir%\js\src\build-release-net35-amd64\dist\bin\mozjs.dll" "%reldir%\NativeBinaries\Net35\amd64\"
copy "%ionmonkeydir%\js\src\build-release-net35-x86\dist\bin\mozjs.dll" "%reldir%\NativeBinaries\Net35\x86\"
copy "%ionmonkeydir%\js\src\build-release-net40-amd64\dist\bin\mozjs.dll" "%reldir%\NativeBinaries\Net40\amd64\"
copy "%ionmonkeydir%\js\src\build-release-net40-x86\dist\bin\mozjs.dll" "%reldir%\NativeBinaries\Net40\x86\"
copy "%ionmonkeydir%\js\src\build-release-net45-amd64\dist\bin\mozjs.dll" "%reldir%\NativeBinaries\Net45\amd64\"
copy "%ionmonkeydir%\js\src\build-release-net45-x86\dist\bin\mozjs.dll" "%reldir%\NativeBinaries\Net45\x86\"

mkdir "%reldir%\NativeBinaries"
mkdir "%reldir%\NativeBinaries\Net35"
mkdir "%reldir%\NativeBinaries\Net40"
mkdir "%reldir%\NativeBinaries\Net45"
mkdir "%reldir%\NativeBinaries\Net35\x86"
mkdir "%reldir%\NativeBinaries\Net35\amd64"
mkdir "%reldir%\NativeBinaries\Net40\x86"
mkdir "%reldir%\NativeBinaries\Net40\amd64"
mkdir "%reldir%\NativeBinaries\Net45\x86"
mkdir "%reldir%\NativeBinaries\Net45\amd64"

copy "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\redist\x86\Microsoft.VC90.CRT\*.*" "%reldir%\NativeBinaries\Net35\x86\"
copy "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\redist\amd64\Microsoft.VC90.CRT\*.*" "%reldir%\NativeBinaries\Net35\amd64\"
copy "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\redist\x86\Microsoft.VC100.CRT\*.*" "%reldir%\NativeBinaries\Net40\x86\"
copy "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\redist\x64\Microsoft.VC100.CRT\*.*" "%reldir%\NativeBinaries\net40\amd64\"
copy "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\redist\x86\Microsoft.VC110.CRT\*.*" "%reldir%\NativeBinaries\Net45\x86\"
copy "C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\redist\x64\Microsoft.VC110.CRT\*.*" "%reldir%\NativeBinaries\Net45\amd64\"



goto end

:error
echo Build aborted
goto end
:usage
:end
