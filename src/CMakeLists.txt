add_library(app_core STATIC core/greeting.cpp core/greeting.h)
target_include_directories(app_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_core PUBLIC Qt6::Core)

qt_add_executable(QTdemo app/main.cpp ui/mainwindow.cpp ui/mainwindow.h)
target_link_libraries(QTdemo PRIVATE app_core Qt6::Widgets)
set_target_properties(QTdemo PROPERTIES WIN32_EXECUTABLE TRUE)
target_compile_options(app_core PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(QTdemo PRIVATE -Wall -Wextra -Wpedantic)
