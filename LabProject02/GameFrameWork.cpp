#include "stdafx.h"
#include "GameFrameWork.h"

#include <iostream>

CGameFrameWork::CGameFrameWork()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dPipelineState = NULL;
	m_pd3dCommandList = NULL;

	for (auto& m_ppd3dRenderTargetBuffer : m_ppd3dRenderTargetBuffers)
	{
		m_ppd3dRenderTargetBuffer = NULL;
	}
	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;
	m_nDsvDescriptorIncrementSize = 0;

	m_nSwapChainBufferIndex = 0;

	m_hInstance = NULL;
	m_pd3dFence = NULL;
	m_nFenceValue = 0;

	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;
	m_nWndClientWidth = FRAME_BUFFER_WIDTH;

}

CGameFrameWork::~CGameFrameWork()
{
}

bool CGameFrameWork::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;
	//Direct3D 디바이스, 명령 큐와 명령 리스트, 스왑 체인 등을 생성하는 함수를 호출한다.

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();
	CreateRenderTargetViews();
	CreateDepthStencilView();
	BuildObjects();

	//렌더링할 게임 객체를 생성한다. 
	return(true);

}

void CGameFrameWork::OnDestroy()
{
	WaitForGpuComplete();//GPU가 모든 명령을 처리할 때까지 대기한다.

	ReleaseObjects(); //렌더링 객체들을 소멸한다.

	::CloseHandle(m_hFenceEvent);

	for (auto& render_target_buffer : m_ppd3dRenderTargetBuffers)
	{
		if (render_target_buffer)
		{
			render_target_buffer->Release();
		}
	}

	if (m_pd3dRtvDescriptorHeap)
	{
		m_pd3dRtvDescriptorHeap->Release();
	}
	if (m_pd3dDepthStencilBuffer)
	{
		m_pd3dDepthStencilBuffer->Release();
	}
	if (m_pd3dDsvDescriptorHeap)
	{
		m_pd3dDsvDescriptorHeap->Release();
	}
	if (m_pd3dCommandAllocator)
	{
		m_pd3dCommandAllocator->Release();
	}
	if (m_pd3dCommandQueue)
	{
		m_pd3dCommandQueue->Release();
	}
	if (m_pd3dCommandList)
	{
		m_pd3dCommandList->Release();
	}
	if (m_pd3dFence)
	{
		m_pd3dFence->Release();
	}

	m_pdxgiSwapChain->SetFullscreenState(FALSE, nullptr);

	if (m_pdxgiSwapChain)
	{
		m_pdxgiSwapChain->Release();
	}
	if (m_pd3dDevice)
	{
		m_pd3dDevice->Release();
	}
	if (m_pdxgiFactory)
	{
		m_pdxgiFactory->Release();
	}
#if defined(_DEBUG)
	IDXGIDebug1* pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void**)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif

	
}

void CGameFrameWork::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = m_bMsaa4xEnable ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = m_bMsaa4xEnable ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	dxgiSwapChainDesc.Flags = 0;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(dxgiSwapChainFullScreenDesc));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	m_pdxgiFactory->CreateSwapChainForHwnd(
		m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc,
		nullptr, reinterpret_cast<IDXGISwapChain1**>(&m_pdxgiSwapChain));
	//스왑체인 생성

	m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	//ALT+ENTER 전체화면 전환을 막는다.
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	//현재 백버퍼 인덱스를 가져온다.
}

void CGameFrameWork::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactory = 0;
#if defined(_DEBUG)
	ID3D12Debug* pd3dDebugController = nullptr;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), reinterpret_cast<void**>(&pd3dDebugController));
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactory |= DXGI_CREATE_FACTORY_DEBUG;
#endif
	hResult = ::CreateDXGIFactory2(nDXGIFactory, 
		__uuidof(IDXGIFactory4), reinterpret_cast<void**>(&m_pdxgiFactory));
	//DXGI 팩토리 생성

	IDXGIAdapter1* pd3dAdapter = nullptr;
	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0,
			__uuidof(ID3D12Device), reinterpret_cast<void**>(&m_pd3dDevice))))
		{//모든 하드웨어 어뎁터에 대하여 특성 레벨을 12.0을 지원하는 하드웨어 디바이스를 생성한다.
			break;
		}
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&pd3dAdapter));

		D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_11_0,
			__uuidof(ID3D12Device), reinterpret_cast<void**>(&m_pd3dDevice));
		//특성 레벨 12.0을 지원하는 하드웨어 디바이스를 생성하지 못하면 Wrap 디바이스를 생성함
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4; //msaa x4 샘플링
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	//디바이스 지원하는 다중 샘플링의 레벨을 확인한다.
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1);
	//다중샘플링이 1보다 크면 true

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		__uuidof(ID3D12Fence), reinterpret_cast<void**>(&m_pd3dFence));
	m_nFenceValue = 0;
	//펜스 생성하고 값을 1로 설정

	m_hFenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	//펜스와 동기화를 위한 이벤트 객체를 생성한다. (이벤트 객체의 최기값을 false로 설정)
	//이벤트가 실행되면(signal) 이벤트의 값을 자동적으로 false가 되도록 생성한다.

	m_d3dViewport.TopLeftX = 0.0f;
	m_d3dViewport.TopLeftY = 0.0f;
	m_d3dViewport.Height = static_cast<float>(m_nWndClientHeight);
	m_d3dViewport.Width = static_cast<float>(m_nWndClientWidth);
	m_d3dViewport.MinDepth = 0.0f;
	m_d3dViewport.MaxDepth = 1.0f;
	//뷰포트를 클라이언트 윈도우 전체로 설정

	m_d3dScissorRect = { 0, 0, m_nWndClientWidth, m_nWndClientHeight };
	//시저(가위) 사각형을 클라이언트 윈도우 전체로 설정

	if (pd3dAdapter)
	{
		pd3dAdapter->Release();
	}
}

void CGameFrameWork::CreateCommandQueueAndList()
{
	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc,
		__uuidof(ID3D12CommandQueue), reinterpret_cast<void**>(&m_pd3dCommandQueue));
	if (FAILED(hResult))
	{
		std::cerr << "Failed to create command queue." << std::endl;
		return;
	}

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
		__uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(&m_pd3dCommandAllocator));
	if (FAILED(hResult))
	{
		std::cerr << "Failed to create command allocator." << std::endl;
		return;
	}

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_pd3dCommandAllocator, nullptr, __uuidof(ID3D12GraphicsCommandList),
		reinterpret_cast<void**>(&m_pd3dCommandList));
	if (FAILED(hResult))
	{
		std::cerr << "Failed to create command list." << std::endl;
		return;
	}

	hResult = m_pd3dCommandList->Close();
	if (FAILED(hResult))
	{
		std::cerr << "Failed to close command list." << std::endl;
		return;
	}
}

void CGameFrameWork::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&m_pd3dRtvDescriptorHeap));
	//렌더 타겟 서술자 힙 생성

	m_nRtvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//렌더 타겟 서술자 힙의 원소 크기를 가져온다.

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc,
		__uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&m_pd3dDsvDescriptorHeap));
	//깊이 스텐실 서술자 힙 생성
	m_nDsvDescriptorIncrementSize =
		m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	//깊이 스텐실 서술자 힙의 원소 크기를 가져온다.
}
//스왑체인의 각 후면 버퍼에 대한 렌더 타겟 뷰를 생성한다
void CGameFrameWork::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; ++i)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource),
			reinterpret_cast<void**>(&m_ppd3dRenderTargetBuffers[i]));
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dRenderTargetBuffers[i], nullptr, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}
}

void CGameFrameWork::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = m_bMsaa4xEnable ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = m_bMsaa4xEnable ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;
	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE,
		&d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue,
		__uuidof(ID3D12Resource), reinterpret_cast<void**>(&m_pd3dDepthStencilBuffer));
	//깊이 스텐실 버퍼 생성

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle =
		m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, nullptr, d3dDsvCPUDescriptorHandle);
	//깊이 스텐실 뷰 생성
}

void CGameFrameWork::BuildObjects()
{
}

void CGameFrameWork::ReleaseObjects()
{
}

void CGameFrameWork::ProcessInput()
{
}

void CGameFrameWork::AnimateObjects()
{
}

void CGameFrameWork::FrameAdvance()
{
	ProcessInput();

	AnimateObjects();
	
	HRESULT hResult = m_pd3dCommandAllocator->Reset();
	hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);
	if (FAILED(hResult))
	{
		// 오류 처리 코드
		// 예: 로그 출력, 예외 던지기 등
		std::cerr << "Failed to reset command list." << std::endl;
		
	}
	//명령 할당자와 명령 리스트를 재설정한다.

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = m_ppd3dRenderTargetBuffers[m_nSwapChainBufferIndex];
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	m_pd3dCommandList->RSSetViewports(1, &m_d3dViewport);
	m_pd3dCommandList->RSSetScissorRects(1, &m_d3dScissorRect);
	
	


	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle =
		m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize;

	
	// 현재의 렌더 타겟에 해당하는 서술자의 CPU 주소(핸들)를 계산한다.
	float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
	m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle,
		pfClearColor/*Colors::Azure*/, 0, NULL);
	//원하는 색상으로 렌더 타겟(뷰)을 지운다.

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle =
		m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//깊이-스텐실 서술자의 CPU 주소를 계산한다.

	m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle,
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
	//원하는 값으로 깊이-스텐실(뷰)을 지운다.

	m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE,
		&d3dDsvCPUDescriptorHandle);
	//렌더 타겟 뷰(서술자)와 깊이-스텐실 뷰(서술자)를 출력-병합 단계(OM)에 연결한다.
	//렌더링 코드는 여기에 추가될 것이다.

	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
	/*현재 렌더 타겟에 대한 렌더링이 끝나기를 기다린다. GPU가 렌더 타겟(버퍼)을 더 이상 사용하지 않으면 렌더 타겟
   의 상태는 프리젠트 상태(D3D12_RESOURCE_STATE_PRESENT)로 바뀔 것이다.*/

	hResult = m_pd3dCommandList->Close();
	//명령 리스트를 닫힌 상태로 만든다.

	ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);
	//명령 리스트를 명령 큐에 추가하여 실행한다.

	WaitForGpuComplete();
	//GPU가 모든 명령 리스트를 실행할 때 까지 기다린다.

	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
	/*스왑체인을 프리젠트한다. 프리젠트를 하면 현재 렌더 타겟(후면버퍼)의 내용이 전면버퍼로 옮겨지고 렌더 타겟 인
   덱스가 바뀔 것이다.*/

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
}

void CGameFrameWork::WaitForGpuComplete()
{
	m_nFenceValue++;
	//CPU 펜스 값 증가
	const UINT64 nFence = m_nFenceValue;
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFence);
	//GPU가 펜스의 값을 설정하는 명령 큐에 추가
	if (m_pd3dFence->GetCompletedValue() < nFence)
	{//펜스의 현재 값이 설정한 값보다 작으면 펜스의 현재 값이 설정한 값이 될때까지 대기
		hResult = m_pd3dFence->SetEventOnCompletion(nFence, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFrameWork::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void CGameFrameWork::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_ESCAPE:
			::PostQuitMessage(0);
			break;
		case VK_RETURN:
			break;
		case VK_F8:
			break;
		case VK_F9:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CGameFrameWork::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_SIZE:
		{
			m_nWndClientWidth = LOWORD(lParam);
			m_nWndClientHeight = HIWORD(lParam);
		}
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MOUSEMOVE:
		OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
		break;
	}
	return 0;
}
