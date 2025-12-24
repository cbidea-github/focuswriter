!define APPNAME "FocusWriter"
!define APPVERSION "1.8.13"
!define ABOUTURL "https://gottcode.org/focuswriter/"

!include "MUI2.nsh"

SetCompressor /SOLID /FINAL lzma
Name "${APPNAME}"
OutFile "${APPNAME}_${APPVERSION}_Setup.exe"
InstallDir "$PROGRAMFILES64\${APPNAME}"
RequestExecutionLevel admin

Var StartMenuFolder

!define MUI_ICON "resources\\windows\\focuswriter.ico"
!define MUI_UNICON "resources\\windows\\focuswriter.ico"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_STARTMENU Application $StartMenuFolder
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Section "Install"

    SetOutPath "$INSTDIR"
    File /r "deploy\\FocusWriter\\*"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    SetShellVarContext all
    !insertmacro MUI_STARTMENU_WRITE_BEGIN Application
        CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
        CreateShortCut "$SMPROGRAMS\$StartMenuFolder\${APPNAME}.lnk" "$INSTDIR\${APPNAME}.exe"
    !insertmacro MUI_STARTMENU_WRITE_END
    SetShellVarContext current

SectionEnd

Section "Uninstall"
    RMDir /r "$INSTDIR"

    SetShellVarContext all
    !insertmacro MUI_STARTMENU_GETFOLDER Application $StartMenuFolder
    Delete "$SMPROGRAMS\$StartMenuFolder\${APPNAME}.lnk"
    RMDir "$SMPROGRAMS\$StartMenuFolder"
    SetShellVarContext current
SectionEnd
