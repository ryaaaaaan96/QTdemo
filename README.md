# QTdemo

Windows x64 桌面工程模板：Qt Widgets + LLVM-MinGW Clang + CMake + Ninja，使用 VS Code / clangd 开发，支持测试、EXE 安装包和 ZIP 便携包。

## 环境要求

- Qt 6.8+ LLVM-MinGW x64 SDK，以及与其匹配的 LLVM-MinGW 工具链。
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
├── docs/                # 架构约定
├── packaging/           # 安装器脚本
├── resources/           # 图标、样式、翻译
├── scripts/             # 环境加载与开发命令
├── src/
│   ├── app/             # 启动入口
│   ├── core/            # 可测试业务规则
│   └── ui/              # 窗口与控件
├── tests/               # 业务测试与启动检查
├── CMakeLists.txt
├── CMakePresets.json    # 共享预设，通过环境变量查找工具
├── tools.example.json   # 可提交的配置示例
└── tools.local.json     # 本机配置，不提交
```

build/、stage/、dist/ 分别为编译、部署和发行输出，不提交。依赖方向为 app → ui → core，外部访问按需添加 services，见 [架构说明](docs/architecture.md)。

## 打包与发布

- dist/QTdemo-0.1.0-windows-x64.exe：离线安装器，默认安装到当前用户 Applications/QTdemo，包含开始菜单快捷方式和卸载工具。
- dist/QTdemo-0.1.0-windows-x64.zip：解压后运行 bin/QTdemo.exe。

Qt 部署脚本收集 Qt DLL 和平台插件，额外 install 规则收集 LLVM-MinGW 的 libc++.dll、libunwind.dll、libwinpthread-1.dll。新增第三方库或资源需补充部署规则。改用 QML 时需要 QML 专用部署流程。

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

本机 Debug/Release 构建、业务测试与 GUI 冒烟检查已通过；发行 ZIP 已验证移除 Qt PATH 后可启动。clangd 定义查询已验证可从 UI 调用定位到业务源文件。

尚未完成干净 Windows 上的安装、升级、卸载验证或 VS Code 交互式断点验证。当前环境缺少可选 Vulkan headers 和部分 Direct3D 编译器 DLL；Widgets 示例已通过，需要对应渲染能力时再补齐。

## 官方参考

- [CMake Presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html)
- [clangd 配置](https://clangd.llvm.org/config)
- [Qt 6.8 部署脚本](https://doc.qt.io/qt-6.8/qt-generate-deploy-app-script.html)
- [CPack IFW](https://cmake.org/cmake/help/latest/cpack_gen/ifw.html)
