#include <assert.h>
#include <inttypes.h>
#include <imgui.h>
#include <GLEW/glew.h>
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <memory>
#include "../nfd/nfd.h"
#include <sys/stat.h>

#ifdef _WIN32
#  include <windows.h>
#  include <shellapi.h>
#endif

#include "../../server/TracyBadVersion.hpp"
#include "../../server/TracyFileRead.hpp"
#include "../../server/TracyImGui.hpp"
#include "../../server/TracyStorage.hpp"
#include "../../server/TracyView.hpp"
#include "../../server/TracyWorker.hpp"
#include "../../server/TracyVersion.hpp"
#include "../../server/IconsFontAwesome5.h"

#include "ImGui/imgui_freetype.h"
#include "Arimo.hpp"
#include "Cousine.hpp"
#include "FontAwesomeSolid.hpp"

static void OpenWebpage( const char* url )
{
#ifdef _WIN32
    ShellExecuteA( nullptr, nullptr, url, nullptr, nullptr, 0 );
#else
    char buf[1024];
    sprintf( buf, "xdg-open %s", url );
    system( buf );
#endif
}

static SDL_Window* window = nullptr;
static bool s_customTitle = false;
static void SetWindowTitleCallback( const char* title )
{
    assert( window );
    SDL_SetWindowTitle( window, title );
    s_customTitle = true;
}

int main( int argc, char** argv )
{
    std::unique_ptr<tracy::View> view;
    int badVer = 0;

    if( argc == 2 )
    {
        auto f = std::unique_ptr<tracy::FileRead>( tracy::FileRead::Open( argv[1] ) );
        if( f )
        {
            view = std::make_unique<tracy::View>( *f );
        }
    }

    char title[128];
    sprintf( title, "Urho3D Profiler %i.%i.%i", tracy::Version::Major, tracy::Version::Minor, tracy::Version::Patch );

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
    if( !window ) return 1;
    SDL_GLContext glcontext = SDL_GL_CreateContext(window);

    glewInit();

    float dpiScale = 1.f;
#ifdef _WIN32
    typedef UINT(*GDFS)(void);
    GDFS getDpiForSystem = nullptr;
    HMODULE dll = GetModuleHandleW(L"user32.dll");
    if (dll != INVALID_HANDLE_VALUE)
        getDpiForSystem = (GDFS)GetProcAddress(dll, "GetDpiForSystem");
    if (getDpiForSystem)
        dpiScale = getDpiForSystem() / 96.f;
#endif

    // Setup ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGuiIO& io = ImGui::GetIO();
    std::string iniFileName = tracy::GetSavePath( "imgui.ini" );
    io.IniFilename = iniFileName.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplSDL2_InitForOpenGL( window, glcontext );
    ImGui_ImplOpenGL3_Init();

    static const ImWchar rangesBasic[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x03BC, 0x03BC, // micro
        0,
    };
    static const ImWchar rangesIcons[] = {
        ICON_MIN_FA, ICON_MAX_FA,
        0
    };
    ImFontConfig configMerge;
    configMerge.MergeMode = true;

    io.Fonts->AddFontFromMemoryCompressedTTF( tracy::Arimo_compressed_data, tracy::Arimo_compressed_size, 15.0f * dpiScale, nullptr, rangesBasic );
    io.Fonts->AddFontFromMemoryCompressedTTF( tracy::FontAwesomeSolid_compressed_data, tracy::FontAwesomeSolid_compressed_size, 14.0f * dpiScale, &configMerge, rangesIcons );
    auto fixedWidth = io.Fonts->AddFontFromMemoryCompressedTTF( tracy::Cousine_compressed_data, tracy::Cousine_compressed_size, 15.0f * dpiScale );

    ImGuiFreeType::BuildFontAtlas( io.Fonts, ImGuiFreeType::LightHinting );

    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowBorderSize = 1.f * dpiScale;
    style.FrameBorderSize = 1.f * dpiScale;
    style.FrameRounding = 5.f * dpiScale;
    style.ScrollbarSize *= dpiScale;
    style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.11f, 0.11f, 0.08f, 0.94f );
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4( 1, 1, 1, 0.03f );

    ImVec4 clear_color = ImColor(114, 144, 154);

    char addr[1024] = { "127.0.0.1" };

    std::thread loadThread;

    // Main loop
    bool done = false;
    while (!done)
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
        }
        int display_w, display_h;
        SDL_GetWindowSize (window, &display_w, &display_h);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();

        if( !view )
        {
            if( s_customTitle )
            {
                s_customTitle = false;
                SDL_SetWindowTitle( window, title );
            }

            ImGui::Begin( "Tracy server", nullptr, ImGuiWindowFlags_AlwaysAutoResize );
            char buf[128];
            sprintf( buf, "Tracy %i.%i.%i", tracy::Version::Major, tracy::Version::Minor, tracy::Version::Patch );
            tracy::TextCentered( buf );
            if( ImGui::Button( ICON_FA_BOOK " User manual" ) )
            {
                OpenWebpage( "https://bitbucket.org/wolfpld/tracy/downloads/tracy.pdf" );
            }
            ImGui::SameLine();
            if( ImGui::Button( ICON_FA_GLOBE_AMERICAS " Homepage" ) )
            {
                OpenWebpage( "https://bitbucket.org/wolfpld/tracy" );
            }
            ImGui::SameLine();
            if( ImGui::Button( ICON_FA_VIDEO " Tutorial" ) )
            {
                OpenWebpage( "https://www.youtube.com/watch?v=fB5B46lbapc" );
            }
            ImGui::Separator();
            ImGui::Text( "Connect to client" );
            ImGui::InputText( "Address", addr, 1024 );
            if( ImGui::Button( ICON_FA_WIFI " Connect" ) && *addr && !loadThread.joinable() )
            {
                view = std::make_unique<tracy::View>( addr, fixedWidth, SetWindowTitleCallback );
            }
            ImGui::Separator();
            if( ImGui::Button( ICON_FA_FOLDER_OPEN " Open saved trace" ) && !loadThread.joinable() )
            {
                nfdchar_t* fn;
                auto res = NFD_OpenDialog( "tracy", nullptr, &fn );
                if( res == NFD_OKAY )
                {
                    try
                    {
                        auto f = std::shared_ptr<tracy::FileRead>( tracy::FileRead::Open( fn ) );
                        if( f )
                        {
                            loadThread = std::thread( [&view, f, &badVer, fixedWidth] {
                                try
                                {
                                    view = std::make_unique<tracy::View>( *f, fixedWidth, SetWindowTitleCallback );
                                }
                                catch( const tracy::UnsupportedVersion& e )
                                {
                                    badVer = e.version;
                                }
                            } );
                        }
                    }
                    catch( const tracy::NotTracyDump& e )
                    {
                        badVer = -1;
                    }
                }
            }

            if( badVer != 0 )
            {
                if( loadThread.joinable() ) { loadThread.join(); }
                tracy::BadVersion( badVer );
            }

            ImGui::End();
        }
        else
        {
            if( loadThread.joinable() ) loadThread.join();
            view->NotifyRootWindowSize( display_w, display_h );
            if( !view->Draw() )
            {
                view.reset();
            }
        }
        auto& progress = tracy::Worker::GetLoadProgress();
        auto totalProgress = progress.total.load( std::memory_order_relaxed );
        if( totalProgress != 0 )
        {
            ImGui::OpenPopup( "Loading trace..." );
        }
        if( ImGui::BeginPopupModal( "Loading trace...", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            tracy::TextCentered( ICON_FA_HOURGLASS_HALF );
            auto currProgress = progress.progress.load( std::memory_order_relaxed );
            if( totalProgress == 0 )
            {
                ImGui::CloseCurrentPopup();
                totalProgress = currProgress;
            }
            switch( currProgress )
            {
            case tracy::LoadProgress::Initialization:
                ImGui::Text( "Initialization..." );
                break;
            case tracy::LoadProgress::Locks:
                ImGui::Text( "Locks..." );
                break;
            case tracy::LoadProgress::Messages:
                ImGui::Text( "Messages..." );
                break;
            case tracy::LoadProgress::Zones:
                ImGui::Text( "CPU zones..." );
                break;
            case tracy::LoadProgress::GpuZones:
                ImGui::Text( "GPU zones..." );
                break;
            case tracy::LoadProgress::Plots:
                ImGui::Text( "Plots..." );
                break;
            case tracy::LoadProgress::Memory:
                ImGui::Text( "Memory..." );
                break;
            case tracy::LoadProgress::CallStacks:
                ImGui::Text( "Call stacks..." );
                break;
            default:
                assert( false );
                break;
            }
            ImGui::ProgressBar( float( currProgress ) / totalProgress, ImVec2( 200 * dpiScale, 0 ) );

            ImGui::Text( "Progress..." );
            auto subTotal = progress.subTotal.load( std::memory_order_relaxed );
            auto subProgress = progress.subProgress.load( std::memory_order_relaxed );
            if( subTotal == 0 )
            {
                ImGui::ProgressBar( 1.f, ImVec2( 200 * dpiScale, 0 ) );
            }
            else
            {
                ImGui::ProgressBar( float( subProgress ) / subTotal, ImVec2( 200 * dpiScale, 0 ) );
            }
            ImGui::EndPopup();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(glcontext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
