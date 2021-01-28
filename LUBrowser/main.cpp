#define CINTERFACE
#define _CRT_SECURE_NO_WARNINGS
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#include <Windows.h>
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#endif
#include <GLFW/glfw3.h>
#pragma comment(lib,"glfw3.lib")
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"Psapi.lib")
#include <cstring>
#if defined(_MSC_VER) && (_MSC_VER >= 1900) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS)
#pragma comment(lib, "legacy_stdio_definitions")
#endif

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


char ip[64];
char port[8];
char nickname[16] = { 0 };

#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <stdlib.h>
#include <Psapi.h>

bool GetProcessIdFromProcessName(char* szProcessName, DWORD* dwProcessId)
{
    bool bReturn = false;

    HANDLE hProcessSnapShot = CreateToolhelp32Snapshot(0x00000002, 0);

    if (!hProcessSnapShot)
        return false;

    PROCESSENTRY32 ProcessEntry;
    ProcessEntry.dwSize = sizeof(ProcessEntry);

    if (Process32First(hProcessSnapShot, &ProcessEntry))
    {
        while (Process32Next(hProcessSnapShot, &ProcessEntry))
        {
            if (!strcmp(ProcessEntry.szExeFile, szProcessName))
            {
                *dwProcessId = ProcessEntry.th32ProcessID;
                bReturn = true;
                break;
            }
        }
    }

    CloseHandle(hProcessSnapShot);
    return bReturn;
}

bool TerminateGTAIfRunning(void)
{
    DWORD dwProcessIDs[250];
    DWORD pBytesReturned = 0;
    unsigned int uiListSize = 50;
    if (EnumProcesses(dwProcessIDs, 250 * sizeof(DWORD), &pBytesReturned))
    {
        for (unsigned int i = 0; i < pBytesReturned / sizeof(DWORD); i++)
        {
            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, dwProcessIDs[i]);
            if (hProcess)
            {
                HMODULE pModule;
                DWORD cbNeeded;
                if (EnumProcessModules(hProcess, &pModule, sizeof(HMODULE), &cbNeeded))
                {
                    char szModuleName[500];
                    if (GetModuleFileNameExA(hProcess, pModule, szModuleName, 500))
                    {
                        if (_strcmpi(szModuleName + strlen(szModuleName) - strlen("gta3.exe"), "gta3.exe") == 0)
                        {
                            if (MessageBox(0, "An instance of GTA3 is already running. It needs to be terminated before Liberty Unleashed can be started. Do you want to do that now?", "Error", MB_YESNO | MB_ICONQUESTION) == IDYES)
                            {
                                TerminateProcess(hProcess, 0);
                                CloseHandle(hProcess);
                                return true;
                            }
                            return false;
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        }
    }
    return true;
}

bool isdbg = false;

bool InjectLibraryIntoProcess(HANDLE hProcess, char* szLibraryPath)
{
    bool bReturn = true;
    size_t sLibraryPathLen = (strlen(szLibraryPath) + 1);
    void* pRemoteLibraryPath = VirtualAllocEx(hProcess, NULL, sLibraryPathLen, MEM_COMMIT, PAGE_READWRITE);
    SIZE_T sBytesWritten = 0;
    WriteProcessMemory(hProcess, pRemoteLibraryPath, (void*)szLibraryPath, sLibraryPathLen, &sBytesWritten);

    if (sBytesWritten != sLibraryPathLen)
    {
        MessageBoxA(NULL, "Failed to write library path into remote process.", NULL, NULL);
        bReturn = false;
    }
    else
    {
        HMODULE hKernel32 = GetModuleHandle("Kernel32");
        FARPROC pfnLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pfnLoadLibraryA, pRemoteLibraryPath, 0, NULL);

        if (hThread) {
            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
        else
        {
            MessageBoxA(NULL, "Failed to create remote thread in remote process.", NULL, NULL);
            bReturn = false;
        }
    }
    VirtualFreeEx(hProcess, pRemoteLibraryPath, sizeof(pRemoteLibraryPath), MEM_RELEASE);
    return bReturn;
}
char szParams[1024];

void ImStyl()
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;

    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.44f, 0.44f, 0.44f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    colors[ImGuiCol_Border] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.81f, 0.83f, 0.81f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.93f, 0.65f, 0.14f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.51f, 0.36f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.91f, 0.64f, 0.13f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.78f, 0.55f, 0.21f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14f, 0.26f, 0.42f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

    style->FramePadding = ImVec2(4, 2);
    style->ItemSpacing = ImVec2(10, 2);
    style->IndentSpacing = 12;
    style->ScrollbarSize = 10;

    style->WindowRounding = 4;
    style->FrameRounding = 4;
    style->PopupRounding = 4;
    style->ScrollbarRounding = 6;
    style->GrabRounding = 4;
    style->TabRounding = 4;

    style->WindowTitleAlign = ImVec2(1.0f, 0.5f);
    style->WindowMenuButtonPosition = ImGuiDir_Right;

    style->DisplaySafeAreaPadding = ImVec2(4, 4);
}

int main(int, char**)
{ 
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    remove("imgui.ini");

    HKEY hKey = 0;
    char szBuf[256];

    RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\GTA3LU", 0, KEY_READ, &hKey);

    if (hKey) {

        DWORD dwSize = 256;
        DWORD dwType = REG_SZ;

        RegQueryValueEx(hKey, "nick", 0, &dwType, (PUCHAR)szBuf, &dwSize);
        if (dwSize)
        {
            sprintf(nickname, "%s", szBuf);
        }

        dwSize = 256;
        dwType = REG_SZ;

        RegQueryValueEx(hKey, "server", 0, &dwType, (PUCHAR)szBuf, &dwSize);
        if (dwSize) {
            sprintf(ip, "%s", szBuf);
        }

        dwSize = 256;
        dwType = REG_SZ;

        RegQueryValueEx(hKey, "port", 0, &dwType, (PUCHAR)szBuf, &dwSize);
        if (dwSize) {
            sprintf(port, "%s", szBuf);
        }

        RegCloseKey(hKey);
    }

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(0x00021010, 0);
    
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(200,100, "Liberty Unleashed 0.1 Server Browser",NULL, NULL);
    
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);

    IMGUI_CHECKVERSION(); 
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();
    ImStyl();
    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowPos(ImVec2(1.0f, 1.0f));
            ImGui::Begin("LU",NULL,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoTitleBar);                          // Create a window called "Hello, world!" and append into it.

           
            ImGui::PushItemWidth(100);
            ImGui::InputText("Nickname", nickname, IM_ARRAYSIZE(nickname));
            ImGui::PushItemWidth(100);
            ImGui::InputText("IP", ip, IM_ARRAYSIZE(ip));
            ImGui::PushItemWidth(100);
            ImGui::InputText("Port", port, IM_ARRAYSIZE(port));
            

            if (ImGui::Button("Connect"))
            {
                HKEY hKey = 0;
                RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\GTA3LU", 0, KEY_WRITE, &hKey);
                
                if (!hKey) {
                    RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\GTA3LU", &hKey);
                }
                if (hKey) {
                    DWORD dwSize = sizeof(nickname);
                    DWORD dwType = REG_SZ;
                    RegSetValueEx(hKey, "nick", 0, dwType, (PUCHAR)nickname, dwSize);
                    dwSize = sizeof(ip);
                    dwType = REG_SZ;
                    RegSetValueEx(hKey, "server", 0, dwType, (PUCHAR)ip, dwSize);
                    dwSize = sizeof(port);
                    dwType = REG_SZ;
                    RegSetValueEx(hKey, "port", 0, dwType, (PUCHAR)port, dwSize);
                    RegCloseKey(hKey);
                }
                if (sizeof(nickname) == 0) { break; }
                if (isdbg) { sprintf(szParams, "\"gta3.exe\" -h %s -p %s -n %s -d", ip, port, nickname); }
                else
                {
                    sprintf(szParams, "\"gta3.exe\" -h %s -p %s -n %s", ip, port, nickname);
                }
                char szLibraryPath[1024];
                sprintf(szLibraryPath, "lu.dll");
                char szGtaExe[1024];
                sprintf(szGtaExe, "gta3.exe");
                STARTUPINFO siStartupInfo;
                PROCESS_INFORMATION piProcessInfo;
                memset(&siStartupInfo, 0, sizeof(siStartupInfo));
                memset(&piProcessInfo, 0, sizeof(piProcessInfo));
                siStartupInfo.cb = sizeof(siStartupInfo);
                if (!CreateProcess(szGtaExe, szParams, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &siStartupInfo,
                    &piProcessInfo)) {
                    MessageBoxA(NULL,"Couldn't launch gta3.exe.\nDid you install Liberty Unleashed to your GTA3 directory?","Error",NULL);
                    
                }
                if (!TerminateGTAIfRunning())
                {
                    MessageBoxA(NULL,"Liberty Unleashed could not start because an another instance of GTA3 is running.", "Error", MB_ICONERROR);
                }

                if (!InjectLibraryIntoProcess(piProcessInfo.hProcess, szLibraryPath)) {
                    TerminateProcess(piProcessInfo.hProcess, 0);
                    MessageBoxA(NULL, "Failed to inject lu.dll", "Fatal Error", 0);
                }
                ResumeThread(piProcessInfo.hThread);
            }
            ImGui::SameLine(); ImGui::Checkbox("Console", &isdbg);
            ImGui::End();
        }

        
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(0x000040000);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwMakeContextCurrent(window);
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
