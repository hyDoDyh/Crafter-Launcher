<#
.SYNOPSIS
    根据一个「部署文件夹」+「图标文件」自动生成 Inno Setup 安装包。

.DESCRIPTION
    脚本会：
      1. 扫描 -SourceDir（Qt 的 windeployqt 输出目录、或任何"绿色版"目录），自动识别主程序 exe、版本号；
      2. 根据参数生成一个 Inno Setup 脚本（.iss）；
      3. 调用 ISCC.exe 编译，输出 Setup exe 并打印大小与 SHA256。

    生成的安装包默认：
      · 按当前用户安装到 %LOCALAPPDATA%\Programs\<AppName>（不需要管理员权限）；
      · 中文（简体）+ 英文双语界面；
      · 开始菜单快捷方式（可选桌面图标），图标取自 -IconFile；
      · 卸载时清理安装目录下的 *.ini / *.log。

.PARAMETER SourceDir
    要打包的文件夹（必填）。里面的内容会被整棵目录树原样安装到 {app}。

.PARAMETER IconFile
    安装程序与快捷方式使用的 .ico 文件（可选）。
    省略时会自动在脚本目录和 SourceDir 里找 *.ico；找不到则使用 exe 自带图标。

.EXAMPLE
    .\Build-Installer.ps1 -SourceDir "D:\Out\MyApp" -IconFile ".\app.ico" -AppVersion 1.2.0

.EXAMPLE
    # 版本号从 exe 的版本资源里读，输出 MyApp-Setup-1.2.0.exe
    .\Build-Installer.ps1 -SourceDir "D:\Out\MyApp" -IconFile ".\app.ico"

.EXAMPLE
    # 保留用户配置（卸载时不清 *.ini / *.log），并留着生成的 .iss 方便排查
    .\Build-Installer.ps1 -SourceDir "D:\Out\MyApp" -PreserveUserData -KeepIss
#>
#Requires -Version 5.1
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0, HelpMessage = '要打包的部署文件夹')]
    [string]$SourceDir,

    [Parameter(Position = 1, HelpMessage = '.ico 图标文件（可省略）')]
    [string]$IconFile,

    [string]$AppName,
    [string]$AppVersion,
    [string]$Publisher = 'Craftable Studio',
    [string]$AppUrl,
    [string]$ExeName,
    [string]$AppId,

    [ValidateSet('lowest', 'admin')]
    [string]$Privileges = 'lowest',

    [ValidateSet('x64compatible', 'x86compatible', 'x86 x64compatible')]
    [string]$Arch = 'x64compatible',

    [string]$MinWindowsVersion = '10.0.17763',
    [string]$OutputDir,
    [string]$OutputName,
    [string]$Iscc,

    [string[]]$Exclude = @('*.pdb', '*.ilk', '*.exp', '*.lib'),

    # 卸载时清掉安装目录下的 *.ini / *.log（默认行为）；加上此开关则保留
    [switch]$PreserveUserData,

    # 把生成的 .iss 留在输出目录里，方便检查或手工微调
    [switch]$KeepIss,

    # 编译完成后打开输出目录
    [switch]$OpenOutput,

    # 打印 ISCC 的全部编译日志（默认只输出结果与错误）
    [switch]$ShowCompilerOutput
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Write-Step([string]$Text) { Write-Host "==> $Text" -ForegroundColor Cyan }
function Write-Note([string]$Text) { Write-Host "    $Text" -ForegroundColor DarkGray }
function Write-Warn2([string]$Text) { Write-Host "[警告] $Text" -ForegroundColor Yellow }

# ---------------------------------------------------------------- 0. 参数校验
$SourceDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($SourceDir)
if (-not (Test-Path -LiteralPath $SourceDir -PathType Container)) {
    throw "源文件夹不存在：$SourceDir"
}
$SourceDir = (Resolve-Path -LiteralPath $SourceDir).ProviderPath.TrimEnd('\')

# Inno 脚本是纯文本，路径里出现这些字符会破坏脚本语法，提前拒绝而不是静默生成坏包
foreach ($p in @($SourceDir, $OutputDir, $IconFile)) {
    if ($p -and ($p -match '[#{}"]')) {
        throw ('路径里含有 Inno Setup 无法安全处理的字符（# { } 或引号）：' + $p + '。请把目录改名或移到别的路径。')
    }
}

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition

# ---------------------------------------------------------------- 1. 识别主程序
Write-Step '扫描源文件夹'
$topExes = @(Get-ChildItem -LiteralPath $SourceDir -Filter *.exe -File -ErrorAction SilentlyContinue)
if (-not $ExeName) {
    if ($topExes.Count -eq 0) {
        throw "在 $SourceDir 顶层没有找到任何 .exe，无法确定主程序。可用 -ExeName 指定。"
    }
    elseif ($topExes.Count -eq 1) {
        $ExeName = $topExes[0].Name
    }
    else {
        $biggest = $topExes | Sort-Object Length -Descending | Select-Object -First 1
        $ExeName = $biggest.Name
        Write-Warn2 ("发现 {0} 个 exe（{1}），按体积最大猜主程序为 {2}；如有误请用 -ExeName 指定。" -f `
                $topExes.Count, (($topExes | ForEach-Object { $_.Name }) -join ', '), $ExeName)
    }
}
$mainExe = Join-Path $SourceDir $ExeName
if (-not (Test-Path -LiteralPath $mainExe -PathType Leaf)) {
    throw "指定的主程序不存在：$mainExe"
}
$mainExeItem = Get-Item -LiteralPath $mainExe
Write-Note "主程序：$ExeName（$([math]::Round($mainExeItem.Length / 1MB, 2)) MB）"

$fileCount = @(Get-ChildItem -LiteralPath $SourceDir -Recurse -File).Count
$totalMB = [math]::Round((Get-ChildItem -LiteralPath $SourceDir -Recurse -File | Measure-Object Length -Sum).Sum / 1MB, 2)
Write-Note "共 $fileCount 个文件，$totalMB MB"

# Qt 部署自检：装了 Qt6Core 却没带 platforms\qwindows.dll 是最常见的"拷过去打不开"
if (Test-Path -LiteralPath (Join-Path $SourceDir 'Qt6Core.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $SourceDir 'platforms\qwindows.dll'))) {
        Write-Warn2 "目录里有 Qt6Core.dll 但缺少 platforms\qwindows.dll，装完会提示 "could not find or load the Qt platform plugin"。建议先跑 windeployqt。"
    }
}

# ---------------------------------------------------------------- 2. 应用元数据
if (-not $AppName) { $AppName = [System.IO.Path]::GetFileNameWithoutExtension($ExeName) }

if (-not $AppVersion) {
    $vi = $mainExeItem.VersionInfo
    if ($vi -and $vi.ProductVersion) { $AppVersion = $vi.ProductVersion.Trim() }
    elseif ($vi -and $vi.FileVersion) { $AppVersion = $vi.FileVersion.Trim() }
    else {
        $AppVersion = '1.0.0'
        Write-Warn2 "exe 没有版本资源，版本号用默认的 $AppVersion；可用 -AppVersion 指定。"
    }
    Write-Note "版本号（从 exe 读取）：$AppVersion"
}

# AppId 决定"升级"还是"并存安装"：同一个 AppName 必须永远得到同一个 GUID。
# 没显式传 -AppId 时用 AppName 做 MD5，保证多次构建一致。
if (-not $AppId) {
    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $hash = $md5.ComputeHash([System.Text.Encoding]::UTF8.GetBytes("InnoAppId:$AppName"))
    }
    finally { $md5.Dispose() }
    $AppId = (New-Object System.Guid(, $hash)).ToString('D').ToUpperInvariant()
}
$AppId = $AppId.Trim('{', '}')

# VersionInfoVersion 只接受 1~4 段数字
$numericVersion = '1.0.0.0'
if ($AppVersion -match '^\d+(\.\d+){0,3}$') { $numericVersion = $AppVersion }
else { Write-Warn2 "版本号 `"$AppVersion`" 不是纯数字，版本资源里写作 $numericVersion。" }

$AppVerName = "$AppName $AppVersion"

# ---------------------------------------------------------------- 3. 图标
$iconResolved = $null
if ($IconFile) {
    $iconResolved = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($IconFile)
    if (-not (Test-Path -LiteralPath $iconResolved -PathType Leaf)) { throw "图标文件不存在：$iconResolved" }
}
else {
    # 自动找图标：先找与本应用同名的（<AppName>.ico），再找源文件夹里唯一的 .ico。
    # 脚本目录里其他应用的图标不会被借用，避免打错包。
    $cands = @()
    $sameName = Join-Path $ScriptDir ($AppName + '.ico')
    if (Test-Path -LiteralPath $sameName -PathType Leaf) {
        $cands += @(Get-Item -LiteralPath $sameName)
    }
    else {
        $srcIcons = @(Get-ChildItem -LiteralPath $SourceDir -Filter *.ico -File -ErrorAction SilentlyContinue)
        if ($srcIcons.Count -eq 1) { $cands += $srcIcons }
    }
    if ($cands.Count -gt 0) {
        $iconResolved = $cands[0].FullName
        Write-Note "未指定 -IconFile，自动使用：$iconResolved"
    }
}
if ($iconResolved) {
    $fs = [System.IO.File]::OpenRead($iconResolved)
    try {
        $mb = New-Object byte[] 4
        $null = $fs.Read($mb, 0, 4)
    }
    finally { $fs.Dispose() }
    if (-not ($mb[0] -eq 0 -and $mb[1] -eq 0 -and $mb[2] -eq 1 -and $mb[3] -eq 0)) {
        throw "不是合法的 .ico 文件（文件头不对）：$iconResolved"
    }
    $iconResolved = (Resolve-Path -LiteralPath $iconResolved).ProviderPath
    Write-Note "图标：$iconResolved"
}
else {
    Write-Warn2 '没有提供图标，安装程序和快捷方式将使用 exe 自带图标（exe 也没有图标时是空白图标）。'
}

# 图标若已在源文件夹里，[Files] 的递归条目已经会带上它，不要重复安装
$iconInsideSource = $false
$iconDestName = $null
if ($iconResolved) {
    $iconDestName = [System.IO.Path]::GetFileName($iconResolved)
    if ($iconResolved.StartsWith($SourceDir + '\', [System.StringComparison]::OrdinalIgnoreCase)) {
        $iconInsideSource = $true
    }
}

# ---------------------------------------------------------------- 4. 定位 ISCC
$isccCandidates = New-Object System.Collections.Generic.List[string]
if ($Iscc) { $isccCandidates.Add($Iscc) }
$uninstKeys = @(
    'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1',
    'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1',
    'HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Inno Setup 6_is1'
)
foreach ($k in $uninstKeys) {
    if (Test-Path $k) {
        $loc = (Get-ItemProperty -Path $k -Name InstallLocation -ErrorAction SilentlyContinue).InstallLocation
        if ($loc) { $isccCandidates.Add((Join-Path $loc 'ISCC.exe')) }
    }
}
$isccCandidates.Add((Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'))
if (${env:ProgramFiles(x86)}) { $isccCandidates.Add((Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe')) }
if ($env:ProgramFiles) { $isccCandidates.Add((Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')) }
$cmdIscc = Get-Command ISCC.exe -ErrorAction SilentlyContinue
if ($cmdIscc) { $isccCandidates.Add($cmdIscc.Source) }

$isccPath = $null
foreach ($c in $isccCandidates) { if ($c -and (Test-Path -LiteralPath $c -PathType Leaf)) { $isccPath = $c; break } }
if (-not $isccPath) {
    throw "找不到 ISCC.exe。请先安装 Inno Setup 6（winget install JRSoftware.InnoSetup），或用 -Iscc 指定路径。"
}
Write-Note "编译器：$isccPath"

# ---------------------------------------------------------------- 5. 输出路径
if (-not $OutputDir) { $OutputDir = Join-Path $ScriptDir 'Output' }
$OutputDir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDir)
if (-not (Test-Path -LiteralPath $OutputDir)) { $null = New-Item -ItemType Directory -Path $OutputDir -Force }

if (-not $OutputName) { $OutputName = "$AppName-Setup-$AppVersion" }
$issPath = Join-Path $OutputDir "$OutputName.iss"
$setupPath = Join-Path $OutputDir "$OutputName.exe"

# ---------------------------------------------------------------- 6. 生成 .iss
Write-Step '生成 Inno Setup 脚本'

$esc = { param($s) $s.Replace('"', '""') }   # 兜底：路径里的引号（正常不会有）
$src = & $esc $SourceDir
$out = & $esc $OutputDir
$exe = & $esc $ExeName

$L = New-Object System.Collections.Generic.List[string]
$L.Add('; 由 Build-Installer.ps1 自动生成，请勿手工修改')
$L.Add('; 来源目录: ' + $SourceDir)
$L.Add('')
$L.Add('[Setup]')
$L.Add('AppId={{' + $AppId + '}')
$L.Add('AppName=' + $AppName)
$L.Add('AppVersion=' + $AppVersion)
$L.Add('AppVerName=' + $AppVerName)
$L.Add('AppPublisher=' + $Publisher)
if ($AppUrl) {
    $L.Add('AppPublisherURL=' + $AppUrl)
    $L.Add('AppSupportURL=' + $AppUrl + '/issues')
    $L.Add('AppUpdatesURL=' + $AppUrl + '/releases')
}
$L.Add('VersionInfoVersion=' + $numericVersion)
$L.Add('VersionInfoCompany=' + $Publisher)
$L.Add('VersionInfoDescription=' + $AppVerName + ' Setup')
$L.Add('VersionInfoProductName=' + $AppName)
$L.Add('VersionInfoProductVersion=' + $numericVersion)
$L.Add('')
$L.Add('; lowest = 装到当前用户目录，不需要管理员权限；程序若把配置写在 exe 同级，必须用 lowest')
$L.Add('PrivilegesRequired=' + $Privileges)
$L.Add('DefaultDirName={autopf}\' + $AppName)
$L.Add('DefaultGroupName=' + $AppName)
$L.Add('DisableProgramGroupPage=yes')
$L.Add('ArchitecturesAllowed=' + $Arch)
if ($Arch -ne 'x86compatible') { $L.Add('ArchitecturesInstallIn64BitMode=' + $Arch) }
$L.Add('MinVersion=' + $MinWindowsVersion)
$L.Add('')
$L.Add('OutputDir=' + $out)
$L.Add('OutputBaseFilename=' + $OutputName)
if ($iconResolved) { $L.Add('SetupIconFile=' + $iconResolved) }
if ($iconResolved) { $L.Add('UninstallDisplayIcon={app}\' + $iconDestName) }
else { $L.Add('UninstallDisplayIcon={app}\' + $ExeName) }
$L.Add('UninstallDisplayName=' + $AppVerName)
$L.Add('')
$L.Add('Compression=lzma2/ultra64')
$L.Add('SolidCompression=yes')
$L.Add('WizardStyle=modern')
$L.Add('CloseApplications=yes')
$L.Add('RestartApplications=no')
$L.Add('AllowNoIcons=yes')
$L.Add('ShowLanguageDialog=auto')
$L.Add('')
$L.Add('[Languages]')

$chineseIsl = Join-Path $ScriptDir 'ChineseSimplified.isl'
$hasChinese = Test-Path -LiteralPath $chineseIsl
if ($hasChinese) {
    $L.Add('Name: "chinesesimplified"; MessagesFile: "' + $chineseIsl + '"')
}
else {
    Write-Warn2 "脚本目录下没有 ChineseSimplified.isl，安装界面只提供英文。"
}
$L.Add('Name: "english"; MessagesFile: "compiler:Default.isl"')
$L.Add('')
$L.Add('[Tasks]')
$L.Add('Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"')
$L.Add('')
$L.Add('[Files]')
$excludes = ''
if ($Exclude -and $Exclude.Count -gt 0) { $excludes = '; Excludes: "' + ($Exclude -join ',') + '"' }
$L.Add('Source: "' + $src + '\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs' + $excludes)
if ($iconResolved -and -not $iconInsideSource) {
    $L.Add('; 图标单独安装，供快捷方式/卸载项引用')
    $L.Add('Source: "' + $iconResolved + '"; DestDir: "{app}"; Flags: ignoreversion')
}
$L.Add('')
$L.Add('[Icons]')
if ($iconResolved) {
    $L.Add('Name: "{autoprograms}\' + $AppName + '"; Filename: "{app}\' + $ExeName + '"; IconFilename: "{app}\' + $iconDestName + '"')
    $L.Add('Name: "{autoprograms}\{cm:UninstallProgram,' + $AppName + '}"; Filename: "{uninstallexe}"; IconFilename: "{app}\' + $iconDestName + '"')
    $L.Add('Name: "{autodesktop}\' + $AppName + '"; Filename: "{app}\' + $ExeName + '"; IconFilename: "{app}\' + $iconDestName + '"; Tasks: desktopicon')
}
else {
    $L.Add('Name: "{autoprograms}\' + $AppName + '"; Filename: "{app}\' + $ExeName + '"')
    $L.Add('Name: "{autoprograms}\{cm:UninstallProgram,' + $AppName + '}"; Filename: "{uninstallexe}"')
    $L.Add('Name: "{autodesktop}\' + $AppName + '"; Filename: "{app}\' + $ExeName + '"; Tasks: desktopicon')
}
$L.Add('')
$L.Add('[Run]')
$L.Add('Filename: "{app}\' + $ExeName + '"; Description: "{cm:LaunchProgram,' + $AppName + '}"; Flags: nowait postinstall skipifsilent')

if (-not $PreserveUserData) {
    $L.Add('')
    $L.Add('[UninstallDelete]')
    $L.Add('; 卸载时清掉程序运行时在安装目录生成的配置与日志')
    $L.Add('Type: files; Name: "{app}\*.ini"')
    $L.Add('Type: files; Name: "{app}\*.log"')
}

[System.IO.File]::WriteAllText($issPath, ($L -join "`r`n") + "`r`n", (New-Object System.Text.UTF8Encoding($false)))
Write-Note "脚本：$issPath"

# ---------------------------------------------------------------- 7. 编译
Write-Step '编译安装包'
if (Test-Path -LiteralPath $setupPath) { Remove-Item -LiteralPath $setupPath -Force }

$sw = [System.Diagnostics.Stopwatch]::StartNew()
if ($ShowCompilerOutput) { & $isccPath $issPath }
else { & $isccPath '/Q' $issPath }
$code = $LASTEXITCODE
$sw.Stop()

if ($code -ne 0) { throw "编译失败（ISCC 退出码 $code）。生成的 .iss 保留在：$issPath" }
if (-not (Test-Path -LiteralPath $setupPath)) { throw "ISCC 报告成功，但没有找到输出文件：$setupPath" }

if (-not $KeepIss) { Remove-Item -LiteralPath $issPath -Force -ErrorAction SilentlyContinue }

# ---------------------------------------------------------------- 8. 结果
$setupItem = Get-Item -LiteralPath $setupPath
# 用 .NET 算哈希：某些环境里 Get-FileHash 所在的模块不可用
$sha = [System.Security.Cryptography.SHA256]::Create()
try {
    $fsHash = [System.IO.File]::OpenRead($setupPath)
    try { $hashBytes = $sha.ComputeHash($fsHash) } finally { $fsHash.Dispose() }
}
finally { $sha.Dispose() }
$hash = ([System.BitConverter]::ToString($hashBytes) -replace '-', '')
$viOut = $setupItem.VersionInfo
# Inno 会把版本资源的字符串补齐到固定宽度，显示时去掉尾部空格
$viProdName = if ($viOut.ProductName) { $viOut.ProductName.Trim() } else { '' }
$viProdVer = if ($viOut.ProductVersion) { $viOut.ProductVersion.Trim() } else { '' }
$viCompany = if ($viOut.CompanyName) { $viOut.CompanyName.Trim() } else { '' }

Write-Host ''
Write-Host '安装包已生成' -ForegroundColor Green
$installedCount = $fileCount
if ($iconResolved -and -not $iconInsideSource) { $installedCount++ }
Write-Note "路径：$setupPath"
Write-Note ("大小：{0} MB（{1} 字节）" -f [math]::Round($setupItem.Length / 1MB, 2), $setupItem.Length)
Write-Note "SHA256：$hash"
Write-Note ("版本信息：{0} / {1} / {2}" -f $viProdName, $viProdVer, $viCompany)
Write-Note ("打包内容：{0} 个文件，{1} MB；耗时 {2} 秒" -f $installedCount, $totalMB, [math]::Round($sw.Elapsed.TotalSeconds, 1))

if ($OpenOutput) { Start-Process -FilePath explorer.exe -ArgumentList ('"' + $OutputDir + '"') | Out-Null }
