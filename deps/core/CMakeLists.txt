cmake_minimum_required(VERSION 3.12)
project(core)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 0)
set(VERSION_PATCH 1)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_PREFIX_PATH E:/Env/QT/6.2.4/msvc2019_64)
set(CMAKE_INCLUDE_CURRENT_DIR ON) # 添加build目录

include(../../cmake/windeployqt.cmake)

# 检查qt
find_package(QT NAMES Qt6 Qt5 COMPONENTS Sql Qml Core Gui Quick QuickControls2 QuickWidgets Widgets Network SerialPort LinguistTools OpenGLWidgets Concurrent REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Sql Qml Core Gui Quick QuickControls2 QuickWidgets Widgets Network SerialPort LinguistTools Concurrent REQUIRED)
add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)

set(my_sources_tree1
    ./include/lib_export.h
    ./include/thread_pool.h
    ./include/singleton.h
    ./include/data_wrapper.h

    ./src/thread_pool.cpp
    ./src/singleton.cpp
    ./src/data_wrapper.cpp

    # qt项目添加国际化文件依赖
    )

list(APPEND my_defines ${OPENCV_DEFINITION} MYLIBARAY_EXPORTS SPDLOG_COMPILED_LIB )

# 添加外部文件依赖
FILE(GLOB_RECURSE my_dep_headers 
    LIST_DIRECTORIES false
    CONFIGURE_DEPENDS
    ../nlohmann/include/*.h*
    ../core/*.h*
    )

# my_sources_tree2
set(my_sources_tree2
    ${my_dep_headers}
)

set(PROJECT_SOURCES ${my_sources_tree1} ${my_sources_tree2})

# 添加附加包含目录
set(my_includes ../ ../spdlog/include/ ../spdlog/include/spdlog/)

# qt自动刷新moc和uic生成文件依赖
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

# compiler check
if(MSVC)
  list(APPEND my_cflags /wd4828 )
  if (MSVC_VERSION GREATER 1925)
		add_compile_options(/Zc:preprocessor /wd5105)
    else()
		add_compile_options(/experimental:preprocessor /wd5105)
	endif()
endif()

# 设置 my_sources_tree1 my_sources_tree2 目录到IDE中， 对应文件夹目录
if(MSVC)
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. FILES ${my_sources_tree1} ${my_sources_tree2})
endif()

# 依赖的所有文件都加入dll中 - 设置编译成动态库
add_library(core SHARED ${PROJECT_SOURCES})

# 添加编译宏定义参数
target_compile_definitions(core PUBLIC ${my_defines})

# 添加编译选项
target_compile_options(core PUBLIC ${my_cflags})

# 添加包含依赖头文件
target_include_directories(core PUBLIC ${my_includes})

# 添加需要链接的库文件依赖
target_link_directories(core PUBLIC ${my_libraries})

install(TARGETS core)
install(FILES ${my_headers} DESTINATION include)

# Use the widgets module from Qt 6 设置链接库
target_link_libraries(${PROJECT_NAME}
	PUBLIC
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
    Qt6::Concurrent
    log)


