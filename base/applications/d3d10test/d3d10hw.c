
#include <Windows.h>
#include <d3d10_1.h>
#include <d3d10.h>
#include <debug.h>

/* D3D10Core */
HRESULT WINAPI D3D10CoreCreateDevice(IDXGIFactory *factory, IDXGIAdapter *adapter,
        unsigned int flags, D3D_FEATURE_LEVEL feature_level, ID3D10Device **device);
static HWND g_hwnd = NULL;
static ATOM g_atom = 0;
const FLOAT CLEAR_COLOR[4] = { 1.0f, 0.1f, 1.0f, 1.0f };
HINSTANCE hInst = 0;
static const WCHAR szClassName[] = L"D3DClass";
static void msgerror(LPCWSTR msg)
{
	MessageBoxW(g_hwnd,msg, L"FATAL ERROR", MB_OK | MB_ICONERROR);
}

ID3D10Device           *GlobalD3DDevice;
IDXGIFactory           *GlobalDxgiFactory;
IDXGIAdapter           *GlobalDxgiAdapter;
IDXGIDevice            *GlobalDxgiDevice;
IDXGISwapChain         *GlobalDxgiSwapChain;
ID3D10RenderTargetView *GlobalD3D10RenderTargetView;

static void D3D10Render()
{
	GlobalD3DDevice->lpVtbl->ClearRenderTargetView(GlobalD3DDevice, GlobalD3D10RenderTargetView, CLEAR_COLOR);


	GlobalDxgiSwapChain->lpVtbl->Present(GlobalDxgiSwapChain, 0, 0);
}

static HRESULT InitD3D10()
{
	DPRINT1("Starting D3D10 Enumeration in the WDDM 1.0 Style->\n");
	HRESULT hr;
	BOOLEAN FoundAFactory = FALSE;
	hr = CreateDXGIFactory(&IID_IDXGIFactory, (void **)&GlobalDxgiFactory);
	if (hr != 0)
		msgerror(L"Failed to create Dxgi Factory\n");
	DPRINT1("Dxgi Has sucessfully made its factory\n");
	int i = 0;
	while (GlobalDxgiFactory->lpVtbl->EnumAdapters(GlobalDxgiFactory, i, &GlobalDxgiAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC Desc = {0};
		FoundAFactory = TRUE;
		DPRINT1("EnumAdapters: Found a factory\n");
		hr = GlobalDxgiAdapter->lpVtbl->GetDesc(GlobalDxgiAdapter, &Desc);
		if (hr != 0)
			msgerror(L"Could not obtain DxgiAdapter Description\n");
		DPRINT1("DXGI Has found a device with the Following information:\n");
		DPRINT1("    Description: ");
		OutputDebugStringW(Desc.Description);
		DPRINT1("\n");
		DPRINT1("    Vendor: %X Device: %X\n", Desc.VendorId, Desc.DeviceId);
		i++;
	}

	if (FoundAFactory != TRUE)
	{
		msgerror(L"No DxgiFactory was found\n");
		return 1;
	}

	hr = D3D10CreateDevice(GlobalDxgiAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &GlobalD3DDevice);
	if (hr != 0)
	{
		msgerror(L"D3D10CreateDevice - Failed\n");
		DPRINT1("D3D10CreateDevice failed with status %X\n", hr);
	}
	DPRINT1("InitD3D10: Created d3d10 device\n");
	GlobalD3DDevice->lpVtbl->Release(GlobalD3DDevice);
	DPRINT1("released D3D10 Device\n");

	/* ------------------------------------------------------------ */

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
	swapChainDesc.Windowed = TRUE;
	hr = D3D10CreateDeviceAndSwapChain(GlobalDxgiAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &swapChainDesc, &GlobalDxgiSwapChain, &GlobalD3DDevice);
	if (FAILED(hr))
	{
		DPRINT1("D3D10CreateDeviceAndSwapChain: Failed\n");
		msgerror(L"D3D10CreateDeviceAndSwapChain - Failed\n");
		return hr;
	}

	DPRINT1("D3D10CreateDeviceAndSwapChain: SUCCESS\n");

	// get back buffer from output/swapchain
	ID3D10Texture2D* bb;
	hr = GlobalDxgiSwapChain->lpVtbl->GetBuffer(GlobalDxgiSwapChain, 0, &IID_ID3D10Texture2D, (void**)&bb);
	if (FAILED(hr))
		return hr;
	DPRINT1("Obtained le backfbuffer\n");
	// create rtv
	hr = GlobalD3DDevice->lpVtbl->CreateRenderTargetView(GlobalD3DDevice, (ID3D10Resource *)bb, NULL, &GlobalD3D10RenderTargetView);
	if (FAILED(hr))
		return hr;

	// tell d3d10 to render to the screen
	GlobalD3DDevice->lpVtbl->OMSetRenderTargets(GlobalD3DDevice, 1, &GlobalD3D10RenderTargetView, NULL);

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	WNDCLASSEXW wcex;
	wcex.cbClsExtra = 0;
	wcex.cbSize = sizeof(wcex);
	wcex.cbWndExtra = 0;
	wcex.lpszClassName = szClassName;
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wcex.hInstance = hInst;
	wcex.lpszMenuName = NULL;
	wcex.style = CS_VREDRAW | CS_HREDRAW;
	wcex.lpfnWndProc = WinProc;

	g_atom = RegisterClassExW(&wcex); 
	if (!g_atom)
	{
		msgerror(L"RegisterClassEx fail");
		return -1;
	}

	g_hwnd = CreateWindowW(szClassName, (LPCWSTR)L"Direct3D10 app", WS_OVERLAPPEDWINDOW, 0, 0, 800, 600, NULL, NULL, hInst, NULL);
	if (!g_hwnd)
	{
		msgerror(L"CreateWindow fail");
		UnregisterClassW(szClassName, hInst);
		return -1;
	}

	auto hr = InitD3D10();
	if (FAILED(hr))
	{
		msgerror(L"D3D10 Init fail");
		DestroyWindow(g_hwnd);
		UnregisterClassW(szClassName, hInst);
		return -1;
	}

	ShowWindow(g_hwnd, SW_SHOW);
	UpdateWindow(g_hwnd);

	MSG msg;

	while (1)
	{
		if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
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

	DestroyWindow(g_hwnd);
	UnregisterClassW(szClassName, hInst);
	return 0;
}
