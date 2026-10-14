# 文件变更监控与历史版本恢复工具

![CI](https://github.com/sh1783079089/ft/actions/workflows/ci.yml/badge.svg)

本项目是一个基于 C 语言的本地文件版本管理工具。程序采用控制台菜单交互方式，用户通过输入编号完成初始化、保存版本、查看历史、恢复版本、文件监控、分支管理、标签管理和提交图查看等操作。

## 项目特点

- 面向普通文件和二进制文件进行版本管理。
- 使用 SHA-256 对文件内容进行标识，单文件自实现，无第三方依赖。
- 相同内容只保存一份，减少磁盘占用。
- 使用双向链表维护提交历史，用单向链表维护 blob 引用计数。
- 支持多分支管理，可在分支间切换并恢复工作区内容。
- 支持为版本创建标签。
- 支持 ASCII 提交图可视化。
- 支持文件自动监控，内容变化时自动提交。
- 使用控制台菜单降低使用难度，无需命令行参数。

## 控制台菜单

```text
====================================
 文件追踪与历史版本恢复系统
====================================
1. 打开/创建文件仓库
2. 提交当前版本
3. 查看历史版本
4. 恢复指定版本
5. 监控文件变化
6. 分支管理
7. 标签管理
8. 查看提交图
9. 删除/重置仓库
0. 退出程序
请输入操作编号：
```

## 目录结构

```text
ft/
├── CMakeLists.txt
├── README.md
├── CONTRIBUTING.md
├── LICENSE
├── docs/
│   ├── design.md      # 架构与设计
│   ├── usage.md       # 使用说明
│   ├── building.md    # 构建与运行
│   ├── faq.md         # 常见问题与开发记录
│   └── assets/        # 文档插图
└── src/
    ├── main.c
    ├── tracker.h
    ├── tracker.c
    ├── lib.c
    └── storage.c
```

## 仓库存储结构

运行程序后在目标文件所在目录自动创建：

```text
工作目录/
├── .ft/
│   ├── index
│   └── repos/
│       └── <文件名>/
│           ├── HEAD
│           ├── commits
│           ├── branches
│           ├── tags
│           ├── blobrefs
│           └── blobs/
│               ├── <hash1>
│               └── <hash2>
```

## 快速开始

使用 CLion 或 CMake 编译项目，生成的可执行文件名称为 `file_tracker`：

```powershell
gcc -std=c11 -O2 -o file_tracker.exe src/main.c src/tracker.c src/storage.c src/lib.c
```

运行后，根据控制台菜单输入编号即可使用对应功能。详细步骤见 [`docs/building.md`](docs/building.md) 与 [`docs/usage.md`](docs/usage.md)。

## 文档

| 文档 | 内容 |
|------|------|
| [`docs/design.md`](docs/design.md) | 架构、数据结构、存储格式、提交流程与安全边界 |
| [`docs/usage.md`](docs/usage.md) | 菜单操作、功能说明与完整演示流程 |
| [`docs/building.md`](docs/building.md) | 环境要求、多种编译方式、运行与字符编码 |
| [`docs/faq.md`](docs/faq.md) | 常见问题排查与开发过程记录 |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | 协作规则、开发顺序与测试流程 |

## 平台限制

本项目**仅支持 Windows**。代码使用了 `<conio.h>`、`<direct.h>`、`<windows.h>` 等 Windows 专属 API，无法在 Linux / macOS 上直接编译运行。

## 来源与贡献者

项目第一版源于程序设计课程设计，由 **tick47** 与 **bdyxh** 共同开发；现由 **tick47** 维护并持续开发。

## 许可

本项目基于 [BSD 3-Clause License](LICENSE) 发布，版权归 FT Contributors 所有。
