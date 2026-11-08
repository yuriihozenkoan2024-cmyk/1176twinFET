; TwinFET - Windows installer (Inno Setup 6.3 or newer).
; GitHub Actions builds it automatically. Locally (after the CMake Release build):
;   ISCC /DArtefacts=build\TwinFET_artefacts\Release /DOutDir=dist installer.iss
;
; Installs the VST3 bundle into C:\Program Files\Common Files\VST3 (where every DAW looks)
; and, optionally, the standalone application into C:\Program Files\TwinFET.

#define AppName "TwinFET"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef Artefacts
  #define Artefacts AddBackslash(SourcePath) + "build\TwinFET_artefacts\Release"
#endif
#ifndef OutDir
  #define OutDir AddBackslash(SourcePath) + "dist"
#endif

; optional files: the archive keeps them in fonts\, a flat upload keeps them next to this script
#define FontLicense ""
#if FileExists(AddBackslash(SourcePath) + "fonts\BigShoulders-OFL.txt")
  #define FontLicense AddBackslash(SourcePath) + "fonts\BigShoulders-OFL.txt"
#elif FileExists(AddBackslash(SourcePath) + "BigShoulders-OFL.txt")
  #define FontLicense AddBackslash(SourcePath) + "BigShoulders-OFL.txt"
#endif
#define HaveReadme FileExists(AddBackslash(SourcePath) + "README.md")
#define HaveUkrainian FileExists(AddBackslash(CompilerPath) + "Languages\Ukrainian.isl")

[Setup]
AppId={{6F1C2E4B-7A3D-4C59-9B1E-2D8F0A6C5E31}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=FETLab
DefaultDirName={autopf}\{#AppName}
DisableDirPage=auto
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir={#OutDir}
OutputBaseFilename=TwinFET-Setup-Windows
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
UninstallDisplayName={#AppName} {#AppVersion}

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
#if HaveUkrainian
Name: "uk"; MessagesFile: "compiler:Languages\Ukrainian.isl"
#endif

[CustomMessages]
en.TypeFull=Full installation
en.TypeCustom=Custom installation
en.CompVST3=VST3 plug-in (C:\Program Files\Common Files\VST3)
en.CompStandalone=Standalone application (no DAW needed)
#if HaveUkrainian
uk.TypeFull=Повне встановлення
uk.TypeCustom=Вибіркове встановлення
uk.CompVST3=Плагін VST3 (C:\Program Files\Common Files\VST3)
uk.CompStandalone=Окрема програма (працює без DAW)
#endif

[Types]
Name: "full"; Description: "{cm:TypeFull}"
Name: "custom"; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
Name: "vst3"; Description: "{cm:CompVST3}"; Types: full custom; Flags: fixed
Name: "standalone"; Description: "{cm:CompStandalone}"; Types: full

[InstallDelete]
; an older copy (bundle folder or a single-file .vst3) is replaced cleanly
Type: filesandordirs; Name: "{commoncf64}\VST3\TwinFET.vst3"

[Files]
Source: "{#Artefacts}\VST3\TwinFET.vst3\*"; DestDir: "{commoncf64}\VST3\TwinFET.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: vst3
Source: "{#Artefacts}\Standalone\TwinFET.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: standalone
#if FontLicense != ""
Source: "{#FontLicense}"; DestDir: "{app}"; DestName: "BigShoulders-OFL.txt"; Flags: ignoreversion
#endif
#if HaveReadme
Source: "{#AddBackslash(SourcePath)}README.md"; DestDir: "{app}"; Flags: ignoreversion
#endif

[Icons]
Name: "{autoprograms}\TwinFET"; Filename: "{app}\TwinFET.exe"; Components: standalone

[Run]
Filename: "{app}\TwinFET.exe"; Description: "{cm:LaunchProgram,TwinFET}"; Flags: nowait postinstall skipifsilent unchecked; Components: standalone
