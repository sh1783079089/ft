# 变更日志

本项目所有值得记录的变更都会写入本文件。格式参考 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/)，版本号遵循[语义化版本](https://semver.org/lang/zh-CN/)。

## [0.1.0] - 2026-10-14

首个开源版本。

### Added

- 控制台菜单交互：打开/创建仓库、提交、历史、恢复、监控、分支、标签、提交图、删除/重置、退出。
- 自实现 SHA-256 内容摘要（`lib.c`，无第三方依赖）。
- 基于内容哈希的 blob 去重存储与引用计数（`BlobRef` 单链表）。
- 提交历史双向链表：`next` 维护全局时间顺序，`prev` 维护分支父关系。
- 单文件独立子仓库与 `.ft/index` 映射，支持同一工作目录多文件。
- 分支管理：创建、切换、删除，切换时恢复目标分支内容。
- 标签管理：为指定提交创建/删除标签并持久化。
- 文件监控：轮询 mtime 与哈希，内容真实变化时自动提交。
- ASCII 提交图可视化，展示分支轨道、当前版本与标签。
- 二进制安全的文件读写与文本元数据持久化（`HEAD`、`commits`、`branches`、`tags`、`blobrefs`）。
- 误操作防护：恢复/切换分支前检查未提交修改；非分支头禁止提交与监控。
- 输入与存储安全：拒绝包含分隔符 `|` 的说明/分支名/标签名。
- 项目文档：`README.md`、`CONTRIBUTING.md`、`docs/design.md`、`docs/usage.md`、`docs/building.md`、`docs/faq.md`、`docs/roadmap.md`。
- BSD 3-Clause 许可证。
- GitHub Actions：构建、格式检查（clang-format）与标签触发的发布工作流。
