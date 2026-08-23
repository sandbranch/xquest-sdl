; XQuest Windows installer (engine only — see assets/README).
; Expects XQUEST_STAGE_DIR to contain: xquest.exe, SDL2.dll, xquest.ico (optional)
; and XQUEST_VERSION / XQUEST_OUT to be set by the caller (see
; scripts/build-windows.ps1).

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

    WriteUninstaller "$INSTDIR\Uninstall.exe"

    CreateDirectory "$SMPROGRAMS\XQuest"
    CreateShortcut "$SMPROGRAMS\XQuest\XQuest.lnk" "$INSTDIR\xquest.exe"
    CreateShortcut "$SMPROGRAMS\XQuest\Uninstall.lnk" "$INSTDIR\Uninstall.exe"

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
    Delete "$INSTDIR\Uninstall.exe"
    RMDir "$INSTDIR"
    Delete "$SMPROGRAMS\XQuest\XQuest.lnk"
    Delete "$SMPROGRAMS\XQuest\Uninstall.lnk"
    RMDir "$SMPROGRAMS\XQuest"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XQuest"
SectionEnd
