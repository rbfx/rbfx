#include <algorithm>
#include <assert.h>
#include <atomic>
#include <chrono>
#include <inttypes.h>
#include <ImGui/imgui.h>
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "../nfd/nfd.h"
#include <sys/stat.h>
#include <locale.h>

#ifdef _WIN32
#  include <windows.h>
#  include <shellapi.h>
#endif

#define STBI_ONLY_PNG
#include "stb_image.h"

#include "../../common/TracyProtocol.hpp"
#include "../../server/tracy_pdqsort.h"
#include "../../server/tracy_robin_hood.h"
#include "../../server/TracyBadVersion.hpp"
#include "../../server/TracyFileHeader.hpp"
#include "../../server/TracyFileRead.hpp"
#include "../../server/TracyImGui.hpp"
#include "../../server/TracyMouse.hpp"
#include "../../server/TracyPrint.hpp"
#include "../../server/TracyStorage.hpp"
#include "../../server/TracyVersion.hpp"
#include "../../server/TracyView.hpp"
#include "../../server/TracyWeb.hpp"
#include "../../server/TracyWorker.hpp"
#include "../../server/TracyVersion.hpp"
#include "../../server/IconsFontAwesome5.h"
#include "../../client/tracy_rpmalloc.hpp"

#include "imgui_freetype.h"
#include "Arimo.hpp"
#include "Cousine.hpp"
#include "FontAwesomeSolid.hpp"
#include "icon.hpp"
#include "ResolvService.hpp"
//#include "NativeWindow.hpp"
//#include "HttpRequest.hpp"

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CommandLine.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Engine/Application.h>
#include <Urho3D/Engine/EngineDefs.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/SystemUI/SystemUI.h>

using namespace Urho3D;

static void OpenWebpage( const char* url )
{
#ifdef _WIN32
    ShellExecuteA( nullptr, nullptr, url, nullptr, nullptr, 0 );
#elif defined __APPLE__
    char buf[1024];
    sprintf( buf, "open %s", url );
    system( buf );
#else
    char buf[1024];
    sprintf( buf, "xdg-open %s", url );
    system( buf );
#endif
}

Urho3D::Context* gContext = nullptr;

static void SetWindowTitleCallback(const char* title)
{
    assert(gContext != nullptr);
    gContext->GetSubsystem<Graphics>()->SetWindowTitle(title);
}

std::vector<std::unordered_map<std::string, uint64_t>::const_iterator> RebuildConnectionHistory( const std::unordered_map<std::string, uint64_t>& connHistMap )
{
    std::vector<std::unordered_map<std::string, uint64_t>::const_iterator> ret;
    ret.reserve( connHistMap.size() );
    for( auto it = connHistMap.begin(); it != connHistMap.end(); ++it )
    {
        ret.emplace_back( it );
    }
    tracy::pdqsort_branchless( ret.begin(), ret.end(), []( const auto& lhs, const auto& rhs ) { return lhs->second > rhs->second; } );
    return ret;
}

struct ClientData
{
    int64_t time;
    uint32_t protocolVersion;
    int32_t activeTime;
    uint16_t port;
    std::string procName;
    std::string address;
};

enum class ViewShutdown { False, True, Join };

static tracy::unordered_flat_map<uint64_t, ClientData> clients;
static std::unique_ptr<tracy::View> view;
static tracy::BadVersionState badVer;
static uint16_t port = 8086;
static const char* connectTo = nullptr;
static char title[128];
static std::thread loadThread, updateThread, updateNotesThread;
static std::unique_ptr<tracy::UdpListen> broadcastListen;
static std::mutex resolvLock;
static tracy::unordered_flat_map<std::string, std::string> resolvMap;
static ResolvService resolv( port );
static ImFont* bigFont;
static ImFont* smallFont;
static ImFont* fixedWidth;
static char addr[1024] = { "127.0.0.1" };
static std::unordered_map<std::string, uint64_t> connHistMap;
static std::vector<std::unordered_map<std::string, uint64_t>::const_iterator> connHistVec;
static std::atomic<ViewShutdown> viewShutdown { ViewShutdown::False };
static double animTime = 0;
static float dpiScale = 1.f;
static ImGuiTextFilter addrFilter, portFilter, progFilter;
static std::thread::id mainThread;
static std::vector<std::function<void()>> mainThreadTasks;
static std::mutex mainThreadLock;
static uint32_t updateVersion = 0;
static bool showReleaseNotes = false;
static std::string releaseNotes;
static std::string readCapture;

void RunOnMainThread( std::function<void()> cb )
{
    if( std::this_thread::get_id() == mainThread )
    {
        cb();
    }
    else
    {
        std::lock_guard<std::mutex> lock( mainThreadLock );
        mainThreadTasks.emplace_back( cb );
    }
}

void* GetMainWindowNative()
{
    return gContext->GetSubsystem<Graphics>()->GetExternalWindow();
}

#ifdef _WIN32
namespace tracy
{
    bool DiscoveryAVX()
    {
#ifdef __AVX__
        return true;
#else
        return false;
#endif
    }

    bool DiscoveryAVX2()
    {
#ifdef __AVX2__
        return true;
#else
        return false;
#endif
    }
}
#endif

class ProfilerApplication : public Application
{
    URHO3D_OBJECT(ProfilerApplication, Application);
public:
    explicit ProfilerApplication(Context* context)
        : Application(context)
    {
        ::gContext = context;
    }

    void Setup() override
    {
#if __APPLE__
        // Static initialization appears to be broken on apple platforms and allocator has to be manually initialized.
        tracy::rpmalloc_initialize();
#endif
        engineParameters_[EP_RESOURCE_PATHS] = "CoreData";
        engineParameters_[EP_RESOURCE_PREFIX_PATHS] = ";..;../..";
        engineParameters_[EP_FULL_SCREEN] = false;

        // Engine starts listening for profiler application connections automatically. Since we link to the engine we
        // would take over profiler port and profile ourselves. Just terminate the profiler.
        tracy::GetProfiler().RequestShutdown();

        GetCommandLineParser().add_option("capture", readCapture);

#if _WIN32
        uint32_t regs[4]{};
        __cpuidex((int*) regs, 0, 0);
        const uint32_t maxLeaf = regs[0];
        bool cpuHasAVX = false;
        bool cpuHasAVX2 = false;
        if (maxLeaf >= 1)
        {
            __cpuidex((int*) regs, 1, 0);
            cpuHasAVX = (regs[2] & 0x10000000) != 0;
        }
        if (maxLeaf >= 7)
        {
            __cpuidex((int*) regs, 7, 0);
            cpuHasAVX2 = (regs[1] & 0x00000020) != 0;
        }

        if (tracy::DiscoveryAVX2() && !cpuHasAVX2)
        {
            ErrorExit(
                "This program is compiled with AVX2 instruction set, but your CPU doesn't support it. You must recompile with lower instruction set.\n\n"
                "In Visual Studio go to Project properties -> C/C++ -> Code Generation -> Enable Enhanced Instruction Set and select appropriate value for your CPU.");
        }
        if (tracy::DiscoveryAVX() && !cpuHasAVX)
        {
            ErrorExit(
                "This program is compiled with AVX instruction set, but your CPU doesn't support it. You must recompile with lower instruction set.\n\n"
                "In Visual Studio go to Project properties -> C/C++ -> Code Generation -> Enable Enhanced Instruction Set and select appropriate value for your CPU.");
        }
#endif

        engineParameters_[EP_FULL_SCREEN] = false;
        engineParameters_[EP_WINDOW_RESIZABLE] = true;
        engineParameters_[EP_SYSTEMUI_FLAGS] = ImGuiConfigFlags_DockingEnable;

        JSONFile config(context_);
        ea::string preferencesDir = context_->GetSubsystem<FileSystem>()->GetAppPreferencesDir("rbfx", "Profiler");
        if (!context_->GetSubsystem<FileSystem>()->DirExists(preferencesDir))
            context_->GetSubsystem<FileSystem>()->CreateDir(preferencesDir);
        if (config.LoadFile(preferencesDir + "Settings.json"))
        {
            const auto& root = config.GetRoot();
            if (root.IsObject())
            {
                engineParameters_[EP_WINDOW_POSITION_X] = root["x"].GetInt();
                engineParameters_[EP_WINDOW_POSITION_Y] = root["y"].GetInt();
                engineParameters_[EP_WINDOW_WIDTH] = root["width"].GetUInt();
                engineParameters_[EP_WINDOW_HEIGHT] = root["height"].GetUInt();
            }
        }

        strncpy(addr, "127.0.0.1", sizeof(addr));
    }

    void Start() override
    {
        context_->GetSubsystem<Graphics>()->SetWindowTitle(
            Format("Urho3D Profiler {}.{}.{}", tracy::Version::Major, tracy::Version::Minor, tracy::Version::Patch));
        context_->GetSubsystem<Input>()->SetMouseVisible(true);
        context_->GetSubsystem<Input>()->SetMouseMode(MM_FREE);

        ImGui::StyleColorsDark();
        float dpiScale = GetDPIScale();
        auto& style = ImGui::GetStyle();
        style.WindowBorderSize = 1.f * dpiScale;
        style.FrameBorderSize = 1.f * dpiScale;
        style.FrameRounding = 5.f * dpiScale;
        style.ScrollbarSize *= dpiScale;
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(1, 1, 1, 0.03f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.129f, 0.137f, 0.11f, 1.f);

        static const ImWchar rangesBasic[] = {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x03BC, 0x03BC, // micro
            0x03C3, 0x03C3, // small sigma
            0,
        };
        static const ImWchar rangesIcons[] = {
            ICON_MIN_FA, ICON_MAX_FA,
            0
        };

        context_->GetSubsystem<SystemUI>()->AddFontCompressed(tracy::Arimo_compressed_data, tracy::Arimo_compressed_size, "Arimo", rangesBasic, 15.0f);
        context_->GetSubsystem<SystemUI>()->AddFontCompressed(tracy::FontAwesomeSolid_compressed_data, tracy::FontAwesomeSolid_compressed_size, "FontAwesome", rangesIcons, 14.0f, true);
        fixedWidth = context_->GetSubsystem<SystemUI>()->AddFontCompressed(tracy::Cousine_compressed_data, tracy::Cousine_compressed_size, "Cousine", nullptr, 15.0f);
        bigFont = context_->GetSubsystem<SystemUI>()->AddFontCompressed(tracy::Arimo_compressed_data, tracy::Cousine_compressed_size, "Arimo", nullptr, 20.0f);
        smallFont = context_->GetSubsystem<SystemUI>()->AddFontCompressed(tracy::Arimo_compressed_data, tracy::Cousine_compressed_size, "Arimo", nullptr, 10.0f);

        if (!readCapture.empty())
        {
            std::unique_ptr<tracy::FileRead> file(tracy::FileRead::Open(readCapture.c_str()));
            if (file)
                view = std::make_unique<tracy::View>(RunOnMainThread, *file);
        }

        SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap&) { Update(); });
    }

    void Stop() override
    {
        JSONValue root{JSON_OBJECT};
        root["x"] = context_->GetSubsystem<Graphics>()->GetWindowPosition().x_;
        root["y"] = context_->GetSubsystem<Graphics>()->GetWindowPosition().y_;
        root["width"] = context_->GetSubsystem<Graphics>()->GetWidth();
        root["height"] = context_->GetSubsystem<Graphics>()->GetHeight();

        JSONFile config(context_);
        config.GetRoot() = root;
        config.SaveFile(Format("{}/Settings.json", context_->GetSubsystem<FileSystem>()->GetAppPreferencesDir("rbfx", "Profiler")));
    }

    float GetDPIScale()
    {
        return context_->GetSubsystem<Graphics>()->GetDisplayDPI(context_->GetSubsystem<Graphics>()->GetCurrentMonitor()).z_ / 96.f;
    }

    void Update()
    {
        static bool reconnect = false;
        static std::string reconnectAddr;
        static uint16_t reconnectPort;
        static bool showFilter = false;
        auto& style = ImGui::GetStyle();
        auto& io = ImGui::GetIO();
        tracy::MouseFrame();

        setlocale( LC_NUMERIC, "C" );

        if( !view )
        {
            const auto time = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
            if( !broadcastListen )
            {
                broadcastListen = std::make_unique<tracy::UdpListen>();
                if( !broadcastListen->Listen( port ) )
                {
                    broadcastListen.reset();
                }
            }
            else
            {
                tracy::IpAddress addr;
                size_t len;
                for(;;)
                {
                    auto msg = broadcastListen->Read( len, addr, 0 );
                    if( !msg ) break;
                    if( len > sizeof( tracy::BroadcastMessage ) ) continue;
                    tracy::BroadcastMessage bm;
                    memcpy( &bm, msg, len );

                    if( bm.broadcastVersion == tracy::BroadcastVersion )
                    {
                        const uint32_t protoVer = bm.protocolVersion;
                        const auto procname = bm.programName;
                        const auto activeTime = bm.activeTime;
                        const auto listenPort = bm.listenPort;
                        auto address = addr.GetText();

                        const auto ipNumerical = addr.GetNumber();
                        const auto clientId = uint64_t( ipNumerical ) | ( uint64_t( listenPort ) << 32 );
                        auto it = clients.find( clientId );
                        if( activeTime >= 0 )
                        {
                            if( it == clients.end() )
                            {
                                std::string ip( address );
                                resolvLock.lock();
                                if( resolvMap.find( ip ) == resolvMap.end() )
                                {
                                    resolvMap.emplace( ip, ip );
                                    resolv.Query( ipNumerical, [this, ip] ( std::string&& name ) {
                                        std::lock_guard<std::mutex> lock( resolvLock );
                                        auto it = resolvMap.find( ip );
                                        assert( it != resolvMap.end() );
                                        std::swap( it->second, name );
                                    } );
                                }
                                resolvLock.unlock();
                                clients.emplace( clientId, ClientData { time, protoVer, activeTime, listenPort, procname, std::move( ip ) } );
                            }
                            else
                            {
                                it->second.time = time;
                                it->second.activeTime = activeTime;
                                it->second.port = listenPort;
                                if( it->second.protocolVersion != protoVer ) it->second.protocolVersion = protoVer;
                                if( strcmp( it->second.procName.c_str(), procname ) != 0 ) it->second.procName = procname;
                            }
                        }
                        else if( it != clients.end() )
                        {
                            clients.erase( it );
                        }
                    }
                }
                auto it = clients.begin();
                while( it != clients.end() )
                {
                    const auto diff = time - it->second.time;
                    if( diff > 4000 )  // 4s
                    {
                        it = clients.erase( it );
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            auto& style = ImGui::GetStyle();
            style.Colors[ImGuiCol_WindowBg] = ImVec4( 0.129f, 0.137f, 0.11f, 1.f );
            ImGui::Begin( "Get started", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse );
            char buf[128];
            sprintf(buf, "Urho3D Profiler %i.%i.%i", tracy::Version::Major, tracy::Version::Minor, tracy::Version::Patch);
            ImGui::PushFont( bigFont );
            tracy::TextCentered( buf );
            ImGui::PopFont();
            ImGui::SameLine( ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize( ICON_FA_WRENCH ).x - ImGui::GetStyle().FramePadding.x * 2 );
            if( ImGui::Button( ICON_FA_WRENCH ) )
            {
                ImGui::OpenPopup( "About Tracy" );
            }
            bool keepOpenAbout = true;
            if( ImGui::BeginPopupModal( "About Tracy", &keepOpenAbout, ImGuiWindowFlags_AlwaysAutoResize ) )
            {
                ImGui::PushFont( bigFont );
                tracy::TextCentered( buf );
                ImGui::PopFont();
                ImGui::Spacing();
                ImGui::TextUnformatted( "A real time, nanosecond resolution, remote telemetry, hybrid\nframe and sampling profiler for games and other applications." );
                ImGui::Spacing();
                ImGui::TextUnformatted( "Created by Bartosz Taudul" );
                ImGui::SameLine();
                tracy::TextDisabledUnformatted( "<wolf@nereid.pl>" );
                tracy::TextDisabledUnformatted( "Additional authors listed in AUTHORS file and in git history." );
                ImGui::Separator();
                tracy::TextFocused( "Protocol version", tracy::RealToString( tracy::ProtocolVersion ) );
                tracy::TextFocused( "Broadcast version", tracy::RealToString( tracy::BroadcastVersion ) );
                tracy::TextFocused( "Build date", __DATE__ ", " __TIME__ );
                ImGui::EndPopup();
            }
            ImGui::Spacing();
            if( ImGui::Button( ICON_FA_BOOK " Manual" ) )
            {
                 tracy::OpenWebpage( "https://github.com/wolfpld/tracy/releases" );
            }
            ImGui::SameLine();
            if( ImGui::Button( ICON_FA_GLOBE_AMERICAS " Web" ) )
            {
                 ImGui::OpenPopup( "web" );
            }
            if( ImGui::BeginPopup( "web" ) )
            {
                if( ImGui::Selectable( ICON_FA_HOME " Tracy Profiler home page" ) )
                {
                    tracy::OpenWebpage( "https://github.com/wolfpld/tracy" );
                }
                ImGui::Separator();
                if( ImGui::Selectable( ICON_FA_VIDEO " Overview of v0.2" ) )
                {
                    tracy::OpenWebpage( "https://www.youtube.com/watch?v=fB5B46lbapc" );
                }
                if( ImGui::Selectable( ICON_FA_VIDEO " New features in v0.3" ) )
                {
                    tracy::OpenWebpage( "https://www.youtube.com/watch?v=3SXpDpDh2Uo" );
                }
                if( ImGui::Selectable( ICON_FA_VIDEO " New features in v0.4" ) )
                {
                    tracy::OpenWebpage( "https://www.youtube.com/watch?v=eAkgkaO8B9o" );
                }
                if( ImGui::Selectable( ICON_FA_VIDEO " New features in v0.5" ) )
                {
                    tracy::OpenWebpage( "https://www.youtube.com/watch?v=P6E7qLMmzTQ" );
                }
                if( ImGui::Selectable( ICON_FA_VIDEO " New features in v0.6" ) )
                {
                    tracy::OpenWebpage( "https://www.youtube.com/watch?v=uJkrFgriuOo" );
                }
                if( ImGui::Selectable( ICON_FA_VIDEO " New features in v0.7" ) )
                {
                    tracy::OpenWebpage( "https://www.youtube.com/watch?v=_hU7vw00MZ4" );
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            if( ImGui::Button( ICON_FA_COMMENT " Chat" ) )
            {
                OpenWebpage( "https://discord.gg/pk78auc" );
            }
            ImGui::SameLine();
            if( ImGui::Button( ICON_FA_HEART " Sponsor" ) )
            {
                OpenWebpage( "https://github.com/sponsors/wolfpld/" );
            }
            /*
            if( updateVersion != 0 && updateVersion > tracy::FileVersion( tracy::Version::Major, tracy::Version::Minor, tracy::Version::Patch ) )
            {
                ImGui::Separator();
                ImGui::TextColored( ImVec4( 1, 1, 0, 1 ), ICON_FA_EXCLAMATION " Update to %i.%i.%i is available!", ( updateVersion >> 16 ) & 0xFF, ( updateVersion >> 8 ) & 0xFF, updateVersion & 0xFF );
                ImGui::SameLine();
                if( ImGui::SmallButton( ICON_FA_GIFT " Get it!" ) )
                {
                    showReleaseNotes = true;
                    if( !updateNotesThread.joinable() )
                    {
                        updateNotesThread = std::thread( [] {
                            HttpRequest( "nereid.pl", "/tracy/notes", 8099, [] ( int size, char* data ) {
                                std::string notes( data, data+size );
                                delete[] data;
                                RunOnMainThread( [notes = move( notes )] { releaseNotes = std::move( notes ); } );
                            } );
                        } );
                    }
                }
            }
            */
            ImGui::Separator();
            ImGui::TextUnformatted( "Client address" );
            bool connectClicked = false;
            connectClicked |= ImGui::InputTextWithHint( "###connectaddress", "Enter address", addr, 1024, ImGuiInputTextFlags_EnterReturnsTrue );
            if( !connHistVec.empty() )
            {
                ImGui::SameLine();
                if( ImGui::BeginCombo( "##frameCombo", nullptr, ImGuiComboFlags_NoPreview ) )
                {
                    int idxRemove = -1;
                    const auto sz = std::min<size_t>( 5, connHistVec.size() );
                    for( size_t i=0; i<sz; i++ )
                    {
                        const auto& str = connHistVec[i]->first;
                        if( ImGui::Selectable( str.c_str() ) )
                        {
                            memcpy( addr, str.c_str(), str.size() + 1 );
                        }
                        if( ImGui::IsItemHovered() && ImGui::IsKeyPressed( ImGui::GetKeyIndex( ImGuiKey_Delete ), false ) )
                        {
                            idxRemove = (int)i;
                        }
                    }
                    if( idxRemove >= 0 )
                    {
                        connHistMap.erase( connHistVec[idxRemove] );
                        connHistVec = RebuildConnectionHistory( connHistMap );
                    }
                    ImGui::EndCombo();
                }
            }
            connectClicked |= ImGui::Button( ICON_FA_WIFI " Connect" );
            if( connectClicked && *addr && !loadThread.joinable() )
            {
                const auto addrLen = strlen( addr );
                std::string addrStr( addr, addr+addrLen );
                auto it = connHistMap.find( addrStr );
                if( it != connHistMap.end() )
                {
                    it->second++;
                }
                else
                {
                    connHistMap.emplace( std::move( addrStr ), 1 );
                }
                connHistVec = RebuildConnectionHistory( connHistMap );

                auto ptr = addr + addrLen - 1;
                while( ptr > addr && *ptr != ':' ) ptr--;
                if( *ptr == ':' )
                {
                    std::string addrPart = std::string( addr, ptr );
                    uint16_t portPart = (uint16_t)atoi( ptr+1 );
                    view = std::make_unique<tracy::View>( RunOnMainThread, addrPart.c_str(), portPart, fixedWidth, smallFont, bigFont, SetWindowTitleCallback, GetMainWindowNative );
                }
                else
                {
                    view = std::make_unique<tracy::View>( RunOnMainThread, addr, port, fixedWidth, smallFont, bigFont, SetWindowTitleCallback, GetMainWindowNative );
                }
            }
            ImGui::SameLine( 0, ImGui::GetFontSize() * 2 );

#ifndef TRACY_NO_FILESELECTOR
            if( ImGui::Button( ICON_FA_FOLDER_OPEN " Open saved trace" ) && !loadThread.joinable() )
            {
                nfdchar_t* fn;
                auto res = NFD_OpenDialog( "tracy", nullptr, &fn, GetMainWindowNative() );
                if( res == NFD_OKAY )
                {
                    try
                    {
                        auto f = std::shared_ptr<tracy::FileRead>( tracy::FileRead::Open( fn ) );
                        if( f )
                        {
                            loadThread = std::thread([this, f] {
                                try
                                {
                                    view = std::make_unique<tracy::View>( RunOnMainThread, *f, fixedWidth, smallFont, bigFont, SetWindowTitleCallback, GetMainWindowNative );
                                }
                                catch( const tracy::UnsupportedVersion& e )
                                {
                                    badVer.state = tracy::BadVersionState::UnsupportedVersion;
                                    badVer.version = e.version;
                                }
                                catch( const tracy::LegacyVersion& e )
                                {
                                    badVer.state = tracy::BadVersionState::LegacyVersion;
                                    badVer.version = e.version;
                                }
                            } );
                        }
                    }
                    catch( const tracy::NotTracyDump& )
                    {
                        badVer.state = tracy::BadVersionState::BadFile;
                    }
                    catch( const tracy::FileReadError& )
                    {
                        badVer.state = tracy::BadVersionState::ReadError;
                    }
                }
            }

            if( badVer.state != tracy::BadVersionState::Ok )
            {
                if( loadThread.joinable() ) { loadThread.join(); }
                tracy::BadVersion( badVer );
            }
#endif

            if( !clients.empty() )
            {
                ImGui::Separator();
                ImGui::TextUnformatted( "Discovered clients:" );
                ImGui::SameLine();
                tracy::SmallToggleButton( ICON_FA_FILTER " Filter", showFilter );
                if( addrFilter.IsActive() || portFilter.IsActive() || progFilter.IsActive() )
                {
                    ImGui::SameLine();
                    tracy::TextColoredUnformatted( 0xFF00FFFF, ICON_FA_EXCLAMATION_TRIANGLE );
                    if( ImGui::IsItemHovered() )
                    {
                        ImGui::BeginTooltip();
                        ImGui::TextUnformatted( "Filters are active" );
                        ImGui::EndTooltip();
                    }
                    if( showFilter )
                    {
                        ImGui::SameLine();
                        if( ImGui::SmallButton( ICON_FA_BACKSPACE " Clear" ) )
                        {
                            addrFilter.Clear();
                            portFilter.Clear();
                            progFilter.Clear();
                        }
                    }
                }
                if( showFilter )
                {
                    const auto w = ImGui::GetFontSize() * 12;
                    ImGui::Separator();
                    addrFilter.Draw( "Address filter", w );
                    portFilter.Draw( "Port filter", w );
                    progFilter.Draw( "Program filter", w );
                }
                ImGui::Separator();
                static bool widthSet = false;
                ImGui::Columns( 3 );
                if( !widthSet )
                {
                    widthSet = true;
                    const auto w = ImGui::GetWindowWidth();
                    ImGui::SetColumnWidth( 0, w * 0.35f );
                    ImGui::SetColumnWidth( 1, w * 0.175f );
                    ImGui::SetColumnWidth( 2, w * 0.425f );
                }
                std::lock_guard<std::mutex> lock( resolvLock );
                int idx = 0;
                int passed = 0;
                for( auto& v : clients )
                {
                    const bool badProto = v.second.protocolVersion != tracy::ProtocolVersion;
                    bool sel = false;
                    const auto& name = resolvMap.find( v.second.address );
                    assert( name != resolvMap.end() );
                    if( addrFilter.IsActive() && !addrFilter.PassFilter( name->second.c_str() ) && !addrFilter.PassFilter( v.second.address.c_str() ) ) continue;
                    if( portFilter.IsActive() )
                    {
                        char buf[32];
                        sprintf( buf, "%" PRIu16, v.second.port );
                        if( !portFilter.PassFilter( buf ) ) continue;
                    }
                    if( progFilter.IsActive() && !progFilter.PassFilter( v.second.procName.c_str() ) ) continue;
                    ImGuiSelectableFlags flags = ImGuiSelectableFlags_SpanAllColumns;
                    if( badProto ) flags |= ImGuiSelectableFlags_Disabled;
                    ImGui::PushID( idx++ );
                    const bool selected = ImGui::Selectable( name->second.c_str(), &sel, flags );
                    ImGui::PopID();
                    if( ImGui::IsItemHovered() )
                    {
                        char portstr[32];
                        sprintf( portstr, "%" PRIu16, v.second.port );
                        ImGui::BeginTooltip();
                        if( badProto )
                        {
                            tracy::TextColoredUnformatted( 0xFF0000FF, "Incompatible protocol!" );
                            ImGui::SameLine();
                            ImGui::TextDisabled( "(used: %i, required: %i)", v.second.protocolVersion, tracy::ProtocolVersion );
                        }
                        tracy::TextFocused( "IP:", v.second.address.c_str() );
                        tracy::TextFocused( "Port:", portstr );
                        ImGui::EndTooltip();
                    }
                    if( v.second.port != port )
                    {
                        ImGui::SameLine();
                        ImGui::TextDisabled( ":%" PRIu16, v.second.port );
                    }
                    if( selected && !loadThread.joinable() )
                    {
                        view = std::make_unique<tracy::View>( RunOnMainThread, v.second.address.c_str(), v.second.port, fixedWidth, smallFont, bigFont, SetWindowTitleCallback, GetMainWindowNative );
                    }
                    ImGui::NextColumn();
                    const auto acttime = ( v.second.activeTime + ( time - v.second.time ) / 1000 ) * 1000000000ll;
                    if( badProto )
                    {
                        tracy::TextDisabledUnformatted( tracy::TimeToString( acttime ) );
                    }
                    else
                    {
                        ImGui::TextUnformatted( tracy::TimeToString( acttime ) );
                    }
                    ImGui::NextColumn();
                    if( badProto )
                    {
                        tracy::TextDisabledUnformatted( v.second.procName.c_str() );
                    }
                    else
                    {
                        ImGui::TextUnformatted( v.second.procName.c_str() );
                    }
                    ImGui::NextColumn();
                    passed++;
                }
                ImGui::EndColumns();
                if( passed == 0 )
                {
                    ImGui::TextUnformatted( "All clients are filtered." );
                }
            }
            ImGui::End();
            /*
            if( showReleaseNotes )
            {
                assert( updateNotesThread.joinable() );
                ImGui::SetNextWindowSize( ImVec2( 600, 400 ), ImGuiCond_FirstUseEver );
                ImGui::Begin( "Update available!", &showReleaseNotes );
                if( ImGui::Button( ICON_FA_DOWNLOAD " Download" ) )
                {
                    OpenWebpage( "https://github.com/wolfpld/tracy/releases" );
                }
                ImGui::BeginChild( "###notes", ImVec2( 0, 0 ), true );
                if( releaseNotes.empty() )
                {
                    static float rnTime = 0;
                    rnTime += ImGui::GetIO().DeltaTime;
                    tracy::TextCentered( "Fetching release notes..." );
                    tracy::DrawWaitingDots( rnTime );
                }
                else
                {
                    ImGui::PushFont( fixedWidth );
                    ImGui::TextUnformatted( releaseNotes.c_str() );
                    ImGui::PopFont();
                }
                ImGui::EndChild();
                ImGui::End();
            }
            */
        }
        else
        {
            // if( showReleaseNotes ) showReleaseNotes = false;
            if( broadcastListen )
            {
                broadcastListen.reset();
                clients.clear();
            }
            if( loadThread.joinable() ) loadThread.join();

            int display_w = context_->GetSubsystem<Graphics>()->GetWidth();
            int display_h = context_->GetSubsystem<Graphics>()->GetHeight();

            view->NotifyRootWindowSize( display_w, display_h );
            if( !view->Draw() )
            {
                viewShutdown.store( ViewShutdown::True, std::memory_order_relaxed );
                reconnect = view->ReconnectRequested();
                if( reconnect )
                {
                    reconnectAddr = view->GetAddress();
                    reconnectPort = view->GetPort();
                }
                loadThread = std::thread( [view = std::move( view ), this] () mutable {
                    view.reset();
                    viewShutdown.store( ViewShutdown::Join, std::memory_order_relaxed );
                } );
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

            animTime += ImGui::GetIO().DeltaTime;
            tracy::DrawWaitingDots( animTime );

            auto currProgress = progress.progress.load( std::memory_order_relaxed );
            if( totalProgress == 0 )
            {
                ImGui::CloseCurrentPopup();
                totalProgress = currProgress;
            }
            switch( currProgress )
            {
            case tracy::LoadProgress::Initialization:
                ImGui::TextUnformatted( "Initialization..." );
                break;
            case tracy::LoadProgress::Locks:
                ImGui::TextUnformatted( "Locks..." );
                break;
            case tracy::LoadProgress::Messages:
                ImGui::TextUnformatted( "Messages..." );
                break;
            case tracy::LoadProgress::Zones:
                ImGui::TextUnformatted( "CPU zones..." );
                break;
            case tracy::LoadProgress::GpuZones:
                ImGui::TextUnformatted( "GPU zones..." );
                break;
            case tracy::LoadProgress::Plots:
                ImGui::TextUnformatted( "Plots..." );
                break;
            case tracy::LoadProgress::Memory:
                ImGui::TextUnformatted( "Memory..." );
                break;
            case tracy::LoadProgress::CallStacks:
                ImGui::TextUnformatted( "Call stacks..." );
                break;
            case tracy::LoadProgress::FrameImages:
                ImGui::TextUnformatted( "Frame images..." );
                break;
            case tracy::LoadProgress::ContextSwitches:
                ImGui::TextUnformatted( "Context switches..." );
                break;
            case tracy::LoadProgress::ContextSwitchesPerCpu:
                ImGui::TextUnformatted( "CPU context switches..." );
                break;
            default:
                assert( false );
                break;
            }
            ImGui::ProgressBar( float( currProgress ) / totalProgress, ImVec2( 200 * dpiScale, 0 ) );

            ImGui::TextUnformatted( "Progress..." );
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
        switch( viewShutdown.load( std::memory_order_relaxed ) )
        {
        case ViewShutdown::True:
            ImGui::OpenPopup( "Capture cleanup..." );
            break;
        case ViewShutdown::Join:
            loadThread.join();
            viewShutdown.store( ViewShutdown::False, std::memory_order_relaxed );
            if( reconnect )
            {
                view = std::make_unique<tracy::View>( RunOnMainThread, reconnectAddr.c_str(), reconnectPort, fixedWidth, smallFont, bigFont, SetWindowTitleCallback, GetMainWindowNative );
            }
            break;
        default:
            break;
        }
        if( ImGui::BeginPopupModal( "Capture cleanup...", nullptr, ImGuiWindowFlags_AlwaysAutoResize ) )
        {
            if( viewShutdown.load( std::memory_order_relaxed ) != ViewShutdown::True ) ImGui::CloseCurrentPopup();
            tracy::TextCentered( ICON_FA_BROOM );
            animTime += ImGui::GetIO().DeltaTime;
            tracy::DrawWaitingDots( animTime );
            ImGui::TextUnformatted( "Please wait, cleanup is in progress" );
            ImGui::EndPopup();
        }
    }
};

URHO3D_DEFINE_APPLICATION_MAIN(ProfilerApplication);
