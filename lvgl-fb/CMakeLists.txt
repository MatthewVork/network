cmake_minimum_required(VERSION 3.10)
project(lvgl_chess_client)

set(CMAKE_C_STANDARD 11)

# --- 1. 定义路径 ---
set(LVGL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lvgl)
set(DRV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lv_drivers)
set(UI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/squi)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# --- 2. 包含目录 ---
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${LVGL_DIR}
    ${DRV_DIR}
    ${UI_DIR}
    ${SRC_DIR}
)

# --- 3. 配置宏定义 ---
add_definitions(-DUSE_SDL)
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)
add_definitions(-DLV_LVGL_H_INCLUDE_SIMPLE)

# --- 4. 查找 SDL2 ---
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# --- 5. 收集所有必要的源文件 ---
file(GLOB_RECURSE LV_SOURCES "${LVGL_DIR}/src/*.c") # LVGL 核心
file(GLOB_RECURSE UI_SOURCES "${UI_DIR}/*.c")     # UI 界面

# 【重点修复】：抓取 SDL 驱动目录下所有的 .c 文件
# 报错里缺的 mouse_handler, keyboard_handler 都在这个目录的其他 .c 里
file(GLOB SDL_DRV_SOURCES "${DRV_DIR}/sdl/*.c")

# 抓取你的业务代码
file(GLOB MY_SOURCES "${SRC_DIR}/*.c")

# --- 6. 生成可执行文件 ---
add_executable(chess_app 
    main.c 
    ${MY_SOURCES}
    ${LV_SOURCES} 
    ${UI_SOURCES}
    ${SDL_DRV_SOURCES} # 这一行会包含 sdl.c, sdl_common.c, mouse.c 等所有驱动
)

# --- 7. 链接库 ---
target_link_libraries(chess_app ${SDL2_LIBRARIES} m pthread)