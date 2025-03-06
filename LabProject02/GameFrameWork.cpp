#include "stdafx.h"
#include "GameFrameWork.h"

CGameFrameWork::CGameFrameWork()
{
	m_pDXGIFactory = NULL;
	m_pDXGISwapChain = NULL;
	m_pD3D12Device = NULL;

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

	//Direct3D 디바이스 명령큐와 명령 리스트, 스왑체인 등을 생성하는 함수를 호출한다.
	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();
	CreateRenderTargetView();
	CreateDepthStencilView();

	BuildObjects();//렌더링 객체 생성

	return true;

}

void CGameFrameWork::OnDestroy()
{
	WaitForGpuComplete();//GPU가 모든 명령을 처리할 때까지 대기한다.

	ReleaseObjects(); //렌더링 객체들을 소멸한다.

	::CloseHandle(m_hFenceEvent);

	for (auto&  render_target_buffer : m_ppd3dRenderTargetBuffers)
	{
		if (render_target_buffer)
		{
			render_target_buffer->Release();
		}
	}
}

void CGameFrameWork::CreateSwapChain()
{
}

void CGameFrameWork::CreateRtvAndDsvDescriptorHeaps()
{
}

void CGameFrameWork::CreateDirect3DDevice()
{
}

void CGameFrameWork::CreateCommandQueueAndList()
{
}

void CGameFrameWork::CreateRenderTargetView()
{
}

void CGameFrameWork::CreateDepthStencilView()
{
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
}

void CGameFrameWork::WaitForGpuComplete()
{
}

void CGameFrameWork::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

void CGameFrameWork::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}

LRESULT CGameFrameWork::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
}
