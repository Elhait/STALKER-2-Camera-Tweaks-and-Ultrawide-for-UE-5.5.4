// Links complete preserved production objects, excluding DllMain only. Never
// constructs Renderer/InputState using headers with a different class layout.
#include "../../../src/plugin/runtime.hpp"
#include "../../../src/overlay/composition/composition_runtime.hpp"
#include "../../../src/diagnostics/diagnostic_runtime.hpp"
#include "../../../src/config/config_repository.hpp"
#include "../../../src/config/config_template.hpp"
#include <dwmapi.h>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int Width = 1200, Height = 800;
constexpr COLORREF Background = RGB(40,100,160);
HWND host{}, display{};
HANDLE owner{};
unsigned clicks{};
bool workerStarted{};
HWND focusPeer{};
config::FeatureConfig loadedConfig{};
bool useConfig{};
bool traceRestore{};
void Require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
LRESULT CALLBACK HostProc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{}; HDC dc=BeginPaint(window,&paint);
        HBRUSH brush=CreateSolidBrush(Background); FillRect(dc,&paint.rcPaint,brush);
        DeleteObject(brush); EndPaint(window,&paint); return 0;
    }
    if (message == WM_LBUTTONDOWN) ++clicks;
    const auto result=DefWindowProcW(window,message,wp,lp);
    if(traceRestore && (message==WM_WINDOWPOSCHANGED||message==WM_SIZE||
            message==WM_ACTIVATE||message==WM_SETFOCUS)) {
        const auto* pos=message==WM_WINDOWPOSCHANGED?reinterpret_cast<WINDOWPOS*>(lp):nullptr;
        std::printf("HOST_MESSAGE tick=%llu message=%u flags=%u after=%p foreground=%d\n",
            GetTickCount64(),message,pos?pos->flags:0,pos?pos->hwndInsertAfter:nullptr,GetForegroundWindow()==window);
    }
    return result;
}
void Pump(unsigned duration) {
    const auto end=GetTickCount64()+duration;
    do {
        MSG message{};
        unsigned count=0;
        while (count++ < 1000 && PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            TranslateMessage(&message); DispatchMessageW(&message);
        }
        Sleep(5);
    } while (GetTickCount64()<end);
}
BOOL CALLBACK FindDisplay(HWND window, LPARAM) {
    DWORD process{}; GetWindowThreadProcessId(window,&process);
    wchar_t name[128]{}; GetClassNameW(window,name,128);
    if (process==GetCurrentProcessId() &&
        std::wstring_view(name)==L"STALKER2CameraTweaks.OverlayDisplayWindow") display=window;
    return TRUE;
}
bool Snapshot(plugin::RuntimeSettingsSnapshot& out, void*) {
    out={}; out.overlayLocaleCode="en"; out.overlayLocaleAuto=false;
    out.gameplayEnabled=true; out.hotkeysEnabled=true;
    if(useConfig) {
        out.gameplayEnabled=loadedConfig.gameplayEnabled;
        out.gameplayMode=loadedConfig.gameplayMode;
        out.cinematicsEnabled=loadedConfig.cinematicsEnabled;
        out.cinematicAspectPolicy=loadedConfig.cinematicAspectPolicy;
        out.cinematicFovMode=loadedConfig.cinematicFovMode;
        out.dialogueEnabled=loadedConfig.dialogueEnabled;
        out.dialogueZoomPolicy=loadedConfig.dialogueZoomPolicy;
        out.overlayLocaleAuto=loadedConfig.overlayLocaleAuto;
        out.overlayFontSize=loadedConfig.overlayFontSize;
        out.overlayToggleKey=loadedConfig.overlayToggleKey;
        out.hotkeysEnabled=loadedConfig.hotkeysEnabled;
        out.overlayLocaleCode=loadedConfig.overlayLocaleCode;
        if(out.overlayLocaleAuto) {
            // Native camera admission is intentionally unopened in this host.
            // Model an already accepted English fallback, not a game-language
            // handshake (GetRuntimeSettingsSnapshot cannot exercise that here).
            out.overlayLocaleCode="en";
            out.overlayLocaleAutoSynchronized=true;
        }
    }
    return true;
}
bool Semantic(plugin::OverlaySemanticSnapshot& out, void*) {
    out={};
    // Named synthetic camera facts permit the real settings panel to render;
    // they are not observations of game hooks or camera behavior.
    out.gameplayEnabled.valid=out.gameplayMode.valid=true;
    out.cinematicAspectPolicy.valid=out.cinematicFovMode.valid=true;
    out.dialogueEnabled.valid=out.dialogueZoomPolicy.valid=true;
    out.gameplayEnabled.value=true;
    out.gameplayMode.value=config::GameplayMode::RuntimeCombine;
    out.cinematicAspectPolicy.value=config::CinematicAspectPolicy::Auto;
    out.cinematicFovMode.value=config::CinematicFovMode::GameplayHorPlus;
    out.dialogueEnabled.value=true;
    out.dialogueZoomPolicy.value=config::DialogueZoomPolicy::Adaptive;
    return true;
}
void Key(WORD key) {
    Require(GetForegroundWindow()==host,"host not foreground; refusing SendInput");
    INPUT input[2]{}; input[0].type=input[1].type=INPUT_KEYBOARD;
    input[0].ki.wVk=input[1].ki.wVk=key; input[1].ki.dwFlags=KEYEVENTF_KEYUP;
    Require(SendInput(2,input,sizeof(INPUT))==2,"SendInput key"); Pump(500);
}
void Click() {
    POINT point{50,Height-50}; ClientToScreen(host,&point);
    wchar_t title[200]{}; GetWindowTextW(WindowFromPoint(point),title,200);
    std::printf("CLICK point=%ld,%ld target_host=%d foreground_host=%d other_title=%ls\n",
        point.x,point.y,WindowFromPoint(point)==host,GetForegroundWindow()==host,title);
    Require(WindowFromPoint(point)==host,"foreign window at fixture click");
    SetCursorPos(point.x,point.y);
    INPUT input[2]{}; input[0].type=input[1].type=INPUT_MOUSE;
    input[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN; input[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
    Require(SendInput(2,input,sizeof(INPUT))==2,"SendInput mouse"); Pump(150);
}
bool Near(BYTE value, int expected) { return abs(int(value)-expected)<12; }
bool DisplayAboveHost();
// Independent desktop pixels; not renderer readback, HRESULT or IsWindowVisible.
bool Capture(const char* phase, bool expectMenu=false) {
    POINT origin{}; ClientToScreen(host,&origin); DwmFlush();
    HDC screen=GetDC(nullptr), memory=CreateCompatibleDC(screen);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=Width; info.bmiHeader.biHeight=-Height;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32;
    void* pixels{}; HBITMAP bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    Require(bitmap && memory,"desktop capture allocation");
    HGDIOBJ previous=SelectObject(memory,bitmap);
    const bool copied=BitBlt(memory,0,0,Width,Height,screen,origin.x,origin.y,SRCCOPY|CAPTUREBLT)!=FALSE;
    unsigned changed{}, upper{}, background{}, toastRegion{}, menuBody{};
    const auto* bytes=static_cast<BYTE*>(pixels);
    for(int y=10;y<Height-10;y+=4) for(int x=10;x<Width-10;x+=4) {
        const BYTE* pixel=bytes+4*(y*Width+x);
        const bool match=Near(pixel[0],160)&&Near(pixel[1],100)&&Near(pixel[2],40);
        if(!match) {
            ++changed; if(y<Height/2) ++upper;
            if(y>=20&&y<160&&x>Width/2-300&&x<Width/2+300) ++toastRegion;
            if(y>=Height/4) ++menuBody;
        } else ++background;
    }
    const std::string name=std::string(phase)+".bmp";
    FILE* file{}; fopen_s(&file,name.c_str(),"wb");
    if(file) {
        BITMAPFILEHEADER header{}; header.bfType=0x4d42;
        header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
        header.bfSize=header.bfOffBits+Width*Height*4;
        fwrite(&header,sizeof(header),1,file); fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,file);
        fwrite(pixels,Width*Height*4,1,file); fclose(file);
    }
    SelectObject(memory,previous); DeleteObject(bitmap); DeleteDC(memory); ReleaseDC(nullptr,screen);
    const bool visible=copied && background>3000 && (expectMenu?menuBody>1000:toastRegion>100);
    std::printf("PIXELS phase=%s changed=%u upper=%u background=%u toast_region=%u menu_body=%u oracle=%s hwnd_visible=%d foreground_host=%d\n",
        phase,changed,upper,background,toastRegion,menuBody,visible?"VISIBLE":"INVISIBLE",IsWindowVisible(display),GetForegroundWindow()==host);
    RECT rectangle{}; GetWindowRect(display,&rectangle);
    DWORD cloaked{}; DwmGetWindowAttribute(display,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));
    std::printf("WINDOW phase=%s host_origin=%ld,%ld display_rect=%ld,%ld,%ld,%ld above=%d cloaked=%lu\n",
        phase,origin.x,origin.y,rectangle.left,rectangle.top,rectangle.right,rectangle.bottom,DisplayAboveHost(),cloaked);
    return visible;
}
bool DisplayAboveHost() {
    HWND window=GetWindow(host,GW_HWNDPREV);
    for(unsigned count=0;window&&count<256;++count,window=GetWindow(window,GW_HWNDPREV))
        if(window==display) return true;
    return false;
}
bool Cleanup() {
    if(display && IsWindow(display)) PostMessageW(display,WM_CLOSE,0,0);
    const auto end=GetTickCount64()+5000;
    while(owner && WaitForSingleObject(owner,0)==WAIT_TIMEOUT && GetTickCount64()<end) Pump(20);
    const bool exited=!workerStarted || (owner && WaitForSingleObject(owner,0)==WAIT_OBJECT_0);
    const bool gone=!display||!IsWindow(display);
    if(owner) CloseHandle(owner);
    if(focusPeer && IsWindow(focusPeer)) DestroyWindow(focusPeer);
    if(host && IsWindow(host)) DestroyWindow(host);
    std::printf("CLEANUP worker_started=%d owner_exited_or_not_started=%d display_destroyed=%d\n",workerStarted,exited,gone);
    return exited&&gone;
}
}
int main(int argc,char** argv) {
    setvbuf(stdout,nullptr,_IONBF,0);
    HWND previous=GetForegroundWindow(); POINT cursor{}; GetCursorPos(&cursor);
    int result=2;
    try {
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        HMODULE module=GetModuleHandleW(nullptr); plugin::SetModuleHandle(module);
        useConfig=argc>2 && std::string_view(argv[2])=="config";
        if(useConfig) {
            wchar_t path[MAX_PATH]{}; GetModuleFileNameW(module,path,MAX_PATH);
            const auto ini=std::filesystem::path(path).replace_filename(L"STALKER2CameraTweaks.ini");
            Require(config::LoadFeatureConfig(ini,loadedConfig,
                [](const std::filesystem::path& file) {return config::SynchronizeManagedConfigTemplate(file);},
                [](std::string text) {std::printf("CONFIG %s\n",text.c_str());}),"config load");
            std::printf("CONFIG overlay_enabled=%d auto=%d font=%d toggle=%d\n",loadedConfig.overlayEnabled,
                loadedConfig.overlayLocaleAuto,loadedConfig.overlayFontSize,loadedConfig.overlayToggleKey);
            Require(loadedConfig.overlayEnabled,"INI disables Overlay");
        }
        // Publish only the stable callback-bundle prefix. No API object is
        // constructed here; G5/F4 each retain their own actual runtime owner.
        plugin::RuntimeSettingsApi::CallbackBundle callbacks{};
        callbacks.snapshotHandler=&Snapshot; callbacks.semanticSnapshotHandler=&Semantic;
        Require(plugin::GetRuntimeSettingsApi().Publish(callbacks),"settings publication");
        diagnostics::SetEnabled(argc>1 && std::string_view(argv[1])=="on");
        WNDCLASSW cls{}; cls.lpfnWndProc=&HostProc; cls.hInstance=module;
        cls.lpszClassName=L"CameraTweaksProductionDesktopHost";
        Require(RegisterClassW(&cls)!=0,"host class");
        RECT bounds{0,0,Width,Height}; AdjustWindowRect(&bounds,WS_OVERLAPPEDWINDOW,FALSE);
        host=CreateWindowExW(0,cls.lpszClassName,L"Camera Tweaks — production desktop probe",
            WS_OVERLAPPEDWINDOW,100,100,bounds.right-bounds.left,bounds.bottom-bounds.top,
            nullptr,nullptr,module,nullptr);
        Require(host!=nullptr,"host creation"); ShowWindow(host,SW_SHOW); UpdateWindow(host);
        SetWindowPos(host,HWND_TOP,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        SetForegroundWindow(host); Pump(500);
        if(GetForegroundWindow()!=host) Click();
        Require(GetForegroundWindow()==host,"desktop foreground unavailable");
        Require(!Capture("baseline"),"baseline falsely reports UI");
        NotifyOverlayCameraCoreReady(); Require(StartOverlayDiscovery(module),"worker start");
        workerStarted=true;
        const auto until=GetTickCount64()+5000;
        do { Pump(50); EnumWindows(&FindDisplay,0); } while(!display&&GetTickCount64()<until);
        Require(display!=nullptr,"display window discovery");
        owner=OpenThread(SYNCHRONIZE,FALSE,GetWindowThreadProcessId(display,nullptr));
        Require(owner!=nullptr,"owner thread observation"); Pump(1500);
        const bool raiseHost=argc>3 && std::string_view(argv[3])=="raise-host";
        if(raiseHost) {
            Require(SetWindowPos(host,HWND_TOP,0,0,0,0,
                SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE)!=FALSE,"host startup z-order assertion");
            Pump(300);
        }
        const bool toast=Capture("toast");
        const bool toastNotMenu=!Capture("toast-only-not-menu",true);
        Key(VK_DELETE); const bool menu=Capture("menu",true);
        bool raised=true;
        if(raiseHost) {
            Require(SetWindowPos(host,HWND_TOP,0,0,0,0,
                SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE)!=FALSE,"host native z-order assertion");
            Pump(300); RequestOverlayRedraw(); Pump(300);
            raised=Capture("host-raised",true);
            std::printf("ZORDER display_above_host=%d display_topmost=%d\n",
                DisplayAboveHost(),
                (GetWindowLongPtrW(display,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0);
        }
        const unsigned before=clicks; Click();
        std::printf("INPUT menu_game_clicks=%u\n",clicks-before);
        const bool captured=clicks==before;
        // Same oracle with intentionally hidden display must reject visibility.
        ShowWindow(display,SW_HIDE); Pump(40);
        const bool negative=!Capture("negative-hidden",true);
        std::printf("NEGATIVE_CONTROL rejected_hidden=%d\n",negative);
        // Undo the deliberate external mutation; production owns/caches display
        // visibility and cannot be expected to reconcile a test's hidden HWND.
        ShowWindow(display,SW_SHOWNOACTIVATE); Pump(50);
        Key(VK_DELETE); Key(VK_DELETE);
        const bool reopened=Capture("reopened",true);
        RECT hostRect{}; GetWindowRect(host,&hostRect);
        focusPeer=CreateWindowExW(0,cls.lpszClassName,L"Owned test focus peer",
            WS_OVERLAPPEDWINDOW,hostRect.right+20,hostRect.top,360,240,nullptr,nullptr,module,nullptr);
        Require(focusPeer!=nullptr,"focus peer creation"); ShowWindow(focusPeer,SW_SHOW);
        SetForegroundWindow(focusPeer); Pump(500);
        const bool focusHidden=GetForegroundWindow()==focusPeer && !IsWindowVisible(display);
        SetForegroundWindow(host); Pump(500);
        const bool focusRestored=Capture("focus-restored",true);
        std::printf("FOCUS hidden=%d restored=%d\n",focusHidden,focusRestored);
        DestroyWindow(focusPeer); focusPeer=nullptr;
        traceRestore=true;
        ShowWindow(host,SW_MINIMIZE); Pump(250);
        const bool minimized=!IsWindowVisible(display);
        ShowWindow(host,SW_RESTORE); SetForegroundWindow(host); Pump(1800);
        const bool restored=Capture("restored",true);
        Key(VK_DELETE);
        const unsigned beforeClosedClick=clicks; Click();
        const bool released=clicks==beforeClosedClick+1;
        std::printf("INPUT closed_game_clicks=%u\n",clicks-beforeClosedClick);
        std::printf("RESULT toast=%d menu=%d negative=%d reopened=%d minimized=%d restored=%d\n",
            toast,menu,negative,reopened,minimized,restored);
        result=toast&&toastNotMenu&&menu&&negative&&reopened&&minimized&&restored&&raised&&
            captured&&released&&focusHidden&&focusRestored?0:1;
    } catch(const std::exception& error) { std::printf("ERROR %s last_error=%lu\n",error.what(),GetLastError()); }
    if(!Cleanup()) result=3;
    SetCursorPos(cursor.x,cursor.y); if(previous&&IsWindow(previous)) SetForegroundWindow(previous);
    return result;
}
