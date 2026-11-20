#ifndef Version
  #define Version "1.1.0"
#endif
#ifndef BundleDir
  #error BundleDir is required
#endif
#ifndef OutputFolder
  #error OutputFolder is required
#endif

[Setup]
; The 1.0.0 installer used AppName as the implicit AppId.
AppName=Skeuomorphic Space Panner
AppVersion={#Version}
AppPublisher=gett1
DefaultDirName={commoncf64}\VST3\Skeuomorphic Space Panner
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputFolder}
OutputBaseFilename=Skeuomorphic_Space_Panner_Win_Installer
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
italian.LegacyBusy=Impossibile aggiornare il plug-in esistente. Chiudi la DAW e riprova.
english.LegacyBusy=Unable to update the existing plug-in. Close your DAW and try again.

[Files]
Source: "{#BundleDir}\*"; DestDir: "{app}\Skeuomorphic Space Panner.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\Manuali\Manuale_Utente.pdf"; DestDir: "{commondocs}\Gettone\Skeuomorphic Space Panner"; DestName: "manuale_space_panner.pdf"; Flags: ignoreversion
Source: "..\Manuali\Installazione_Windows.pdf"; DestDir: "{commondocs}\Gettone\Skeuomorphic Space Panner"; Flags: ignoreversion
Source: "..\LEGGIMI.txt"; DestDir: "{commondocs}\Gettone\Skeuomorphic Space Panner"; DestName: "README.txt"; Flags: ignoreversion
Source: "..\Assets\OFL.txt"; DestDir: "{commondocs}\Gettone\Skeuomorphic Space Panner"; DestName: "PixelifySans-OFL.txt"; Flags: ignoreversion

[Icons]
Name: "{commonprograms}\Skeuomorphic Space Panner Manual"; Filename: "{commondocs}\Gettone\Skeuomorphic Space Panner\manuale_space_panner.pdf"

[Code]
var
  LegacyFile: String;
  LegacyBackup: String;
  LegacyMoved: Boolean;
  InstallCompleted: Boolean;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  LegacyFile := ExpandConstant('{app}\Skeuomorphic Space Panner.vst3');
  if FileExists(LegacyFile) then
  begin
    LegacyBackup := LegacyFile + '.v1.0-backup';
    if FileExists(LegacyBackup) or DirExists(LegacyBackup) then
    begin
      Result := CustomMessage('LegacyBusy') + #13#10 + LegacyBackup;
      Exit;
    end;
    LegacyMoved := RenameFile(LegacyFile, LegacyBackup);
    if not LegacyMoved then
      Result := CustomMessage('LegacyBusy');
  end;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssDone then
  begin
    InstallCompleted := True;
    if LegacyMoved then
      DeleteFile(LegacyBackup);
  end;
end;

procedure DeinitializeSetup;
begin
  if LegacyMoved and not InstallCompleted then
  begin
    if DirExists(LegacyFile) then
      RemoveDir(LegacyFile);
    if not FileExists(LegacyFile) and not DirExists(LegacyFile) then
      RenameFile(LegacyBackup, LegacyFile);
  end;
end;
