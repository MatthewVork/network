#include <SDL2/SDL.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // 1. 初始化 SDL 视频子系统
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL 无法初始化! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    // 2. 创建窗口
    SDL_Window* window = SDL_CreateWindow("C-SDL2 Collaborative Drawing", 
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                          800, 600, SDL_WINDOW_SHOWN);
    if (!window) return 1;

    // 3. 创建渲染器 (使用硬件加速)
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // 4. 变量准备
    int quit = 0;
    SDL_Event e;
    int last_x = 0, last_y = 0;
    int drawing = 0; // 鼠标是否按下

    // 设置背景为白色
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    // 5. 主循环
    while (!quit) {
        // 处理事件队列
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            } 
            else if (e.type == SDL_MOUSEBUTTONDOWN) {
                drawing = 1;
                last_x = e.button.x;
                last_y = e.button.y;
            } 
            else if (e.type == SDL_MOUSEBUTTONUP) {
                drawing = 0;
            } 
            else if (e.type == SDL_MOUSEMOTION && drawing) {
                // 当鼠标按下并移动时，画线
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 红色笔触
                SDL_RenderDrawLine(renderer, last_x, last_y, e.motion.x, e.motion.y);
                
                // 更新上一次坐标
                last_x = e.motion.x;
                last_y = e.motion.y;
                
                // 这里就是未来你要添加 send() 函数的地方
                // printf("Sending coordinates: %d, %d\n", last_x, last_y);
            }
        }
        // 刷新屏幕显示
        SDL_RenderPresent(renderer);
    }

    // 6. 清理退出
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}