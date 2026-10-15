set(CPACK_PACKAGE_NAME "QTdemo")
set(CPACK_PACKAGE_VENDOR "QTdemo Team")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Industrial data workbench - simulated Widgets/QML prototype")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-windows-x64")
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/dist")
set(CPACK_GENERATOR "IFW;ZIP")
set(CPACK_IFW_PACKAGE_TITLE "QTdemo Setup")
set(CPACK_IFW_PACKAGE_NAME "com.example.qtdemo")
set(CPACK_IFW_PACKAGE_GROUP "Application")
set(CPACK_IFW_TARGET_DIRECTORY "@HomeDir@/Applications/QTdemo")
set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY "QTdemo")
set(CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME "Uninstall")
set(CPACK_IFW_PACKAGE_RUN_PROGRAM "@TargetDir@/bin/QTdemo.exe")
include(CPack)
include(CPackIFW)
cpack_ifw_configure_component_group(Application
    NAME "com.example.qtdemo"
    DISPLAY_NAME "QTdemo"
    DESCRIPTION "Application and Qt runtime"
    SCRIPT "${PROJECT_SOURCE_DIR}/packaging/installscript.qs"
)
