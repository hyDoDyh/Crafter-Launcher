# Crafter Launcher

A lightweight, open-source launcher for **Minecraft: Java Edition**, built with **Qt 6 / C++17**.

Crafter Launcher can sign in with a Microsoft account, install game versions and their
libraries/assets, manage multiple launch configurations, and start the game with the
arguments required by modern Minecraft versions.

> 中文简介见文末 · [Privacy Policy](PRIVACY.md)

## Features

- **Microsoft account sign-in** using the OAuth 2.0 device code flow — no embedded browser
  and no password is ever handled by the launcher.
- **Xbox Live / XSTS / Minecraft authentication chain**, with the refresh token persisted
  locally so you stay signed in between sessions.
- **Offline mode** with a configurable player name and UUID.
- **Version browser and installer** — fetches the official version manifest and downloads
  the client jar, libraries and assets into a game directory of your choice.
- **Launch configuration management** — save, load and edit account details per version.
- **Game directory selection**, launch-argument preview and live launcher log.
- **Animated main menu** driven by `theme.json` (positions, sizes, easing and styles).
- Chinese UI translation (`Crafter_zh_CN.ts`).

## Requirements

- Windows (developed and tested on Windows 11)
- Qt **6** with the `widgets`, `network` and `concurrent` modules
  (developed and tested with Qt 6.10.1, MinGW 64-bit)
- A C++17 compiler (MinGW or MSVC)
- QuaZip is bundled in `third_party/quazip`

## Build

```bash
git clone https://github.com/hyDoDyh/Crafter-Launcher.git
cd Crafter-Launcher
mkdir build
cd build
qmake ../Crafter.pro
mingw32-make -j8      # use nmake with MSVC
```

The executable is written to `debug/Crafter.exe` (or `release/Crafter.exe`). The build
copies `libquazip1-qt6.dll` and `theme.json` next to the executable automatically.

## Run

Qt's runtime DLLs must be discoverable by the executable. Either add Qt's `bin` directory
to `PATH`, or deploy the dependencies once:

```bat
windeployqt debug\Crafter.exe
debug\Crafter.exe
```

## Configuration and local data

- Settings are stored in an INI file in the application directory (`QSettings`).
- The Microsoft refresh token is stored under `Auth/Microsoft/RefreshToken` and the last
  account name under `Auth/Microsoft/LastName`. Access tokens are kept in memory only.
- Game files, libraries and assets are stored in the game directory selected in the
  launcher.
- The OAuth client ID can be overridden with the `CRAFTER_MS_CLIENT_ID` environment
  variable or the `Auth/Microsoft/ClientId` setting. The built-in client ID is a public
  OAuth client identifier, not a secret.

## Authentication notes

Minecraft's authentication service only accepts OAuth clients that have been approved by
Mojang. If the client ID is not on the allowlist, the Xbox/XSTS steps succeed but
`POST https://api.minecraftservices.com/authentication/login_with_xbox` returns
`HTTP 403 Invalid app registration`. This is a server-side registration check, not a
launcher bug.

## Privacy

Crafter Launcher collects no telemetry and sends no account data to the developer. See
[PRIVACY.md](PRIVACY.md) for details on what is processed during sign-in and how it is
stored.

## Project structure

| Path | Purpose |
| --- | --- |
| `mainwindow.*` | Main menu, page switching and menu animations |
| `launcherpage.*` | Launch configurations, account panel, launch button |
| `downloadpage.*` | Version browser and installer UI |
| `launcher.*`, `launcharguments.*` | Builds the Java command line and starts the game |
| `microsoftauth.*` | Device code flow and Xbox Live/XSTS/Minecraft auth chain |
| `logindialog.*` | Microsoft sign-in dialog |
| `download.*`, `downloadthread.*` | HTTP downloads and installation tasks |
| `minecraftversion.*`, `library.*`, `minecraft.*` | Version/library models and UI |
| `theme.json` | Main-menu animation and style configuration |
| `third_party/quazip` | ZIP extraction used when installing versions |

## Contributing

Issues and pull requests are welcome. Please keep changes focused and describe how they
were tested.

## 中文简介

Crafter Launcher 是一个基于 Qt 6 / C++17 的开源 Minecraft Java 版启动器，支持微软正版账号
登录（OAuth 2.0 设备代码流）、离线启动、版本下载与安装、启动配置管理以及自定义游戏目录。

构建方式：用 Qt 6 的 qmake 打开 `Crafter.pro`，或在 `build` 目录执行 `qmake ../Crafter.pro`
后运行 `mingw32-make`。运行前请确保 Qt 的 DLL 在 `PATH` 中，或执行 `windeployqt`。

本启动器不收集任何遥测数据，账号令牌仅保存在本机；详情见 [PRIVACY.md](PRIVACY.md)。
