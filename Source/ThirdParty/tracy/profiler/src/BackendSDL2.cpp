#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"
#ifdef __EMSCRIPTEN__
#  include <GLES2/gl2.h>
#  include <emscripten/html5.h>
#else
#  include "imgui_impl_opengl3_loader.h"
#endif

#include <chrono>
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

#include "profiler/TracyConfig.hpp"
#include "profiler/TracyImGui.hpp"

#include "Backend.hpp"
#include "RunQueue.hpp"


static SDL_Window* s_window;
static std::function<void()> s_redraw;
static RunQueue* s_mainThreadTasks;
static WindowPosition* s_winPos;
static bool s_iconified;
static bool s_done = false;
static SDL_GLContext gl_context = nullptr;

extern tracy::Config s_config;


Backend::Backend( const char* title, const std::function<void()>& redraw, const std::function<void(float)>& scaleChanged, const std::function<int(void)>& isBusy, RunQueue* mainThreadTasks )
{
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return;
    }

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    s_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, m_winPos.w, m_winPos.h, window_flags);
    SDL_SetWindowPosition(s_window, m_winPos.x, m_winPos.y);
    gl_context = SDL_GL_CreateContext(s_window);
    SDL_GL_MakeCurrent(s_window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Platform/Renderer bindings
    ImGui_ImplSDL2_InitForOpenGL(s_window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    s_redraw = redraw;
    s_mainThreadTasks = mainThreadTasks;
    s_winPos = &m_winPos;
    s_iconified = false;
}

Backend::~Backend()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}

void Backend::Show()
{
    SDL_ShowWindow( s_window );
}

void Backend::Run()
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    while (!s_done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                s_done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(s_window))
                s_done = true;
        }

        s_redraw();
        bool has_focus = SDL_GetWindowFlags(s_window) & SDL_WINDOW_INPUT_FOCUS;
        if( s_config.focusLostLimit && !has_focus ) std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
        s_mainThreadTasks->Run();
    }
}

void Backend::Attention()
{
#if GLFW_VERSION_MAJOR > 3 || ( GLFW_VERSION_MAJOR == 3 && GLFW_VERSION_MINOR >= 3 )
    if( !glfwGetWindowAttrib( s_window, GLFW_FOCUSED ) )
    {
        glfwRequestWindowAttention( s_window );
    }
#endif
}

void Backend::NewFrame( int& w, int& h )
{
    SDL_GetWindowSize( s_window, &w, &h );
    m_w = w;
    m_h = h;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
}

void Backend::EndFrame()
{
    const ImVec4 clear_color = ImColor( 20, 20, 17 );

    ImGui::Render();
    glViewport( 0, 0, m_w, m_h );
    glClearColor( clear_color.x, clear_color.y, clear_color.z, clear_color.w );
    glClear( GL_COLOR_BUFFER_BIT );
    ImGui_ImplOpenGL3_RenderDrawData( ImGui::GetDrawData() );

    SDL_GL_SwapWindow(s_window);
}

void Backend::SetIcon( uint8_t* data, int w, int h )
{
    // TODO: SDL_SetWindowIcon(s_window, ...)
}

void Backend::SetTitle( const char* title )
{
    SDL_SetWindowTitle( s_window, title );
}

float Backend::GetDpiScale()
{
#ifdef __EMSCRIPTEN__
    return EM_ASM_DOUBLE( { return window.devicePixelRatio; } );
#else
    float dpi = 0;
    SDL_GetDisplayDPI(0, &dpi, nullptr, nullptr);
    return dpi / 96.0f;
#endif
    return 1;
}

#ifdef __EMSCRIPTEN__
extern "C" int nativeResize( int width, int height )
{
    SDL_SetWindowSize( s_window, width, height );
    return 0;
}
#endif
