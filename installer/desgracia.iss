; Inno Setup script: installs DESGRACIA.vst3 into the standard VST3 folder + presets
#define AppName "Desgracia"
#define AppVersion "2.1.0"

[Setup]
AppId={{6C1F2B7E-5D2A-4F0B-9E7C-DE5C0A1A0001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Ciro
DefaultDirName={commoncf64}\VST3
DisableDirPage=no
DirExistsWarning=no
DisableProgramGroupPage=yes
OutputBaseFilename=Desgracia-Setup-Windows-x64
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
UninstallFilesDir={commonpf64}\Desgracia

[Files]
Source: "..\build\Desgracia_artefacts\Release\VST3\Desgracia.vst3\*"; DestDir: "{app}\Desgracia.vst3"; Flags: recursesubdirs ignoreversion

[Messages]
WelcomeLabel2=Esto instala DESGRACIA (VST3, 64-bit) en la carpeta estandar de plugins VST3.%n%nDespues abri tu DAW y escanea plugins.
