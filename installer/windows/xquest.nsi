; XQuest Windows installer - bundles the game data (see assets/README).
; Expects XQUEST_STAGE_DIR to contain: xquest.exe, SDL2.dll, data\*
; and XQUEST_VERSION / XQUEST_OUT to be set by the caller (see
; scripts/build-windows.ps1).

!include "WinMessages.nsh"

!ifndef XQUEST_STAGE_DIR
  !define XQUEST_STAGE_DIR "stage"
!endif
!ifndef XQUEST_VERSION
  !define XQUEST_VERSION "0.0.0"
!endif
!ifndef XQUEST_OUT
  !define XQUEST_OUT "XQuest-Setup.exe"
!endif

Name "XQuest"
OutFile "${XQUEST_OUT}"
InstallDir "$PROGRAMFILES64\XQuest"
RequestExecutionLevel admin

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
    SetOutPath "$INSTDIR"
    File "${XQUEST_STAGE_DIR}\xquest.exe"
    File "${XQUEST_STAGE_DIR}\SDL2.dll"
    SetOutPath "$INSTDIR\data"
    File "${XQUEST_STAGE_DIR}\data\*.*"
    SetOutPath "$INSTDIR"

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    CreateDirectory "$SMPROGRAMS\XQuest"
    CreateShortcut "$SMPROGRAMS\XQuest\XQuest.lnk" "$INSTDIR\xquest.exe"
    CreateShortcut "$SMPROGRAMS\XQuest\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

    ; Persist XQUEST_DATA_DIR so the game finds its data regardless of how
    ; it's launched (Start Menu, double-click, etc).
    WriteRegExpandStr HKCU "Environment" "XQUEST_DATA_DIR" "$INSTDIR\data"
    SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000

    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XQuest" \
        "DisplayName" "XQuest"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XQuest" \
        "DisplayVersion" "${XQUEST_VERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XQuest" \
        "UninstallString" "$INSTDIR\Uninstall.exe"
SectionEnd

Section "Uninstall"
    Delete "$INSTDIR\xquest.exe"
    Delete "$INSTDIR\SDL2.dll"
    Delete "$INSTDIR\data\*.*"
    RMDir "$INSTDIR\data"
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    Delete "$SMPROGRAMS\XQuest\XQuest.lnk"
    Delete "$SMPROGRAMS\XQuest\Uninstall.lnk"
    RMDir "$SMPROGRAMS\XQuest"
    DeleteRegValue HKCU "Environment" "XQUEST_DATA_DIR"
    SendMessage ${HWND_BROADCAST} ${WM_WININICHANGE} 0 "STR:Environment" /TIMEOUT=5000
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XQuest"
SectionEnd
