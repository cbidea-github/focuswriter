@ECHO OFF
SETLOCAL

REM === CONFIG ===
SET ROOT=%~dp0
IF "%ROOT:~-1%"=="\" SET ROOT=%ROOT:~0,-1%

SET QT=%LOCALAPPDATA%\Programs\Qt\6.9.3\msvc2022_64\bin
SET ZIP_EXE=%ProgramFiles%\7-Zip\7z.exe
SET BUILDDIR=%ROOT%\build\Release
SET BUILDDIR_QM=%ROOT%\build
SET APP=FocusWriter
SET VERSION=1.8.13
SET TARGET=%ROOT%\portable\%APP%

REM === LIMPIAR ===
RMDIR /S /Q "%ROOT%\portable" 2>NUL
MKDIR "%TARGET%"

ECHO Copying executable and base DLLs
COPY "%BUILDDIR%\%APP%.exe" "%TARGET%\%APP%.exe" >nul
COPY "%BUILDDIR%\iconv-2.dll" "%TARGET%\" >nul
COPY "%BUILDDIR%\zlib1.dll" "%TARGET%\" >nul

ECHO Copying translations
MKDIR "%TARGET%\translations"
COPY "%BUILDDIR_QM%\focuswriter_*.qm" "%TARGET%\translations" >nul

ECHO Copying icons
ROBOCOPY "%ROOT%\resources\images\icons\oxygen\hicolor" "%TARGET%\icons\hicolor" /E /NFL /NDL /NJH /NJS >nul

ECHO Copying dictionaries
ROBOCOPY "%ROOT%\resources\windows\dicts" "%TARGET%\dictionaries" /E /NFL /NDL /NJH /NJS >nul

ECHO Copying sounds
ROBOCOPY "%ROOT%\resources\sounds" "%TARGET%\sounds" /E /NFL /NDL /NJH /NJS >nul

ECHO Copying symbols
COPY "%ROOT%\resources\symbols\symbols1600.dat" "%TARGET%\" >nul

ECHO Copying themes
ROBOCOPY "%ROOT%\resources\themes" "%TARGET%\themes" /E /NFL /NDL /NJH /NJS >nul

ECHO Copying Qt6 runtime
"%QT%\windeployqt6.exe" --no-translations --no-compiler-runtime --no-opengl-sw --no-system-dxc-compiler --no-system-d3d-compiler --skip-plugin-types iconengines "%TARGET%\%APP%.exe"

ECHO Making portable
MKDIR "%TARGET%\Data"
COPY "%ROOT%\COPYING" "%TARGET%\COPYING.txt" >nul

ECHO Creating ReadMe
TYPE "%ROOT%\README" >> "%TARGET%\ReadMe.txt"
TYPE "%ROOT%\CREDITS" >> "%TARGET%\ReadMe.txt"
TYPE "%ROOT%\ChangeLog" >> "%TARGET%\ReadMe.txt"

ECHO Creating compressed file
"%ZIP_EXE%" a -mx=9 "%ROOT%\%APP%_%VERSION%_portable.zip" "%ROOT%\portable\*"

ENDLOCAL
