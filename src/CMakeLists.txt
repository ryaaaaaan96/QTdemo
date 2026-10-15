add_library(app_core STATIC
    core/types.h core/types.cpp core/protocol_session.h
    services/config/config_repository.h services/config/config_repository.cpp
    services/data/data_hub.h services/data/data_hub.cpp services/data/sample_mailbox.h
    services/runtime/runtime_manager.h services/runtime/runtime_manager.cpp
    services/runtime/connection_worker.h services/runtime/connection_worker.cpp
    services/runtime/application_service.h services/runtime/application_service.cpp
    drivers/simulated/simulated_session.h drivers/simulated/simulated_session.cpp
)
target_include_directories(app_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_core PUBLIC Qt6::Core)

add_library(app_ui STATIC
    ui/mainwindow.h ui/mainwindow.cpp
    ui/models/point_table_model.h ui/models/point_table_model.cpp
    ui/qml/page_host.h ui/qml/page_host.cpp
)
target_link_libraries(app_ui PUBLIC app_core Qt6::Widgets Qt6::QuickWidgets)
qt_add_executable(QTdemo app/main.cpp)
target_link_libraries(QTdemo PRIVATE app_ui)
set_target_properties(QTdemo PROPERTIES WIN32_EXECUTABLE TRUE)
target_compile_options(app_core PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(QTdemo PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(app_ui PRIVATE -Wall -Wextra -Wpedantic)
set_source_files_properties(ui/qml/Dashboard.qml PROPERTIES QT_RESOURCE_ALIAS Dashboard.qml)
qt_add_qml_module(QTdemo
    URI PlatformUi VERSION 1.0 RESOURCE_PREFIX /qt/qml
    QML_FILES ui/qml/Dashboard.qml
)
qt_add_resources(QTdemo example_config
    PREFIX /config
    BASE "${PROJECT_SOURCE_DIR}/config/examples"
    FILES "${PROJECT_SOURCE_DIR}/config/examples/simulated.json"
)
install(FILES "${PROJECT_SOURCE_DIR}/config/examples/simulated.json" DESTINATION examples)
install(FILES ui/qml/Dashboard.qml DESTINATION examples)
