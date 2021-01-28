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

    HANDLE hProcessSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

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
                    if (GetModuleFileNameEx(hProcess, pModule, szModuleName, 500))
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
int main(int, char**)
{
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
    
    ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
    // Setup window

    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_DOUBLEBUFFER, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(300,300, "Liberty Unleashed 0.1 Server Browser", NULL, NULL);
    
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION(); 
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsClassic();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::Begin("Hello, world!",NULL,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoTitleBar);                          // Create a window called "Hello, world!" and append into it.

           
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
                sprintf(szParams, "\"gta3.exe\" -h %s -p %s -n %s", ip,port,nickname);
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
           
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

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
