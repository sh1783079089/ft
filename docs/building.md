# 构建与运行

## 1. 开发环境

| 项目 | 说明 |
|------|------|
| 操作系统 | Windows 10 / 11 |
| 编译器 | MinGW-w64 GCC |
| 构建工具 | CMake ≥ 3.20（可选 Ninja） |
| C 标准 | C11（gnu11，GNU 扩展的 C11） |
| 第三方依赖 | 无 |
| 平台限制 | **仅支持 Windows**。代码中使用了 `<conio.h>`（`_kbhit`、`_getch`）、`<direct.h>`（`_mkdir`、`_stat`）、`<windows.h>`（`Sleep`）等 Windows 专属 API，无法在 Linux / macOS 上直接编译运行 |

业务代码集中在 `src/` 目录下，共 5 个文件：

| 文件 | 职责 |
|------|------|
| `src/main.c` | 程序入口，主菜单循环 |
| `src/tracker.h` | 公共头文件：结构体定义、宏、函数声明 |
| `src/tracker.c` | 核心逻辑：提交、分支、标签、版本图、恢复、监控 |
| `src/storage.c` | 持久化存储：blob 读写、commit/branch/tag 的保存与加载 |
| `src/lib.c` | 工具函数：SHA-256 自实现、路径解析、输入校验 |

## 2. 环境要求

| 条件 | 是否必需 | 检查方法 |
|------|----------|----------|
| Windows 操作系统（Win10 / Win11） | **必需** | - |
| MinGW-w64 GCC | **必需** | 打开终端，执行 `gcc --version` |
| CMake ≥ 3.20 | 可选 | 打开终端，执行 `cmake --version`。若没有，可用方法一直接 gcc 编译 |
| Ninja | 可选 | CMake 默认需要。若没有，方法二中可用 `-G "MinGW Makefiles"` 替代 |

## 3. 编译方法

以下四种方法任选其一，**推荐方法一**，最简单。

### 方法一：直接用 gcc 编译（最简方式，推荐）

**前提**：MinGW-w64 GCC 已安装且 `gcc` 在系统 PATH 中。

在项目根目录打开终端（PowerShell 或 cmd），执行：

```powershell
gcc -std=c11 -O2 -o file_tracker.exe src/main.c src/tracker.c src/storage.c src/lib.c
```

参数说明：

- `-std=c11`：使用 C11 标准
- `-O2`：开启优化（Release 模式，体积小运行快；若想保留调试信息可改为 `-g`）
- `-o file_tracker.exe`：输出文件名

编译成功后，当前目录下生成 `file_tracker.exe`。

> **若没有 MinGW-w64**：可以安装 [MSYS2](https://www.msys2.org/)（安装后 `pacman -S mingw-w64-ucrt-x86_64-gcc`），或使用 [winlibs](https://winlibs.com/) 提供的免安装 MinGW-w64 压缩包，解压后将其 `bin` 目录加入系统 PATH 即可。

### 方法二：CMake + MinGW Makefiles（无需 Ninja）

**前提**：已安装 CMake 和 MinGW-w64 GCC。

```powershell
mkdir cmake-build-release
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -B cmake-build-release
cmake --build cmake-build-release
```

编译完成后，`file_tracker.exe` 位于 `cmake-build-release\` 目录下。

> 默认 CMake 会尝试使用 Ninja 生成器。若电脑上没有 Ninja，则需显式指定 `-G "MinGW Makefiles"` 使用 MinGW 的 make 工具。MinGW 安装时通常自带 `mingw32-make.exe`。

### 方法三：CMake + Ninja

**前提**：已安装 CMake、Ninja、MinGW-w64 GCC。

```powershell
mkdir cmake-build-release
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B cmake-build-release
cmake --build cmake-build-release
```

编译完成后，`file_tracker.exe` 位于 `cmake-build-release\` 目录下。

### 方法四：直接用 CLion 打开

1. 启动 CLion。
2. 菜单 → File → Open → 选择项目根目录（包含 `CMakeLists.txt` 的目录）。
3. CLion 会自动检测 CMake 配置并加载项目。
4. 点击右上角锤子图标（Build）或按 `Ctrl+F9` 构建。
5. 构建完成后，`file_tracker.exe` 在 `cmake-build-debug\` 目录下。如需 Release 版本，在顶部工具栏将构建类型从 Debug 切换为 Release 后再构建。

## 4. 运行程序

### 启动方式

**必须在终端（命令行）中运行**，不要直接双击 `file_tracker.exe`（否则程序运行完就关掉了）。

- PowerShell：在 exe 所在目录执行 `.\file_tracker.exe`
- cmd：在 exe 所在目录执行 `file_tracker.exe`

### 关于字符编码

程序启动时会自动执行 `chcp 65001` 将终端编码切换为 UTF-8，以正确显示中文。若中文显示为乱码，可能原因及解决：

| 现象 | 解决方式 |
|------|----------|
| Python 风格乱码（如 `鍙樻洿`） | 终端编码未切换成功，手动在终端执行 `chcp 65001` 后再运行 |
| 方框或问号 | 终端字体不支持中文字符，切换为**新宋体**或 **SimSun**（cmd 属性中设置）；或使用 **Windows Terminal**（Microsoft Store 可下载，天然支持 UTF-8） |

推荐使用 **Windows Terminal**：右键文件夹 → “在 Windows Terminal 中打开”，然后运行 `.\file_tracker.exe`。

## 5. CMakeLists.txt 说明

```cmake
cmake_minimum_required(VERSION 3.20)
project(file_tracker C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(file_tracker
        src/main.c
        src/tracker.c
        src/storage.c
        src/lib.c
)
```

配置非常简单，仅需 C11 标准和 4 个源文件，无外部依赖。
