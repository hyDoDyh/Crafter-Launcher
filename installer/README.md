# 安装包构建

用 [Inno Setup](https://jrsoftware.org/isinfo.php) 把 Qt 的部署目录（`windeployqt` 的输出，
或任何"绿色版"目录）打包成 Windows 安装程序。

**核心就一句话：给一个文件夹 + 一个 `.ico`，生成 Setup exe。**

```
installer\
├─ Build-Installer.ps1     构建脚本（引擎，参数化）
├─ build-installer.cmd     便捷入口：拖拽 / 双击 / 命令行
├─ build-crafter.cmd       Crafter 的预设，一键出包
├─ crafter.ico             应用图标
├─ ChineseSimplified.isl   安装界面简体中文（可选，缺了就只出英文界面）
└─ Output\                 产物目录（已 gitignore）
```

## 快速开始

三种用法，任选：

**1. 拖拽 / 双击（最省事）**

把部署文件夹（想连图标一起，就把 `.ico` 也一起选上）拖到 `installer\build-installer.cmd` 上，
生成的 `*-Setup-*.exe` 出现在 `installer\Output\`。

双击 `build-installer.cmd` 则会依次询问：文件夹 → 图标（可留空）→ 版本号（可留空自动读 exe）。

**2. 命令行**

```bat
installer\build-installer.cmd "D:\Out\MyApp" "D:\icon\app.ico" 1.2.0
```

**3. 直接调用 PowerShell（需要更多参数时）**

```powershell
installer\Build-Installer.ps1 -SourceDir "D:\Out\MyApp" -IconFile "D:\icon\app.ico" `
    -AppName MyApp -AppVersion 1.2.0 -Publisher "My Studio" `
    -AppUrl "https://github.com/me/MyApp" -PreserveUserData -ShowCompilerOutput
```

**本项目出包（Crafter）**

```bat
installer\build-crafter.cmd
```

它在 `Build-Installer.ps1` 之上固定了 Crafter 的名字、版本、AppId 和图标；
换版本号改 `build-crafter.cmd` 里的 `VER` 即可。

## 参数

| 参数 | 说明 |
| --- | --- |
| `-SourceDir` | **必填**。部署文件夹，整棵目录树原样装进安装目录 |
| `-IconFile` | `.ico` 图标，用于安装程序本身、快捷方式和卸载项。省略时自动在脚本目录/源目录里找 |
| `-AppName` | 应用名，默认取主程序 exe 的文件名 |
| `-AppVersion` | 版本号。省略时读 exe 的版本资源，读不到用 `1.0.0` |
| `-Publisher` | 发布者，默认 `Craftable Studio` |
| `-AppUrl` | 项目主页；给了就一并写入 Publisher/Support/Updates URL |
| `-ExeName` | 主程序文件名。省略时自动识别（顶层只有一个 exe 就用它，多个则按体积最大猜） |
| `-AppId` | 安装标识 GUID。**升级靠它**：同名应用每次构建必须一致，省略时按 AppName 算 MD5 固定生成 |
| `-Privileges` | `lowest`（默认，装到当前用户，免管理员）/ `admin`（装到 Program Files） |
| `-Arch` | 默认 `x64compatible`，可选 `x86compatible` |
| `-MinWindowsVersion` | 默认 `10.0.17763`（Win10 1809） |
| `-OutputDir` / `-OutputName` | 输出目录（默认 `installer\Output`）/ 文件名（默认 `<AppName>-Setup-<版本>`） |
| `-Exclude` | [Files] 排除的通配符，默认 `*.pdb,*.ilk,*.exp,*.lib` |
| `-PreserveUserData` | 卸载时**不**清理安装目录下的 `*.ini` / `*.log` |
| `-KeepIss` | 把生成的 `.iss` 留在输出目录，方便检查或手工微调 |
| `-OpenOutput` | 编译完打开输出目录 |
| `-ShowCompilerOutput` | 打印 ISCC 完整日志（默认只输出结果和错误） |
| `-Iscc` | 指定 `ISCC.exe` 路径。默认自动从注册表/默认安装位置/$PATH 里找 |

## 生成的安装包长什么样

- 按当前用户安装到 `%LOCALAPPDATA%\Programs\<AppName>`，**不需要管理员权限**；
  如果程序把配置写在 exe 同级（比如用 `QSettings::setPath(..., applicationDirPath())`），
  必须用 `lowest`，否则装进 Program Files 后写不了配置。
- 简体中文 + 英文双语界面（`ShowLanguageDialog=auto`，跟随系统语言）。
- 开始菜单快捷方式（+ 可选桌面图标），图标用 `-IconFile`。
- 控制面板里的卸载项可正常卸载，卸载时清掉 `{app}\*.ini`、`{app}\*.log`
  （`-PreserveUserData` 可关闭）。
- lzma2 高压缩；Qt 部署目录 86 MB → 安装包约 24 MB。
- 安装完成后可选直接启动程序（静默安装不启动）。

## 依赖

- Windows PowerShell 5.1 或 PowerShell 7+（系统自带即可）。
- [Inno Setup 6](https://jrsoftware.org/isdl.php)，缺失时可一键装：

  ```powershell
  winget install --id JRSoftware.InnoSetup -e
  ```

- 中文界面需要 `ChineseSimplified.isl`（仓库里已带；缺失时脚本会提示并只出英文界面）。

## 说明

- 只识别**顶层**的 exe 作主程序，子目录里的 exe（例如 Qt 插件、辅助工具）不受影响。
- 源目录里如果有 `.git`、`*.pdb` 之类不想进包的内容，用 `-Exclude` 加通配符。
- 路径里若含 `#` `{` `}` 或引号，脚本会直接报错——这些字符会破坏 Inno 脚本语法。
- 生成的 `.iss` 每次都是**全新重写**的，不要手工改 `Output\` 里的那一份；要定制请改
  `Build-Installer.ps1` 里的生成逻辑，或用 `-KeepIss` 拿到副本后手工编译。
