@ECHO OFF
SETLOCAL

SET ROOT=%~dp0
IF "%ROOT:~-1%"=="\" SET ROOT=%ROOT:~0,-1%

SET QT=%LOCALAPPDATA%\Programs\Qt\6.9.3\msvc2022_64\bin
SET NSIS_EXE=%ProgramFiles(x86)%\NSIS\makensis.exe
SET BUILDDIR=%ROOT%\build\Release
SET BUILDDIR_QM=%ROOT%\build
SET APP=FocusWriter
SET DEPLOY=%ROOT%\deploy\%APP%

RMDIR /S /Q "%ROOT%\deploy" 2>NUL
MKDIR "%DEPLOY%"

REM === BASE ===
COPY "%BUILDDIR%\%APP%.exe" "%DEPLOY%\%APP%.exe" >nul
COPY "%BUILDDIR%\iconv-2.dll" "%DEPLOY%\" >nul
COPY "%BUILDDIR%\zlib1.dll" "%DEPLOY%\" >nul

REM === TRADUCCIONES ===
MKDIR "%DEPLOY%\translations"
COPY "%BUILDDIR_QM%\focuswriter_*.qm" "%DEPLOY%\translations" >nul

REM === RECURSOS (idénticos al portable) ===
ROBOCOPY "%ROOT%\resources\images\icons\oxygen\hicolor" "%DEPLOY%\icons\hicolor" /E /NFL /NDL /NJH /NJS >nul
ROBOCOPY "%ROOT%\resources\windows\dicts" "%DEPLOY%\dictionaries" /E /NFL /NDL /NJH /NJS >nul
ROBOCOPY "%ROOT%\resources\sounds" "%DEPLOY%\sounds" /E /NFL /NDL /NJH /NJS >nul
COPY "%ROOT%\resources\symbols\symbols1600.dat" "%DEPLOY%\" >nul
ROBOCOPY "%ROOT%\resources\themes" "%DEPLOY%\themes" /E /NFL /NDL /NJH /NJS >nul

REM === QT6 ===
"%QT%\windeployqt6.exe" --no-compiler-runtime --no-opengl-sw --no-system-dxc-compiler --no-system-d3d-compiler --skip-plugin-types iconengines "%DEPLOY%\%APP%.exe"

REM === README ===
TYPE "%ROOT%\README" >> "%DEPLOY%\ReadMe.txt"
TYPE "%ROOT%\CREDITS" >> "%DEPLOY%\ReadMe.txt"
TYPE "%ROOT%\ChangeLog" >> "%DEPLOY%\ReadMe.txt"

REM Ejecución dinámica de NSIS
"%NSIS_EXE%" FocusWriter.nsi

ENDLOCAL
