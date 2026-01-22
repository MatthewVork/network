cmake_minimum_required(VERSION 3.10)
project(lvgl_chess_client)

set(CMAKE_C_STANDARD 11)

# --- 1. 定义路径 ---
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
set(DRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers)
set(UI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/squi)

# --- 2. 包含目录 ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${LVGL_DIR}
    ${DRV_DIR}
    ${UI_DIR}
)

# --- 3. 配置宏定义 ---
add_definitions(-DUSE_SDL)
# 解决 lv_drv_conf.h 相对路径找不到的问题
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)

# --- 4. 查找 SDL2 ---
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# --- 5. 收集所有必要的源文件 ---
# 收集 LVGL 核心源码
file(GLOB_RECURSE LV_SOURCES "${LVGL_DIR}/src/*.c")

# 收集 SLS 导出的 UI 源码
file(GLOB_RECURSE UI_SOURCES "${UI_DIR}/*.c")

# 【重要】收集 SDL 驱动目录下所有的 .c 文件（解决鼠标、键盘、事件处理函数丢失）
file(GLOB DRV_SOURCES "${DRV_DIR}/sdl/*.c")

# --- 6. 生成程序 ---
add_executable(chess_app 
    main.c 
    ${LV_SOURCES} 
    ${DRV_SOURCES} 
    ${UI_SOURCES}
)

# --- 7. 链接库 ---
target_link_libraries(chess_app ${SDL2_LIBRARIES} m pthread)