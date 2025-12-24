@ECHO OFF
SETLOCAL

REM === CONFIG ===
SET ROOT=%~dp0
IF "%ROOT:~-1%"=="\" SET ROOT=%ROOT:~0,-1%

SET QT=%LOCALAPPDATA%\Programs\Qt\6.9.3\msvc2022_64
SET NSIS=%ProgramFiles(x86)%\NSIS

SET BUILD=%ROOT%\build
SET APP=FocusWriter

REM === 1. CMAKE CONFIG ===
ECHO Configuring CMake...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT%"

REM === 2. BUILD ===
ECHO Building...
cmake --build build --config Release

REM === 3. PORTABLE ===
CALL FocusWriter_portable.bat

REM === 4. INSTALLER ===
CALL FocusWriter_installer.bat

ECHO.
ECHO =============================
ECHO  BUILD FINISHED SUCCESSFULLY
ECHO =============================
PAUSE
ENDLOCAL
