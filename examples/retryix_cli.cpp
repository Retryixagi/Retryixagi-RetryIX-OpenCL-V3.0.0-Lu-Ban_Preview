#ifdef RETRYIX_CLI_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <string>
#include <vector>
// DX11/Win32 樣板
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
void CreateRenderTarget() { ID3D11Texture2D* pBackBuffer; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)); g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView); pBackBuffer->Release(); }
void CleanupRenderTarget() { if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; } }
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static const char* kCmds[] = { "devices", "platforms", "atomic", "svm-stats", "export-json", "capabilities", "comm-resources", "resource-status", "benchmark" };
static std::string g_result;
void do_devices() { g_result.clear(); retryix_device_t* devs[32]; int ndev = 0; if (retryix_discover_all_devices(devs, 32, &ndev) == 0 && ndev > 0) { for (int i = 0; i < ndev; ++i) { char name[128]={0}, cu[32]={0}, gmem[32]={0}, lmem[32]={0}, caps[32]={0}, ext[1024]={0}; retryix_get_device_info(devs[i], "name", name, sizeof(name)); retryix_get_device_info(devs[i], "compute_units", cu, sizeof(cu)); retryix_get_device_info(devs[i], "global_mem", gmem, sizeof(gmem)); retryix_get_device_info(devs[i], "local_mem", lmem, sizeof(lmem)); retryix_get_device_info(devs[i], "capabilities", caps, sizeof(caps)); retryix_get_device_info(devs[i], "extensions", ext, sizeof(ext)); char buf[2048]; snprintf(buf, sizeof(buf), "[%d] %s | CU=%s | Mem=%s | Local=%s\n    Capabilities: %s\n    Extensions: %s\n", i, name, cu, gmem, lmem, caps, ext); g_result += buf; } } else { g_result = "No devices found.\n"; } }
void do_platforms() { g_result.clear(); retryix_platform_t* plats[16]; int nplat = 0; if (retryix_discover_platforms(plats, 16, &nplat) == 0 && nplat > 0) { for (int i = 0; i < nplat; ++i) { char name[128]={0}, vendor[128]={0}, ver[64]={0}, profile[64]={0}, devcnt[16]={0}; retryix_get_platform_info(plats[i], "name", name, sizeof(name)); retryix_get_platform_info(plats[i], "vendor", vendor, sizeof(vendor)); retryix_get_platform_info(plats[i], "version", ver, sizeof(ver)); retryix_get_platform_info(plats[i], "profile", profile, sizeof(profile)); retryix_get_platform_info(plats[i], "device_count", devcnt, sizeof(devcnt)); char buf[512]; snprintf(buf, sizeof(buf), "[%d] %s | vendor=%s | ver=%s | profile=%s | device_count=%s\n", i, name, vendor, ver, profile, devcnt); g_result += buf; } } else { g_result = "No platforms found.\n"; } }
void do_atomic() { g_result.clear(); retryix_device_t* devs[32]; int ndev = 0; if (retryix_discover_all_devices(devs, 32, &ndev) != 0 || ndev == 0) { g_result = "No devices found.\n"; return; } for (int i = 0; i < ndev; ++i) { int s32=0, s64=0, rc=retryix_check_atomic_support(devs[i], &s32, &s64); char name[128]={0}; retryix_get_device_info(devs[i], "name", name, sizeof(name)); char buf[256]; snprintf(buf, sizeof(buf), "Device [%d] %s: Atomic Int32=%s, Int64=%s (rc=%d)\n", i, name, s32?"YES":"NO", s64?"YES":"NO", rc); g_result += buf; } }
void do_svm_stats() { g_result.clear(); retryix_device_t* devs[32]; int ndev = 0; if (retryix_discover_all_devices(devs, 32, &ndev) != 0 || ndev == 0) { g_result = "No devices found.\n"; return; } for (int i = 0; i < ndev; ++i) { int svm_basic=0, svm_advanced=0, rc=retryix_check_svm_support(devs[i], &svm_basic, &svm_advanced); char name[128]={0}; retryix_get_device_info(devs[i], "name", name, sizeof(name)); char buf[256]; snprintf(buf, sizeof(buf), "Device [%d] %s: SVM basic=%s, advanced=%s (rc=%d)\n", i, name, svm_basic?"YES":"NO", svm_advanced?"YES":"NO", rc); g_result += buf; } }
void do_export_json() { g_result.clear(); retryix_device_t* devs[32]; int ndev = 0; if (retryix_discover_all_devices(devs, 32, &ndev) != 0 || ndev == 0) { g_result = "No devices found.\n"; return; } char json[16384]={0}; int rc = retryix_export_devices_json(devs, ndev, json, (int)sizeof(json)); if (rc == 0) g_result = json; else g_result = "export_devices_json failed."; }
void do_capabilities() { g_result.clear(); retryix_device_t* devs[32]; int ndev = 0; if (retryix_discover_all_devices(devs, 32, &ndev) != 0 || ndev == 0) { g_result = "No devices found.\n"; return; } for (int i = 0; i < ndev; ++i) { char name[128]={0}, caps[32]={0}, ext[1024]={0}; retryix_get_device_info(devs[i], "name", name, sizeof(name)); retryix_get_device_info(devs[i], "capabilities", caps, sizeof(caps)); retryix_get_device_info(devs[i], "extensions", ext, sizeof(ext)); char buf[2048]; snprintf(buf, sizeof(buf), "[%d] %s\n  Capabilities: %s\n  Extensions: %s\n", i, name, caps, ext); g_result += buf; } }
void do_comm_resources() { g_result = "comm resources: "; HMODULE h = LoadLibraryA("retryix.dll"); if (h) { FARPROC f1 = GetProcAddress(h, "comm_init"); FARPROC f2 = GetProcAddress(h, "comm_cleanup"); if (f1 && f2) g_result += "available\n"; else g_result += "not available\n"; FreeLibrary(h); } else g_result += "not available\n"; }
void do_resource_status() { int rc = retryix_get_system_state(); char buf[64]; snprintf(buf, sizeof(buf), "system state: rc=%d\n", rc); g_result = buf; }
void do_benchmark() { g_result.clear(); retryix_device_t* devs[32]; int ndev = 0; if (retryix_discover_all_devices(devs, 32, &ndev) != 0 || ndev == 0) { g_result = "No devices found.\n"; return; } for (int i = 0; i < ndev; ++i) { char name[128]={0}; retryix_get_device_info(devs[i], "name", name, sizeof(name)); int rc = retryix_run_device_benchmark(devs[i], NULL); char buf[256]; snprintf(buf, sizeof(buf), "Device [%d] %s: benchmark rc=%d\n", i, name, rc); g_result += buf; } }

int main_imgui() {
    retryix_init_minimal();
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"RetryIX ImGui CLI", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"RetryIX ImGui CLI", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);
    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); ::UnregisterClassW(wc.lpszClassName, wc.hInstance); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT); ::UpdateWindow(hwnd);
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark(); ImGuiStyle& style = ImGui::GetStyle(); style.ScaleAllSizes(main_scale); style.FontScaleDpi = main_scale; io.ConfigDpiScaleFonts = true; io.ConfigDpiScaleViewports = true;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { style.WindowRounding = 0.0f; style.Colors[ImGuiCol_WindowBg].w = 1.0f; }
    ImGui_ImplWin32_Init(hwnd); ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);
    int cmd_idx = 0; bool show_result = false; bool done = false;
    while (!done) {
        MSG msg; while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) { ::TranslateMessage(&msg); ::DispatchMessage(&msg); if (msg.message == WM_QUIT) done = true; }
        if (done) break;
        ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();
        ImGui::Begin("RetryIX CLI (ImGui)");
        ImGui::Text("選擇指令:");
        ImGui::Combo("Command", &cmd_idx, kCmds, IM_ARRAYSIZE(kCmds));
        if (ImGui::Button("執行查詢")) { switch (cmd_idx) { case 0: do_devices(); break; case 1: do_platforms(); break; case 2: do_atomic(); break; case 3: do_svm_stats(); break; case 4: do_export_json(); break; case 5: do_capabilities(); break; case 6: do_comm_resources(); break; case 7: do_resource_status(); break; case 8: do_benchmark(); break; } show_result = true; }
        if (show_result) { ImGui::InputTextMultiline("結果", (char*)g_result.c_str(), g_result.size()+1, ImVec2(600,400), ImGuiInputTextFlags_ReadOnly); }
        ImGui::End();
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.45f, 0.55f, 0.60f, 1.00f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) { ImGui::UpdatePlatformWindows(); ImGui::RenderPlatformWindowsDefault(); }
        g_pSwapChain->Present(1, 0);
    }
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); ::DestroyWindow(hwnd); ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    retryix_cleanup();
    return 0;
}
#endif
// RetryIX CLI: 只根據庫符號自動產生，完全可編譯可執行
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int retryix_result_t;
#ifndef RETRYIX_SUCCESS
#define RETRYIX_SUCCESS 0
#endif


// 只 forward declare 指標型態，所有查詢都用庫 API
typedef struct retryix_device retryix_device_t;
typedef struct retryix_platform retryix_platform_t;


retryix_result_t retryix_init_minimal(void);
void retryix_cleanup(void);
retryix_result_t retryix_discover_all_devices(retryix_device_t **, int, int*);
retryix_result_t retryix_discover_platforms(retryix_platform_t **, int, int*);
const char* retryix_get_error_string(retryix_result_t);
retryix_result_t retryix_check_atomic_support(const retryix_device_t*, int*, int*);
int retryix_export_devices_json(retryix_device_t **, int, char*, int);
retryix_result_t retryix_check_svm_support(const retryix_device_t*, int*, int*);
retryix_result_t retryix_device_supports_capability(const retryix_device_t*, int);
retryix_result_t retryix_get_system_state(void);
retryix_result_t retryix_run_device_benchmark(const retryix_device_t*, void*);
// 動態查詢設備屬性
retryix_result_t retryix_get_device_info(const retryix_device_t*, const char* key, char* out, int outlen);
retryix_result_t retryix_get_platform_info(const retryix_platform_t*, const char* key, char* out, int outlen);

static void cmd_devices(void) {
    retryix_device_t *devs[32]; int ndev = 0;
    if (retryix_discover_all_devices(devs, 32, &ndev) == RETRYIX_SUCCESS && ndev > 0) {
        for (int i = 0; i < ndev; ++i) {
            char name[128] = {0}, cu[32] = {0}, gmem[32] = {0}, lmem[32] = {0}, caps[32] = {0}, ext[1024] = {0};
            retryix_get_device_info(devs[i], "name", name, sizeof(name));
            retryix_get_device_info(devs[i], "compute_units", cu, sizeof(cu));
            retryix_get_device_info(devs[i], "global_mem", gmem, sizeof(gmem));
            retryix_get_device_info(devs[i], "local_mem", lmem, sizeof(lmem));
            retryix_get_device_info(devs[i], "capabilities", caps, sizeof(caps));
            retryix_get_device_info(devs[i], "extensions", ext, sizeof(ext));
            printf("[%d] %s | CU=%s | Mem=%s | Local=%s\n", i, name, cu, gmem, lmem);
            printf("    Capabilities: %s\n", caps);
            if (ext[0]) printf("    Extensions: %s\n", ext);
        }
    } else {
        printf("No devices found.\n");
    }
}

static void cmd_platforms(void) {
    retryix_platform_t *plats[16]; int nplat = 0;
    if (retryix_discover_platforms(plats, 16, &nplat) == RETRYIX_SUCCESS && nplat > 0) {
        for (int i = 0; i < nplat; ++i) {
            char name[128] = {0}, vendor[128] = {0}, ver[64] = {0}, profile[64] = {0}, devcnt[16] = {0};
            retryix_get_platform_info(plats[i], "name", name, sizeof(name));
            retryix_get_platform_info(plats[i], "vendor", vendor, sizeof(vendor));
            retryix_get_platform_info(plats[i], "version", ver, sizeof(ver));
            retryix_get_platform_info(plats[i], "profile", profile, sizeof(profile));
            retryix_get_platform_info(plats[i], "device_count", devcnt, sizeof(devcnt));
            printf("[%d] %s | vendor=%s | ver=%s | profile=%s | device_count=%s\n",
                i, name, vendor, ver, profile, devcnt);
        }
    } else {
        printf("No platforms found.\n");
    }
}

static void cmd_atomic(void) {
    retryix_device_t *devs[32]; int ndev = 0;
    if (retryix_discover_all_devices(devs, 32, &ndev) != RETRYIX_SUCCESS || ndev == 0) {
        puts("No devices found.");
        return;
    }
    for (int i = 0; i < ndev; ++i) {
        int s32 = 0, s64 = 0;
        int rc = retryix_check_atomic_support(devs[i], &s32, &s64);
        char name[128] = {0};
        retryix_get_device_info(devs[i], "name", name, sizeof(name));
        printf("Device [%d] %s: Atomic Int32=%s, Int64=%s (rc=%d)\n",
            i, name, s32 ? "YES" : "NO", s64 ? "YES" : "NO", rc);
    }
}

static void cmd_svm_stats(void) {
    retryix_device_t *devs[32]; int ndev = 0;
    if (retryix_discover_all_devices(devs, 32, &ndev) != RETRYIX_SUCCESS || ndev == 0) {
        puts("No devices found.");
        return;
    }
    for (int i = 0; i < ndev; ++i) {
        int svm_basic = 0, svm_advanced = 0;
        int rc = retryix_check_svm_support(devs[i], &svm_basic, &svm_advanced);
        char name[128] = {0};
        retryix_get_device_info(devs[i], "name", name, sizeof(name));
        printf("Device [%d] %s: SVM basic=%s, advanced=%s (rc=%d)\n",
            i, name, svm_basic ? "YES" : "NO", svm_advanced ? "YES" : "NO", rc);
    }
}

static void cmd_export_json(void) {
    retryix_device_t *devs[32]; int ndev = 0;
    if (retryix_discover_all_devices(devs, 32, &ndev) != RETRYIX_SUCCESS || ndev == 0) {
        puts("No devices found.");
        return;
    }
    char json[16384] = {0};
    int rc = retryix_export_devices_json(devs, ndev, json, (int)sizeof(json));
    if (rc == RETRYIX_SUCCESS) puts(json);
    else printf("export_devices_json failed: %d\n", rc);
}

static void cmd_capabilities(void) {
    retryix_device_t *devs[32]; int ndev = 0;
    if (retryix_discover_all_devices(devs, 32, &ndev) != RETRYIX_SUCCESS || ndev == 0) {
        puts("No devices found.");
        return;
    }
    for (int i = 0; i < ndev; ++i) {
        char name[128] = {0}, caps[32] = {0}, ext[1024] = {0};
        retryix_get_device_info(devs[i], "name", name, sizeof(name));
        retryix_get_device_info(devs[i], "capabilities", caps, sizeof(caps));
        retryix_get_device_info(devs[i], "extensions", ext, sizeof(ext));
        printf("[%d] %s\n  Capabilities: %s\n  Extensions: %s\n", i, name, caps, ext);
    }
}

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
static void cmd_comm_resources(void) {
    int ok = 0;
#ifdef _WIN32
    HMODULE h = LoadLibraryA("retryix.dll");
    if (h) {
        FARPROC f1 = GetProcAddress(h, "comm_init");
        FARPROC f2 = GetProcAddress(h, "comm_cleanup");
        if (f1 && f2) ok = 1;
        FreeLibrary(h);
    }
#else
    void* h = dlopen("libretryix.so", RTLD_LAZY);
    if (h) {
        void* f1 = dlsym(h, "comm_init");
        void* f2 = dlsym(h, "comm_cleanup");
        if (f1 && f2) ok = 1;
        dlclose(h);
    }
#endif
    printf("comm resources: %s\n", ok ? "available" : "not available");
}

static void cmd_resource_status(void) {
    int rc = retryix_get_system_state();
    printf("system state: rc=%d\n", rc);
}

static void cmd_benchmark(void) {
    retryix_device_t *devs[32]; int ndev = 0;
    if (retryix_discover_all_devices(devs, 32, &ndev) != RETRYIX_SUCCESS || ndev == 0) {
        puts("No devices found.");
        return;
    }
    for (int i = 0; i < ndev; ++i) {
        char name[128] = {0};
        retryix_get_device_info(devs[i], "name", name, sizeof(name));
        int rc = retryix_run_device_benchmark(devs[i], NULL);
        printf("Device [%d] %s: benchmark rc=%d\n", i, name, rc);
    }
}

int main() {
    if (retryix_init_minimal() != RETRYIX_SUCCESS) {
        printf("[error] retryix_init_minimal failed!\n");
        return 1;
    }
    puts("[RetryIX CLI] Commands: devices | platforms | atomic | svm-stats | export-json | capabilities | comm-resources | resource-status | benchmark | quit");
    char line[128];
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        size_t n = strlen(line);
        while (n && (line[n-1]=='\n'||line[n-1]=='\r')) line[--n]=0;
        if (!n) continue;
        if (!strcmp(line, "quit") || !strcmp(line, "exit")) break;
        else if (!strcmp(line, "devices")) cmd_devices();
        else if (!strcmp(line, "platforms")) cmd_platforms();
        else if (!strcmp(line, "atomic")) cmd_atomic();
        else if (!strcmp(line, "svm-stats")) cmd_svm_stats();
        else if (!strcmp(line, "export-json")) cmd_export_json();
        else if (!strcmp(line, "capabilities")) cmd_capabilities();
        else if (!strcmp(line, "comm-resources")) cmd_comm_resources();
        else if (!strcmp(line, "resource-status")) cmd_resource_status();
        else if (!strcmp(line, "benchmark")) cmd_benchmark();
        else printf("Unknown command: %s\n", line);
    }
    retryix_cleanup();
    return 0;
}
