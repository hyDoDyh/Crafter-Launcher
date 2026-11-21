; ============================================================================
;  Crafter 安装程序脚本 (Inno Setup 6)
;  用法: iscc Crafter.iss
;  输出: installer\Output\Crafter-Setup-0.1.0.exe
; ============================================================================

#define MyAppName        "Crafter"
#define MyAppVersion     "0.1.0"
#define MyAppVerName     "Crafter 0.1.0 Beta"
#define MyAppPublisher   "Craftable Studio"
#define MyAppExeName     "Crafter.exe"
#define MyAppURL         "https://github.com/hyDoDyh/Crafter-Launcher"
#define MyBuildDir       "..\build\Desktop_Qt_6_10_1_MinGW_64_bit-Release\beta 0.1.0"

[Setup]
; AppId 必须保持不变，否则升级会变成并列安装
AppId={{94E22B54-079B-4B75-9F51-CBC3791F90F8}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppVerName}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
VersionInfoVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoDescription={#MyAppVerName} Setup
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}

; Crafter 用 QSettings(IniFormat, UserScope) 把配置写在 exe 同级目录，
; 因此按当前用户安装到 LocalAppData\Programs，避免写入 Program Files 需要管理员权限
PrivilegesRequired=lowest
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; 仅支持 64 位 Windows 10 1809 及以上（Qt 6 的最低要求）
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763

OutputDir=Output
OutputBaseFilename=Crafter-Setup-{#MyAppVersion}
SetupIconFile=crafter.ico
; Crafter.exe 本身没有图标资源（.pro 未配置 RC_ICONS），
; 所以快捷方式和卸载项都用随包安装的 Crafter.ico
UninstallDisplayIcon={app}\Crafter.ico
UninstallDisplayName={#MyAppVerName}

Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no
AllowNoIcons=yes
ShowLanguageDialog=auto

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; 整个部署目录（exe + Qt 运行库 + 插件 + 翻译 + theme.json）
Source: "{#MyBuildDir}\Crafter.exe";     DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyBuildDir}\*.dll";          DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyBuildDir}\theme.json";     DestDir: "{app}"; Flags: ignoreversion
Source: "{#MyBuildDir}\platforms\*";    DestDir: "{app}\platforms";    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\styles\*";       DestDir: "{app}\styles";       Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\iconengines\*";  DestDir: "{app}\iconengines";  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\generic\*";      DestDir: "{app}\generic";      Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\tls\*";          DestDir: "{app}\tls";          Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#MyBuildDir}\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs createallsubdirs
; 应用图标：快捷方式与卸载项使用它（Crafter.exe 无内嵌图标）
Source: "crafter.ico";                  DestDir: "{app}";              Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\Crafter.ico"
Name: "{autoprograms}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"; IconFilename: "{app}\Crafter.ico"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\Crafter.ico"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; 卸载时清掉运行时在安装目录生成的配置与日志
Type: filesandordirs; Name: "{app}\{#MyAppPublisher}"
Type: files;          Name: "{app}\*.ini"
Type: files;          Name: "{app}\*.log"
