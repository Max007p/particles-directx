// DirectX_TEST.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "DirectX_TEST.h"


#define MAX_LOADSTRING 100

// Глобальные переменные:
HWND hWnd;
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
IDXGISwapChain* swapChain;
ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
ID3D11RenderTargetView* renderTargetView;
ID3D11ComputeShader* computeShader;
ID3D11VertexShader* vertexShader;
ID3D11PixelShader* pixelShader;
ID3D11InputLayout* inputLayout;
ID3D11Buffer* positionBuffer;
ID3D11Buffer* velocityBuffer;
ID3D11UnorderedAccessView* positionUAV;
ID3D11UnorderedAccessView* velocityUAV;
const int FRAME_TIME_COUNT = 128;
clock_t frameTime[FRAME_TIME_COUNT];
int currentFrame = 0;
int width = 800, height = 800;

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void                InitSwapChain(HWND);
void                DisposeSwapChain();
void                InitRenderTargetView();
void                DisposeRenderTargetView();
void                InitShaders();
void                DisposeShaders();
void                InitBuffers();
void                DisposeBuffers();
void                InitUAV();
void                DisposeUAV();
void                InitBindings();
float               AverageFrameTime();
void                Frame(HWND);
void                ResizeSwapChain(HWND);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.
    

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_DIRECTXTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTXTEST));

    MSG msg;

    // Цикл основного сообщения:
    bool shouldExit = false;
    while (!shouldExit)
    {
        Frame(hWnd);

        while (!shouldExit && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);

            if (msg.message == WM_QUIT)
            {
                shouldExit = true;
            }
        }
    }

    DisposeUAV();
    DisposeBuffers();
    DisposeShaders();
    DisposeRenderTargetView();
    DisposeSwapChain();

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTXTEST));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, width, height, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   InitSwapChain(hWnd);
   InitRenderTargetView();
   InitShaders();
   InitBuffers();
   InitUAV();
   InitBindings();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Добавьте сюда любой код прорисовки, использующий HDC...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        break;
    case WM_SIZE:
        ResizeSwapChain(hWnd);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

void InitSwapChain(HWND hWnd)
{
    HRESULT result;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;

    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;

    swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hWnd;

    swapChainDesc.Windowed = TRUE;

    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;

#ifndef NDEBUG
    UINT flags = D3D11_CREATE_DEVICE_DEBUG;
#else
    UINT flags = 0;
#endif

    result = D3D11CreateDeviceAndSwapChain(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        flags,
        &featureLevel,
        1,
        D3D11_SDK_VERSION,
        &swapChainDesc,
        &swapChain,
        &device,
        NULL,
        &deviceContext
    );

    assert(SUCCEEDED(result));
}

void DisposeSwapChain()
{
    deviceContext->Release();
    device->Release();
    swapChain->Release();
}

void InitRenderTargetView()
{
    HRESULT result;
    ID3D11Texture2D* backBuffer;

    result = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    assert(SUCCEEDED(result));

    result = device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
    assert(SUCCEEDED(result));

    backBuffer->Release();

    deviceContext->OMSetRenderTargets(1, &renderTargetView, NULL);

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = (FLOAT)width;
    viewport.Height = (FLOAT)height;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    deviceContext->RSSetViewports(1, &viewport);
}

void DisposeRenderTargetView()
{
    renderTargetView->Release();
}

void InitShaders()
{
    HRESULT result;
    HRSRC src;
    HGLOBAL res;

    src = FindResource(hInst, MAKEINTRESOURCE(IDR_BYTECODE_COMPUTE), _T("ShaderObject"));
    res = LoadResource(hInst, src);
    result = device->CreateComputeShader(
        res, 
        SizeofResource(hInst, src),
        NULL,
        &computeShader
    );
    assert(SUCCEEDED(result));
    FreeResource(res);

    src = FindResource(hInst, MAKEINTRESOURCE(IDR_BYTECODE_PIXEL), _T("ShaderObject"));
    res = LoadResource(hInst, src);
    result = device->CreatePixelShader(
        res,
        SizeofResource(hInst, src),
        NULL,
        &pixelShader
    );
    assert(SUCCEEDED(result));
    FreeResource(res);

    src = FindResource(hInst, MAKEINTRESOURCE(IDR_BYTECODE_VERTEX), _T("ShaderObject"));
    res = LoadResource(hInst, src);
    result = device->CreateVertexShader(
        res,
        SizeofResource(hInst, src),
        NULL,
        &vertexShader
    );
    assert(SUCCEEDED(result));
    
    D3D11_INPUT_ELEMENT_DESC inputDesc;
    inputDesc.SemanticName = "POSITION";
    inputDesc.SemanticIndex = 0;
    inputDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
    inputDesc.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    inputDesc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    inputDesc.InputSlot = 0;
    inputDesc.InstanceDataStepRate = 0;

    result = device->CreateInputLayout(
        &inputDesc,
        1,
        res,
        SizeofResource(hInst, src),
        &inputLayout
    );
    assert(SUCCEEDED(result));
    FreeResource(res);
}

void DisposeShaders()
{
    inputLayout->Release();
    computeShader->Release();
    vertexShader->Release();
    pixelShader->Release();
}

void InitBuffers()
{
    HRESULT result;

    float* data = new float[2 * PARTICLE_COUNT];

    D3D11_SUBRESOURCE_DATA subres;
    subres.pSysMem = data;
    subres.SysMemPitch = 0;
    subres.SysMemSlicePitch = 0;

    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = sizeof(float[2 * PARTICLE_COUNT]);
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_UNORDERED_ACCESS;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    desc.StructureByteStride = sizeof(float[2]);

    for (int i = 0; i < 2 * PARTICLE_COUNT; i++)
    {
        data[i] = 2.0f * rand() / RAND_MAX - 1.0f;
    }

    result = device->CreateBuffer(&desc, &subres, &positionBuffer);
    assert(SUCCEEDED(result));

    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;

    for (int i = 0; i < 2 * PARTICLE_COUNT; i++)
    {
        data[i] = 0.0f;
    }

    result = device->CreateBuffer(&desc, &subres, &velocityBuffer);
    assert(SUCCEEDED(result));

    delete[] data;
}

void DisposeBuffers()
{
    positionBuffer->Release();
    velocityBuffer->Release();
}

void InitUAV()
{
    HRESULT result;

    D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
    desc.Format = DXGI_FORMAT_R32G32_FLOAT;
    desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    desc.Buffer.FirstElement = 0;
    desc.Buffer.NumElements = PARTICLE_COUNT;
    desc.Buffer.Flags = 0;

    result = device->CreateUnorderedAccessView(positionBuffer, &desc, &positionUAV);
    assert(!result);

    result = device->CreateUnorderedAccessView(velocityBuffer, &desc, &velocityUAV);
    assert(!result);
}

void DisposeUAV()
{
    positionUAV->Release();
    velocityUAV->Release();
}

void InitBindings()
{
    deviceContext->CSSetShader(computeShader, NULL, 0);
    deviceContext->VSSetShader(vertexShader, NULL, 0);
    deviceContext->PSSetShader(pixelShader, NULL, 0);
    deviceContext->CSSetUnorderedAccessViews(1, 1, &velocityUAV, NULL);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
}

float AverageFrameTime()
{
    frameTime[currentFrame] = clock();
    int nextFrame = (currentFrame + 1) % FRAME_TIME_COUNT;
    clock_t delta = frameTime[currentFrame] - frameTime[nextFrame];
    currentFrame = nextFrame;
    return (float)delta / CLOCKS_PER_SEC / FRAME_TIME_COUNT;
}

void Frame(HWND hWnd)
{
    float frameTime = AverageFrameTime();

    char buf[256];
    sprintf_s(buf, "avr. framerate: %.1f", 1.0f / frameTime);
    SetWindowTextA(hWnd, buf);

    float clearColor[] = {0.0f, 0.0f, 0.0f, 0.0f};
    deviceContext->ClearRenderTargetView(renderTargetView, clearColor);

    UINT stride = sizeof(float[2]);
    UINT offset = 0;

    ID3D11Buffer* nullBuffer = NULL;
    ID3D11UnorderedAccessView* nullUAV = NULL;

    deviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);
    deviceContext->CSSetUnorderedAccessViews(0, 1, &positionUAV, NULL);
    deviceContext->Dispatch(PARTICLE_COUNT / NUMTHREADS, 1, 1);
    
    deviceContext->CSSetUnorderedAccessViews(0, 1, &nullUAV, NULL);
    deviceContext->IASetVertexBuffers(0, 1, &positionBuffer, &stride, &offset);
    deviceContext->Draw(PARTICLE_COUNT, 0);

    swapChain->Present(0, 0);
}

void ResizeSwapChain(HWND hWnd)
{
    HRESULT result1;
    RECT rect;

    GetClientRect(hWnd, &rect);

    width = rect.right - rect.left;
    height = rect.bottom - rect.top;

    DisposeRenderTargetView();

    result1 = swapChain->ResizeBuffers(
        0,
        width,
        height,
        DXGI_FORMAT_UNKNOWN,
        0
    );
    assert(SUCCEEDED(result1));
    InitRenderTargetView();

}