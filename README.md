# QTdemo

Windows x64 工业数据工作台原型：Qt Widgets + QML、LLVM-MinGW Clang、CMake、Ninja，使用 VS Code / clangd 开发。当前 v0.2.0 实现模拟采集骨架，支持测试、EXE 安装包和 ZIP 便携包。

## 设计文档

项目已进入第一轮实现，仍只维护一篇设计文档；已实现范围与后续目标分开记录。

| 文档 | 内容 | 状态 |
| --- | --- | --- |
| [多协议工业数据采集与监控平台设计稿](docs/industrial-platform-design.md) | 需求与架构、类与接口、线程生命周期、配置与 QML 更新、开发路线与待决策事项 | v1.3，含首个模拟运行里程碑 |

本轮建议先阅读设计稿第 4～6 节（对象、接口与线程），再看第 8～9 节（配置和界面协作）。

## 当前可运行功能

- 两个模拟连接各自使用 I/O 线程；第一个连接运行 a1/b1，第二个运行 a2，a1/a2 使用同一协议实现和独立状态。
- Widgets 点位表、可写点命令、连接/会话停止、失效值显示，以及有界采样缓存和命令队列。
- 多个本地 QML 页面共享只读点位模型；手动重载入口文件，语法或导入加载失败保留旧页面。
- 配置校验拒绝未知协议、硬件后端、重复 ID、无效引用和越界数值，不把真实设备配置偷偷转成模拟数据。

真实 Modbus/CAN/串口、自动重连、RTU 转发、趋势与文件日志、配置差异应用、完整 QML 页面包版本/状态恢复尚未实现。文档中的未来 Modbus/CAN 示例不是当前可执行配置。

运行 `python scripts/dev.py run` 会加载内置模拟配置。选中标记“可写”的设定值行后可发送命令；先停止全部连接才能打开新配置。停止 a1 后，b1 和 a2 继续采集；恢复 a1 时重启其所在连接。

当前配置示例为 [simulated.json](config/examples/simulated.json)，也可以执行：

```powershell
python scripts/dev.py run --config config/examples/simulated.json
```

本阶段只支持 simulated 后端、simulated_counter/simulated_constant 两种模拟协议及双精度数值。point 的 initial/step 为模拟参数，access=read_write 表示可写。保护上限暂为 32 连接、128 会话和 4096 点，不代表设备容量指标；每连接最多 32 个在途/排队命令，全局最多 64 个，超限明确拒绝。

可通过“新增 QML 页面”打开 [Dashboard.qml](src/ui/qml/Dashboard.qml) 的本地副本，再修改入口文件并重载。当前接口只暴露 pointModel；包含独立组件/模块的复杂页面应使用不同版本目录，完整依赖包更新尚待实现。

## 环境要求

- Qt 6.8+ LLVM-MinGW x64 SDK（Widgets、Qml、Quick、QuickWidgets、Test），以及匹配的 LLVM-MinGW 工具链。
- Python 3.10+（只使用标准库，无需 pip 安装依赖）。
- CMake 3.25+、Ninja；生成 EXE 安装包另需 Qt Installer Framework。
- VS Code 推荐扩展：CMake Tools、clangd、CodeLLDB。
- 提交代码需要 Git。

已验证组合为 Qt 6.8.3、Clang 17.0.6、CMake 3.30.5、IFW 4.11。其他组合需要自行验证。不要混用 MSVC clang-cl 或普通 MinGW GCC 与当前 Qt SDK 的 ABI。

## 首次配置

在仓库根目录复制模板，并填写本机工具位置：

```powershell
Copy-Item tools.example.json tools.local.json
```

| 配置 | 含义 |
| --- | --- |
| QT_ROOT | Qt kit 目录，包含 bin/qtpaths6.exe |
| LLVM_ROOT | LLVM-MinGW 目录，包含 bin/clang++.exe 和 bin/clangd.exe |
| CMAKE_ROOT | CMake 目录，包含 bin/cmake.exe |
| NINJA_ROOT | 包含 ninja.exe 的目录 |
| IFW_ROOT | IFW 目录，包含 bin/binarycreator.exe，仅 EXE 打包必需 |
| IFW_VERSION | IFW 版本，可留空自动探测；旧 CMake 配合新 IFW 建议填写 |

示例路径不要求与本机相同。tools.local.json 被 Git 忽略。也可不创建文件，直接设置上述环境变量，适合 CI；本地文件的非空字段优先。

## VS Code 开发

插件也需要工具环境变量。首次使用或修改路径后，**关闭所有 VS Code 窗口**，从外部 PowerShell 在仓库根目录执行：

```powershell
python scripts/dev.py shell
code .
```

shell 命令启动一个带 Qt 工具环境的子终端，输入 exit 返回原终端，不修改系统或用户环境变量。Python 子进程不能反向修改父终端环境，因此不再使用点号加载脚本。VS Code 已运行时，新窗口可能复用旧进程环境，因此仅“重新加载窗口”不一定有效。

1. 安装工作区推荐扩展。
2. 执行 CMake: Select Configure Preset，选择 debug。
3. Ctrl+Shift+B 编译，F5 调试。
4. “运行任务”可执行 Test Debug 或 Package Release。

clangd 负责跳转、补全、诊断和格式化；工作区禁用了 Microsoft C/C++ 的对应语言服务，CodeLLDB 负责调试。无需全局卸载 C/C++。

根目录 .clangd 指向 build/debug/compile_commands.json，应先配置 Debug。更换开发构建目录时同步修改 .clangd。首次解析需稍等；必要时执行 clangd: Restart language server，然后按 F12 转到定义，在“输出 → clangd”检查日志。无需手动维护 Qt includePath。

## 开发命令

以下脚本自动读取本地配置，无需预先加载环境：

```powershell
python scripts/dev.py build
python scripts/dev.py test
python scripts/dev.py run
python scripts/dev.py install --preset release
python scripts/dev.py package
```

进入 python scripts/dev.py shell 子终端后也可直接使用预设：

```powershell
cmake --preset release
cmake --build --preset release
ctest --preset release
cpack --preset installer
cpack --preset portable
```

CPack 不会先编译。dev.py package 自动完成 Release 构建、测试和两种打包。

## 目录

```text
QTdemo/
├── .vscode/             # 共享扩展、构建、调试配置
├── cmake/               # CPack 配置
├── config/examples/     # 当前可执行的模拟配置
├── docs/                # 架构约定
├── packaging/           # 安装器脚本
├── resources/           # 图标、样式、翻译
├── scripts/             # 环境加载与开发命令
├── src/
│   ├── app/             # 启动入口
│   ├── core/            # 值类型与协议会话接口
│   ├── drivers/         # 当前为独立实例的模拟协议
│   ├── services/        # 配置、数据缓存、运行管理、命令入口
│   └── ui/              # Widgets、展示模型、QML 页面宿主
├── tests/               # 业务测试与启动检查
├── CMakeLists.txt
├── CMakePresets.json    # 共享预设，通过环境变量查找工具
├── tools.example.json   # 可提交的配置示例
└── tools.local.json     # 本机配置，不提交
```

build/、stage/、dist/ 分别为编译、部署和发行输出，不提交。方案与实现进度统一维护在 [平台设计稿](docs/industrial-platform-design.md)。

## 打包与发布

- dist/QTdemo-0.2.0-windows-x64.exe：离线安装器，默认安装到当前用户 Applications/QTdemo，包含开始菜单快捷方式和卸载工具。
- dist/QTdemo-0.2.0-windows-x64.zip：解压后运行 bin/QTdemo.exe；examples/ 包含模拟配置及 QML 示例。

已使用 Qt QML 专用部署脚本收集导入模块、Qt DLL 和平台插件，额外 install 规则收集 LLVM-MinGW 的 libc++.dll、libunwind.dll、libwinpthread-1.dll。外部页面新增导入模块时仍需补齐部署依赖。

正式发布前补齐产品名称、图标、日志配置、依赖许可材料、代码签名和升级策略。本模板尚未指定仓库开源许可证；发布者应根据自己的授权意图选择 LICENSE。

## Git 提交约定

提交源码、文档、CMake 配置、.vscode/、.clangd、格式配置和 tools.example.json。真实本机路径只放 tools.local.json。

忽略本地工具配置、CMakeUserPresets.json、构建缓存、编译数据库、发行产物、环境秘密及签名私钥。二进制产物可上传 GitHub Releases。

首次推送示例，请替换仓库地址：

```powershell
git init -b main
git add .
git diff --cached --stat
git diff --cached
git commit -m "Initialize Qt desktop application template"
git remote add origin https://github.com/YOUR_ACCOUNT/YOUR_REPOSITORY.git
git push -u origin main
```

如果之前已追踪本地配置，新增 .gitignore 不会取消追踪，需用 git rm --cached 单独移出索引。

## 验证范围

验证包括配置错误、有界缓冲、多连接/会话隔离、写入超时不重放、退出结算、旧代次丢弃、QML 重载失败保留旧页及 GUI 启动退出。测试通过不代表真实 Modbus/CAN 已实现或已验收。运行结果以本次构建的 CTest 输出为准。

2026-10-07：v0.2.0 的 Debug/Release 均通过 core_tests、page_tests、gui_smoke 三组检查；IFW/ZIP 已生成。ZIP 解压后仅保留 System32 PATH、禁用 QML 磁盘缓存，已验证示例配置、采样、QML 显示和正常退出；本机软件渲染及默认 Windows 渲染均已检查。

尚未完成干净 Windows 上的安装、升级、卸载验证或 VS Code 交互式断点验证。自动 GUI 测试采用软件渲染；硬件渲染与不同显卡需要单独验证。

## 官方参考

- [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [clangd 配置](https://clangd.llvm.org/config)
- [Qt 6.8 QML 部署脚本](https://doc.qt.io/qt-6.8/qt-generate-deploy-qml-app-script.html)
- [CPack IFW](https://cmake.org/cmake/help/latest/cpack_gen/ifw.html)
