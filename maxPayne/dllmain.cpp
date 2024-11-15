// Standard includes.
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "pch.h"
#include "memVals.h"
#include "resource.h"

// Input includes & libs.
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <Xinput.h>
#pragma comment(lib, "Dinput8.lib")
#pragma comment (lib, "Dxguid.lib")
#pragma comment(lib, "XInput.lib")

// MinHook includes & libs.
#include <MinHook.h>
#if _WIN64
#pragma comment(lib, "libMinHook.x64.lib")
#else
#pragma comment(lib, "libMinHook.x86.lib")
#endif

// Resource includes & libs.
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// GLOBAL VARS.
HMODULE myhModule;      // Handle for this DLL.
std::atomic<bool> isGameManagerInitialized(false);

// idk bro.
bool wasL3Pressed = false;
bool wasR3Pressed = false;

// Func. for checking if L3 and R3 are pressed simultaneously.
bool stickCheck() {
    XINPUT_STATE state;
    if (XInputGetState(0, &state) == ERROR_SUCCESS) { // Assuming controller 0
        bool isL3Pressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
        bool isR3Pressed = state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;

        // Check if both L3 and R3 are pressed in the same cycle and were not previously pressed
        if (isL3Pressed && isR3Pressed && !wasL3Pressed && !wasR3Pressed) {
            wasL3Pressed = true;
            wasR3Pressed = true;
            return true;
        }

        // Update previous states for the next check
        wasL3Pressed = isL3Pressed;
        wasR3Pressed = isR3Pressed;
    }
    return false;
}

// Func. for ending a thread.
DWORD _stdcall EjectThread(LPVOID lpParam) {
    Sleep(100);
    FreeLibraryAndExitThread(myhModule, 0);
    return 0;
}

// Cleanup function for console & running thread.
void cleanup(FILE* dn) {

    MH_Uninitialize();

    std::cout << "Closing test console\n" << std::endl;
    Sleep(2000);
    if (dn != nullptr) {
        fclose(dn);
    }
    FreeConsole();

    CreateThread(0, 0, EjectThread, 0, 0, 0);
    return;
}

// Initialization function that returns GameManagerImp only when the game is loaded.
UINT64 initializeGameManager() {

    HMODULE g_moduleAddr = nullptr;
    UINT64 GameManagerImp = 0;

    while (g_moduleAddr == nullptr) {
        g_moduleAddr = GetModuleHandleA("DarkSoulsII.exe");
        Sleep(100);
    }

    while (GameManagerImp == 0) {
        GameManagerImp = *(UINT64*)((UINT64)g_moduleAddr + 0x16148F0);

        if (GameManagerImp != 0) {
            isGameManagerInitialized = true;
            break;
        }

        Sleep(100);
    }

    return GameManagerImp;
}

// Function to handle startup process for DLL injection.
bool dllStartup() {

    // Load original DLL.
    char dllpath[320];
    bool init_hook = false;
    GetSystemDirectoryA(dllpath, 320);
    strcat_s(dllpath, "\\dinput8.dll");
    HMODULE hinst_dll = LoadLibraryA(dllpath);

    if (!hinst_dll)
    {
        //std::cout << "Failed to load original DLL.\n" << std::endl;
        return false;
    }
    else {
        //std::cout << "Successfully loaded original DLL.\n" << std::endl;

        // Retrieve og add. of the DirectInput8Create function from dinput8.dll.
        auto original_dinput8_create = (decltype(&DirectInput8Create))GetProcAddress(hinst_dll, "DirectInput8Create");
        if (!original_dinput8_create)
        {
           // std::cout << "Failed to load original DirectInput8Create address. \n" << std::endl;
            return false;
        }
        else {
            //std::cout << "Successfully loaded original DirectInput8Create address. \n" << std::endl;
        }
    }

    // Initialize MinHook
    MH_STATUS status = MH_Initialize();
    if (status != MH_OK)
    {
        std::string sStatus = MH_StatusToString(status);
        //std::cout << "Minhook init failed.\n" << std::endl;
    }
    else {
        //std::cout << "Minhook init successful.\n" << std::endl;
    }

    // Free the console and return true.
    //FreeConsole();
    return true;
}

// Function for handling all timer-specific actions during Max Payne mode.
void payneClock(float ogAlpha, float* vignetteAlpha) {
    int duration_ms = 15000; // 15 seconds
    int rampUpDuration_ms = 250; // 1 second
    int rampDownDuration_ms = 250; // 1 second

    float targetVignetteAlpha = 1.0f;  // TEST

    // Get the current time point & calculate interpolation times & end time.
    auto start_time = std::chrono::high_resolution_clock::now();
    auto end_time = start_time + std::chrono::milliseconds(duration_ms);
    auto ramp_up_end_time = start_time + std::chrono::milliseconds(rampUpDuration_ms);
    auto ramp_down_start_time = end_time - std::chrono::milliseconds(rampDownDuration_ms);

    // Gradually increase vignetteAlpha to 1.0 over the first second (allows for smooth transition).
    while (std::chrono::high_resolution_clock::now() < ramp_up_end_time) {

        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - start_time).count(); // Time in seconds.
        *vignetteAlpha = ogAlpha + (targetVignetteAlpha - ogAlpha) * (elapsed / 1.0f); // Linear interpolation.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Ensure vignetteAlpha is set to 1.0 after ramp up.
    *vignetteAlpha = targetVignetteAlpha;

    // Main timer loop for the bulk of remaining time.
    while (std::chrono::high_resolution_clock::now() < ramp_down_start_time) {

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 100ms to avoid busy waiting
    }

    // Gradually decrease vignetteAlpha back to default during last second.
    while (std::chrono::high_resolution_clock::now() < end_time) {

        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - ramp_down_start_time).count(); // Time in seconds.
        *vignetteAlpha = targetVignetteAlpha - (targetVignetteAlpha - ogAlpha) * (elapsed / 1.0f); // Linear interpolation.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Function for controlling max payne functionality.
void maxPayne(defaultVals defV, newVals newV, HMODULE ds2Add, UINT64 GameManagerImp) {
    float* camSpeed = (float*)((UINT64)ds2Add + 0x10ACB0C);
    float* gameSpeed = (float*)((UINT64)ds2Add + 0xB4A5E0);
    float* vignetteAlpha = (float*)(GameManagerImp - 0x8FACC4);
    float* playerSpeed = (float*)(GameManagerImp + 0x3699A88);
    int* animationState = (int*)(GameManagerImp + 0x377F1D0);

    // Record original vignetteAlpha & playerSpeed values.
    float ogAlpha = *vignetteAlpha;
    float ogSpeed = *playerSpeed;

    // Change speed & vignetteAlpha values to max payne mode, & play sound.
    *camSpeed = newV.getPayneCamSpeed();
    *gameSpeed = newV.getPayneGameSpeed();
    *playerSpeed = 1.75f;
    PlaySound(MAKEINTRESOURCE(IDR_SOUND1), GetModuleHandleA("maxPayne.dll"), SND_RESOURCE | SND_ASYNC);

    // Start timer.
    payneClock(ogAlpha, vignetteAlpha);

    // Change speed & vignetteAlpha values back to default.
    *camSpeed = defV.getDefaultCamSpeed();
    *gameSpeed = defV.getDefaultGameSpeed();
    *vignetteAlpha = ogAlpha;
    *playerSpeed = ogSpeed;
}

// Main application loop.
DWORD WINAPI MainThread(LPVOID lpParam) {

    // Create a console for testing purposes.
    AllocConsole();
    SetConsoleTitleA("DS2 Testing Console");

    // Redirect stdout to the console. This also puts modengine's output into this console.
    FILE* pNewStdout = nullptr;
    ::freopen_s(&pNewStdout, "CONOUT$", "w", stdout);
    std::cout.clear();
    std::wcout.clear();

    // Output intro message for console.
    std::cout << "Welcome to the test console for DS2!\n" << std::endl;

    // Initialize general use addresses.
    HMODULE g_moduleAddr = GetModuleHandleA("DarkSoulsII.exe");
    UINT64 GameManagerImp = initializeGameManager();

    // Initialize memVals objects.
    defaultVals defaultVals;
    newVals newVals;

    while (true) {
        Sleep(50);
         
        // If user presses the "UP" key, trigger max payne mode. PLACEHOLDER, the trigger for this will be changed later.
        if (stickCheck()) {
            maxPayne(defaultVals, newVals, g_moduleAddr, GameManagerImp);
        }

        //GetAsyncKeyState(VK_UP) & 1
    }

    cleanup(pNewStdout);
    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (dllStartup()) {
            DisableThreadLibraryCalls(hModule);
            CreateThread(0, 0, &MainThread, nullptr, 0, nullptr);
        }
        else {
            break;
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

