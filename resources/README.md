存放 icons/、images/、styles/ 和翻译资源。使用 qt_add_resources 将资源编译到程序中，避免依赖工作目录。需要 Designer 时，将 .ui 文件放入 src/ui，并加入目标源文件列表；AUTOUIC 已启用。
