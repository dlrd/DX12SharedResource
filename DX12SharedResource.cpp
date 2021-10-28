// Copyright (c) 2016, NVIDIA CORPORATION.  All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.  Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include <stdio.h>
#include "DX12Present.h"
#include "DX12SharedData.h"
#include "SmodeErrorAndAssert.h"

enum RuntimeMode {
  SINGLE_THREADED,
  MULTI_THREADED,
  CROSS_PROCESS,
};

#define MIN_SHARED_BUFFERS   2

class DX12SharedResource
{
protected:
  LPCSTR m_program = nullptr;
  RuntimeMode m_mode;
  bool m_initialized = false;
  HINSTANCE m_hInstance = nullptr;
  HANDLE m_hThread = nullptr;
  HANDLE m_hMapFile = nullptr;
  class DX12Present* m_dxPresent = nullptr;
  LARGE_INTEGER m_frequency = { 0, };
  LARGE_INTEGER m_startTime = { 0, };
  LARGE_INTEGER m_stopTime = { 0, };
  UINT m_numSharedBuffers = 0;
  UINT m_numFrames = 0;
  UINT m_duration = 0;
  UINT m_elapsed = 0;
  UINT m_status = 0;
  //bool m_verify = 0;
  bool m_vsync = false;
  bool m_forceDedicatedMemory = false;
  //UINT m_captureFrame = 0;
  //LPCSTR m_captureFile = nullptr;
  HANDLE startEvent = nullptr;
  HANDLE doneEvent = nullptr;
  double m_fps = 0.0;
  struct DX12SharedData* m_pSharedData = nullptr;
  class AbstractRender* m_vkRender = nullptr;

public:
  DX12SharedResource(HINSTANCE hInstance, LPCSTR lpszProgram, UINT numSharedBuffers, UINT duration, RuntimeMode mode, bool vsync, /*bool verify,*/ bool dedicated/*, UINT captureFrame, LPCSTR captureFile*/);
  ~DX12SharedResource();

  UINT GetStatus() { return m_status; }
  double GetFPS() { return m_fps; }
  void InitSharedData(HWND hWnd, UINT width, UINT height);
  bool Init(HWND hWnd, UINT width, UINT height);
  void Cleanup();
  void Render();
};

#define VK_DX12_SHARED_RESOURCE L"DX12SharedResource"
#define VK_DX12_SHARED_RESOURCE_CLIENT_ARG "DX12SharedResource$egahasu64167ghfggfadsd51545gjja66717615gsdfgajhjhsghdfghsjk$"

DX12SharedResource::DX12SharedResource(HINSTANCE hInstance, LPCSTR lpszProgram, UINT numSharedBuffers, UINT duration, RuntimeMode mode, bool vsync, /*bool verify,*/ bool dedicated/*, UINT captureFrame, LPCSTR captureFile*/)
{
  m_program = lpszProgram;
  m_hInstance = hInstance;
  m_numSharedBuffers = numSharedBuffers;
  //m_verify = verify;
  m_vsync = vsync;
  m_forceDedicatedMemory = dedicated;
  m_mode = mode;
  m_duration = duration;
  //m_captureFrame = captureFrame;
  //m_captureFile = captureFile;
}

void DX12SharedResource::InitSharedData(HWND hWnd, UINT width, UINT height)
{
  m_pSharedData->currentBufferIndex = 0;
  m_pSharedData->numSharedBuffers = m_numSharedBuffers;
  //m_pSharedData->verify = m_verify;
  m_pSharedData->vsync = m_vsync;
  m_pSharedData->forceDedicatedMemory = m_forceDedicatedMemory;
  //m_pSharedData->captureFile = m_captureFile;
  //m_pSharedData->captureFrame = m_captureFrame;
  m_pSharedData->hWnd = hWnd;
  m_pSharedData->width = width;
  m_pSharedData->height = height;
  m_pSharedData->startEvent = startEvent;
  m_pSharedData->doneEvent = doneEvent;
  m_pSharedData->terminate = false;
  m_pSharedData->terminated = false;
}

DX12SharedResource::~DX12SharedResource()
{
    Cleanup();
}

static DWORD WINAPI PresentThread(void* param)
{
    DX12SharedData* pSharedData = reinterpret_cast<DX12SharedData*>(param);
    DWORD retVal = 1;

    WaitForSingleObject(pSharedData->startEvent, INFINITE);
    DX12Present* dxPresent = new DX12Present();

    if (dxPresent->Init(pSharedData)) {
        SetEvent(pSharedData->doneEvent);

        while (1) {
            WaitForSingleObject(pSharedData->startEvent, INFINITE);

            if (pSharedData->terminate) {
                //dxPresent->WaitForCompletion();
                SetEvent(pSharedData->doneEvent);
                WaitForSingleObject(pSharedData->startEvent, INFINITE);
                retVal = 0;
                break;
            }

            if (!dxPresent->Render()) {
                break;
            }

            SetEvent(pSharedData->doneEvent);
        }
    }

    dxPresent->Cleanup();
    delete dxPresent;

    if (retVal) {
        pSharedData->terminate = true;

        SetEvent(pSharedData->doneEvent);
    }

    pSharedData->terminated = true;

    return retVal;
}

bool DX12SharedResource::Init(HWND hWnd, UINT width, UINT height)
{
    switch (m_mode) {
    case SINGLE_THREADED:
        {
            m_pSharedData = new DX12SharedData;
            InitSharedData(hWnd, width, height);

            m_vkRender = newAbstractRender();
            m_dxPresent = new DX12Present();

            if (!m_dxPresent->Init(m_pSharedData)) {
                return false;
            }

            if (!m_vkRender->Init(m_pSharedData)) {
                return false;
            }
        }
        break;
    case MULTI_THREADED:
        {
            m_pSharedData = new DX12SharedData;

            startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            assert(startEvent);

            doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            assert(doneEvent);

            InitSharedData(hWnd, width, height);

            m_hThread = CreateThread(NULL, NULL, PresentThread, (void*)m_pSharedData, NULL, NULL);

            SetEvent(startEvent);
            WaitForSingleObject(doneEvent, INFINITE);

            if (m_pSharedData->terminate) {
                return false;
            }

            m_vkRender = newAbstractRender();

            if (!m_vkRender->Init(m_pSharedData)) {
                return false;
            }
        }
        break;
    default: // CROSS_PROCESS
        {
            m_hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(DX12SharedData), VK_DX12_SHARED_RESOURCE);
            m_pSharedData = reinterpret_cast<DX12SharedData*>(MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DX12SharedData)));

            startEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            assert(startEvent);

            doneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            assert(doneEvent);

            m_dxPresent = new DX12Present();

            InitSharedData(hWnd, width, height);

            if (!m_dxPresent->Init(m_pSharedData)) {
                return false;
            }

            char cmdLine[256];
            sprintf_s(cmdLine, "%s %s", m_program, VK_DX12_SHARED_RESOURCE_CLIENT_ARG);

            STARTUPINFOA si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));                        
            CreateProcessA(NULL,   // No module name (use command line)
                cmdLine,        // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                TRUE,          // Set handle inheritance to FALSE
                0,              // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory 
                &si,            // Pointer to STARTUPINFO structure
                &pi);           // Pointer to PROCESS_INFORMATION structure

            WaitForSingleObject(doneEvent, INFINITE);

            if (m_pSharedData->terminate) {
                return false;
            }
        }
    }

    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_startTime);

    m_numFrames = 0;

    return true;
}

void DX12SharedResource::Cleanup()
{
    if (m_hThread) {
        if (m_pSharedData->terminate) {
            if (m_vkRender) {
                m_vkRender->Cleanup();
                delete m_vkRender;
                m_vkRender = 0;
            }
        } else {
            m_pSharedData->terminate = true;
            SetEvent(startEvent);

            WaitForSingleObject(doneEvent, INFINITE);

            if (m_vkRender) {
                m_vkRender->Cleanup();
                delete m_vkRender;
                m_vkRender = 0;
            }

            SetEvent(startEvent);
        }

        WaitForSingleObject(m_hThread, INFINITE);

        assert(m_pSharedData->terminated);

        m_hThread = 0;
    } else if (m_mode == CROSS_PROCESS) {
        if (m_pSharedData) {
            if (!m_pSharedData->terminate) {
                m_pSharedData->terminate = true;
                SetEvent(startEvent);
            }

            volatile bool* pTerminated = &m_pSharedData->terminated;
            while (!*pTerminated) {
                Sleep(1);
            }
        }

        if (m_dxPresent) {
            m_dxPresent->Cleanup();
            delete m_dxPresent;
            m_dxPresent = 0;
        }
    } else {
        if (m_vkRender) {
            m_vkRender->Cleanup();
            delete m_vkRender;
            m_vkRender = 0;
        }

        if (m_dxPresent) {
            m_dxPresent->Cleanup();
            delete m_dxPresent;
            m_dxPresent = 0;
        }
    }

    if (startEvent) {
        CloseHandle(startEvent);
        startEvent = 0;
    }

    if (doneEvent) {
        CloseHandle(doneEvent);
        doneEvent = 0;
    }

    if (m_mode == CROSS_PROCESS) {
        if (m_pSharedData) {
            UnmapViewOfFile(m_pSharedData);
        }
        if (m_hMapFile) {
            CloseHandle(m_hMapFile);
            m_hMapFile = 0;
        }
    } else {
        delete m_pSharedData;
    }

    m_pSharedData = 0;

    m_numSharedBuffers = 0;
}

void DX12SharedResource::Render()
{
    bool terminate = false;
    bool initialized = false;

    if (m_mode == CROSS_PROCESS){
        initialized = !m_pSharedData->terminate;
    } else {
        initialized = m_vkRender && m_vkRender->Initialized();
    }

    if (initialized) {
        switch (m_mode) {
        case SINGLE_THREADED:
            m_vkRender->Render();
            if (!m_dxPresent->Render()) {
                fprintf(stderr, "Incorrect Render Data\n");
                m_status = 1;
                terminate = true;
            }
            break;
        case MULTI_THREADED:
            m_vkRender->Render();
            SetEvent(startEvent);
            WaitForSingleObject(doneEvent, INFINITE);
            if (m_pSharedData->terminate) {
                fprintf(stderr, "Incorrect Render Data\n");
                m_status = 1;
                terminate = true;
            }
            break;
        default: // CROSS_PROCESS
            SetEvent(startEvent);
            WaitForSingleObject(doneEvent, INFINITE);
            if (m_pSharedData->terminate) {
                terminate = true;
            } else {
                if (!m_dxPresent->Render()) {
                    fprintf(stderr, "Incorrect Render Data\n");
                    m_status = 1;
                    terminate = true;
                }
            }
        }

        m_numFrames++;
/*        if (m_pSharedData->captureFile) {
            if (m_pSharedData->captureFrame == m_numFrames - 1) {
                terminate = true;
            }
        } else*/ {
            QueryPerformanceCounter(&m_stopTime);
            DWORD ms = (DWORD)((m_stopTime.QuadPart - m_startTime.QuadPart) * 1000 / m_frequency.QuadPart);
            if (ms > 1000) {
                TCHAR header[128];
                m_fps = (double)m_numFrames / (double)ms * 1000.;
                m_numFrames = 0;
                swprintf_s(header, L"DX12SharedResource - %5u fps (%u shared buffer(s)/%s)", 
                    (DWORD)m_fps, m_pSharedData->numSharedBuffers, 
                    m_mode == MULTI_THREADED ? L"multi-threaded" : (m_mode == SINGLE_THREADED ? L"single-threaded" : L"cross-process"));
                SetWindowText(m_pSharedData->hWnd, header);
                QueryPerformanceCounter(&m_startTime);
                m_elapsed++;

                if (m_duration && (m_elapsed >= m_duration)) {
                    terminate = true;
                }
            }
        }
    } else {
        m_status = 1;
        terminate = true;
    }

    if (terminate) {
        SendMessage(m_pSharedData->hWnd, WM_CLOSE, 0, 0);
    }
}

LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            DX12SharedResource* pSharedResource = reinterpret_cast<DX12SharedResource*>(pCreateStruct->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pSharedResource));
            RECT rc;
            GetClientRect(hWnd, &rc);
            if (!pSharedResource->Init(hWnd, rc.right - rc.left, rc.bottom - rc.top)) {
                return FALSE;
            }
            return TRUE;
        }
    case WM_CLOSE:
        {
            DX12SharedResource* pSharedResource = reinterpret_cast<DX12SharedResource*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            pSharedResource->Cleanup();
            DestroyWindow(hWnd);
            PostQuitMessage(0);
            return TRUE;
        }
    case WM_DESTROY:
        return TRUE;
    case WM_ERASEBKGND:
        break;
    case WM_PAINT:
        {
            DX12SharedResource* pSharedResource = reinterpret_cast<DX12SharedResource*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
            PAINTSTRUCT ps;
            BeginPaint(hWnd, &ps);
            pSharedResource->Render();
            EndPaint(hWnd, &ps);
            return TRUE;
        }
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

static void IdleProc(HWND hWnd)
{
    DX12SharedResource* pSharedResource = reinterpret_cast<DX12SharedResource*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    pSharedResource->Render();
    ValidateRect(hWnd, NULL);
}

typedef struct _Config {
    UINT windowWidth = 1024;
    UINT windowHeight = 768;
    RuntimeMode mode = SINGLE_THREADED;
    UINT numBuffers = 3;
    UINT duration = 0;
    bool vsync = false;
  /*  bool validate = false;*/
    bool dedicated = false;
    //UINT captureFrame = 0;
    //LPCSTR captureFile = NULL;
} Config;

static HWND InitWindow(HINSTANCE hInstance, Config* pConfig, DX12SharedResource* pSharedResource)
{
    DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    DWORD dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    RECT windowRect;
    windowRect.left = 0;
    windowRect.right = pConfig->windowWidth;
    windowRect.top = 0;
    windowRect.bottom = pConfig->windowHeight;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

    HWND hWnd = CreateWindowEx(dwExStyle,
                               VK_DX12_SHARED_RESOURCE,
                               VK_DX12_SHARED_RESOURCE,
                               dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               windowRect.right - windowRect.left,
                               windowRect.bottom - windowRect.top,
                               NULL,
                               NULL,
                               hInstance,
                               pSharedResource);

    if (!hWnd) {
        return NULL;
    }

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

    return hWnd;
}

static int render()
{
    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, VK_DX12_SHARED_RESOURCE);

    DX12SharedData* pSharedData = reinterpret_cast<DX12SharedData*>(MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(DX12SharedData)));

    DWORD pid;
    GetWindowThreadProcessId(pSharedData->hWnd, &pid);
    HANDLE remoteProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
    HANDLE startEvent = 0;
    HANDLE doneEvent  = 0;
    DuplicateHandle(remoteProcess, pSharedData->startEvent, GetCurrentProcess(), &startEvent, 0, TRUE, DUPLICATE_SAME_ACCESS);
    DuplicateHandle(remoteProcess, pSharedData->doneEvent, GetCurrentProcess(), &doneEvent, 0, TRUE, DUPLICATE_SAME_ACCESS);

    for (UINT i = 0; i < pSharedData->numSharedBuffers; i++) {
        HANDLE hRemoteSharedMemHandle = pSharedData->sharedMemHandle[i];
        pSharedData->sharedMemHandle[i] = 0;
        DuplicateHandle(remoteProcess, hRemoteSharedMemHandle, GetCurrentProcess(), &pSharedData->sharedMemHandle[i], 0, TRUE, DUPLICATE_SAME_ACCESS);
        assert(pSharedData->sharedMemHandle[i]);

        HANDLE hRemoteSharedFenceHandle = pSharedData->sharedFenceHandle[i];
        pSharedData->sharedFenceHandle[i] = 0;
        DuplicateHandle(remoteProcess, hRemoteSharedFenceHandle, GetCurrentProcess(), &pSharedData->sharedFenceHandle[i], 0, TRUE, DUPLICATE_SAME_ACCESS);
        assert(pSharedData->sharedFenceHandle[i]);
    }

    auto* vkRender = newAbstractRender();

    if (vkRender->Init(pSharedData)) {
        SetEvent(doneEvent);

        while (1) {
            WaitForSingleObject(startEvent, INFINITE);

            if (pSharedData->terminate) {
                break;
            }

            vkRender->Render();

            SetEvent(doneEvent);
        }
    } else {
        pSharedData->terminate = true;
        SetEvent(doneEvent);
    }

    vkRender->Cleanup();
    delete vkRender;

    for (UINT i = 0; i < pSharedData->numSharedBuffers; i++) {
        if (pSharedData->sharedMemHandle[i]) {
            CloseHandle(pSharedData->sharedMemHandle[i]);
            pSharedData->sharedMemHandle[i] = 0;
        }
        if (pSharedData->sharedFenceHandle[i]) {
            CloseHandle(pSharedData->sharedFenceHandle[i]);
            pSharedData->sharedFenceHandle[i] = 0;
        }
    }

    if (startEvent) {
        CloseHandle(startEvent);
        startEvent = 0;
    }

    if (doneEvent) {
        CloseHandle(doneEvent);
        doneEvent = 0;
    }

    pSharedData->terminated = true;

    return 0;
}

static int test(const char* program, HINSTANCE hInstance, Config* pConfig)
{
    DX12SharedResource* pSharedResource = new DX12SharedResource(hInstance, 
                                                                     program, 
                                                                     pConfig->numBuffers, 
                                                                     pConfig->duration, 
                                                                     pConfig->mode, 
                                                                     pConfig->vsync, 
                                                                     //pConfig->validate, 
                                                                     pConfig->dedicated /*,
                                                                     pConfig->captureFile ? pConfig->captureFrame : 0,
                                                                     pConfig->captureFile */
                                                                    );
    if (!pSharedResource) {
        return 0;
    }

    HWND hWnd = InitWindow(hInstance, pConfig, pSharedResource);
    if (!hWnd) {
        return 0;
    }

    bool running = true;
    MSG msg;
    while (running)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE)) {
            if (GetMessage(&msg, 0, 0, 0)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                running = false;
            }
        }
        else {
            IdleProc(hWnd);
        }
    }

    UINT status = pSharedResource->GetStatus();

  /*  if (pConfig->validate) {
        printf("%u shared buffer(s) / %s / VSync %-3s : %s\n", 
            pConfig->numBuffers, 
            pConfig->mode   == MULTI_THREADED ? "multi-threaded " : (pConfig->mode == SINGLE_THREADED ? "single-threaded" : "cross-process  "),
            pConfig->vsync ? "On" : "Off",
            status ? "Failed" : "Passed");
    } else */ if (!status) {
        printf("%u shared buffer(s) / %s / VSync %-3s : %1.0f fps\n", 
            pConfig->numBuffers, 
            pConfig->mode   == MULTI_THREADED ? "multi-threaded " : (pConfig->mode == SINGLE_THREADED ? "single-threaded" : "cross-process  "),
            pConfig->vsync ? "On" : "Off",
            pSharedResource->GetFPS());
    }

    delete pSharedResource;

    return (int)status;
}

static void usage()
{
    fprintf(stdout, "\nDX12SharedResource [options]\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "    -mt                Run multi-threaded\n");
    fprintf(stdout, "    -p                 Run cross-process\n");
    //fprintf(stdout, "    -validate          Validate results\n");
    fprintf(stdout, "    -vsync             Present after vertical blank\n");
    fprintf(stdout, "    -dedicated         Use dedicated memory (if supported)\n");
    fprintf(stdout, "    -n <n>             Use <n> shared buffers (2 <= <n> <= 4)\n");
    fprintf(stdout, "    -d <n>             Duration in seconds\n");
    // fprintf(stdout, "    -capture <n> <fn>  Capture frame <n> to BMP file <fn>\n");
    fprintf(stdout, "    -fulltest          Run full QA test\n");
    fprintf(stdout, "    -h                 Show this help\n");
    exit(0);
}

int main(int argc, char* argv[])
{
    Config cfg;

    if ((argc == 2) && (_stricmp(argv[1], VK_DX12_SHARED_RESOURCE_CLIENT_ARG) == 0)) {
        return render();
    }

    printf("DX12SharedResource \n\n");

    if ((argc == 2) && ((_stricmp(argv[1], "-h") == 0) ||
                        (_stricmp(argv[1], "-help") == 0) ||
                        (_stricmp(argv[1], "--help") == 0) ||
                        (_stricmp(argv[1], "-?") == 0))) {
        usage();
        return 0;
    }

    WNDCLASSEX wndClass;
    HINSTANCE hInstance = GetModuleHandle(NULL);
        
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = VK_DX12_SHARED_RESOURCE;
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

    if (!RegisterClassEx(&wndClass)) {
        return 1;
    }

    bool fulltest = false;
    for (int i = 1; i < argc; i++) {
        if (_stricmp(argv[1], "-fulltest") == 0) {
            fulltest = true;
            break;
        }
    }

    if (fulltest) {
        cfg.duration = 5;

        for (int i = 1; i < argc; i++) {
            if (_stricmp(argv[i], "-fulltest") == 0) {
                continue;
            }
            if (_stricmp(argv[i], "-dedicated") == 0) {
                cfg.dedicated = true;
                continue;
            }
            if ((_stricmp(argv[i], "-d") == 0) && (i < argc - 1)) {
                cfg.duration = atoi(argv[++i]);
                continue;
            }
            fprintf(stderr, "\nInvalid option: %s\n", argv[i]);
            fprintf(stderr, "\nFor help: DX12SharedResource -h\n");
            exit(1);
        }
        for (int i = 0; i < 3; i++) {
            switch (i) {
            case 0:
                cfg.mode = SINGLE_THREADED;
                break;
            case 1:
                cfg.mode = MULTI_THREADED;
                break;
            default:
                cfg.mode = CROSS_PROCESS;
                break;
            }
            for (cfg.numBuffers = MIN_SHARED_BUFFERS; cfg.numBuffers <= MAX_SHARED_BUFFERS; cfg.numBuffers++) {
                for (int vsync = 0; vsync < 2; vsync++) {
                    cfg.vsync = vsync != 0;
                    for (int validate = 0; validate < 2; validate++) {
                        //cfg.validate = validate != 0;
                        int status = test(argv[0], hInstance, &cfg);
                        if (status) {
                            return status;
                        }
                    }
                }
            }
        }
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (_stricmp(argv[i], "-mt") == 0) {
            cfg.mode = MULTI_THREADED;
            continue;
        }
        if (_stricmp(argv[i], "-p") == 0) {
            cfg.mode = CROSS_PROCESS;
            continue;
        }
        //if (_stricmp(argv[i], "-validate") == 0) {
        //    cfg.validate = true;
        //    continue;
        //}
        if (_stricmp(argv[i], "-vsync") == 0) {
            cfg.vsync = true;
            continue;
        }
        if (_stricmp(argv[i], "-dedicated") == 0) {
            cfg.dedicated = true;
            continue;
        }
        if ((_stricmp(argv[i], "-n") == 0) && (i < argc - 1)) {
            cfg.numBuffers = atoi(argv[++i]);
            if (cfg.numBuffers < MIN_SHARED_BUFFERS) {
                fprintf(stderr, "\nMininum number of buffers is %d\n", MIN_SHARED_BUFFERS);
                exit(1);
            }
            if (cfg.numBuffers > MAX_SHARED_BUFFERS) {
                fprintf(stderr, "\nMaximum number of buffers is %d\n", MAX_SHARED_BUFFERS);
                exit(1);
            }
            continue;
        }
        if ((_stricmp(argv[i], "-d") == 0) && (i < argc - 1)) {
            cfg.duration = atoi(argv[++i]);
            continue;
        }
        //if ((_stricmp(argv[i], "-capture") == 0) && (i < argc - 2)) {
        //    cfg.captureFrame = atoi(argv[++i]);
        //    cfg.captureFile = argv[++i];
        //    continue;
        //}
        fprintf(stderr, "\nInvalid option: %s\n", argv[i]);
        fprintf(stderr, "\nFor help: DX12SharedResource -h\n");
        exit(1);
    }

    return test(argv[0], hInstance, &cfg);
}
