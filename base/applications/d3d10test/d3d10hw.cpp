
#include <Windows.h>
#include <d3d10.h>

#include <string>

static HWND g_hwnd = nullptr;
static ATOM g_atom = 0;
static ID3D10Device* g_dev = nullptr;
static IDXGISwapChain* g_sc = nullptr;
static ID3D10RenderTargetView* g_rtv = nullptr;

constexpr FLOAT CLEAR_COLOR[4] = { 1.0f, 0.1f, 1.0f, 1.0f };

static void msgerror(LPCWSTR msg, DWORD err = GetLastError())
{
	std::wstring x = msg;
	x += L"\nWIN32 ERROR: ";
	x += std::to_wstring(err);
	MessageBoxW(g_hwnd, x.c_str(), L"FATAL ERROR", MB_OK | MB_ICONERROR);
}

static void D3D10Render()
{
	g_dev->ClearRenderTargetView(g_rtv, CLEAR_COLOR);

	// submit vtx to pipeline

	g_sc->Present(0, 0);
}

static HRESULT InitD3D10()
{
	RECT rc;
	GetClientRect(g_hwnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	//set buffer dimensions and format
	swapChainDesc.BufferCount = 2;
	swapChainDesc.BufferDesc.Width = width;
	swapChainDesc.BufferDesc.Height = height;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;;

	//set refresh rate
	swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;

	//sampling settings
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.SampleDesc.Count = 1;

	//output window handle
	swapChainDesc.OutputWindow = g_hwnd;
	swapChainDesc.Windowed = true;

	// for sure: D3D10CreateDevice -> Output from factory -> Swapchain...
	auto hr = D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, nullptr, 0, D3D10_SDK_VERSION, &swapChainDesc, &g_sc, &g_dev);
	if (FAILED(hr))
	{
		return hr;
	}

	// get back buffer from output/swapchain
	ID3D10Texture2D* bb;
	hr = g_sc->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&bb);
	if (FAILED(hr))
		return hr;

	// create rtv
	hr = g_dev->CreateRenderTargetView(bb, nullptr, &g_rtv);
	if (FAILED(hr))
		return hr;

	bb->Release();

	// tell d3d10 to render to the screen
	g_dev->OMSetRenderTargets(1, &g_rtv, nullptr);

	return S_OK;
}

static LRESULT CALLBACK WinProc(_In_ HWND hWnd, _In_ UINT Msg, _In_opt_ WPARAM wParam, _In_opt_ LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProcW(hWnd, Msg, wParam, lParam);
	}

	return FALSE;
}

INT WINAPI wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrev, _In_ LPWSTR lpCmdLine, _In_ INT nCmdShow)
{
	WNDCLASSEXW wcex;
	wcex.cbClsExtra = 0;
	wcex.cbSize = sizeof(wcex);
	wcex.cbWndExtra = 0;
	wcex.lpszClassName = L"DX_APP_TEST";
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hInstance = hInst;
	wcex.lpszMenuName = nullptr;
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = WinProc;

	g_atom = RegisterClassExW(&wcex); 
	if (!g_atom)
	{
		msgerror(L"RegisterClassEx fail");
		return -1;
	}

	g_hwnd = CreateWindowW(MAKEINTATOM(g_atom), L"Direct3D10 app", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, nullptr, nullptr, hInst, nullptr);
	if (!g_hwnd)
	{
		msgerror(L"CreateWindow fail");
		UnregisterClassW(MAKEINTATOM(g_atom), hInst);
		return -1;
	}

	auto hr = InitD3D10();
	if (FAILED(hr))
	{
		msgerror(L"D3D10 Init fail", hr);
		DestroyWindow(g_hwnd);
		UnregisterClassW(MAKEINTATOM(g_atom), hInst);
		return -1;
	}

	ShowWindow(g_hwnd, SW_SHOW);
	UpdateWindow(g_hwnd);

	MSG msg;

	while (true)
	{
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else
		{
			D3D10Render();
		}
	}

	if (g_rtv)
		g_rtv->Release();

	if (g_sc)
		g_sc->Release();

	if (g_dev)
		g_dev->Release();

	DestroyWindow(g_hwnd);
	UnregisterClassW(MAKEINTATOM(g_atom), hInst);
	return 0;
}
