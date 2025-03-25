#pragma once
#include "Timer.h"
class CGameFrameWork {
public:
	CGameFrameWork();
	~CGameFrameWork();
	//응용 프로그램이 실행되어 주윈도우가 생성되면 호출됨
	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);//프레임 워크 초기화하는 함수
	void OnDestroy();//프레임 워크 종료하는 함수

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();
	void CreateRtvAndDsvDescriptorHeaps();
	//디바이스, 서술자 힙, 명령큐, 할당자, 리스트를 생성하는 함수

	void CreateRenderTargetViews();
	void CreateDepthStencilView();
	//렌더 타겟 뷰와 깊이 -스탠실 뷰를 생성하는 함수

	void BuildObjects();
	void ReleaseObjects();//렌더링 할때 메쉬 생성 소멸

	//프레임 워크의 핵심
	void ProcessInput();//입력 처리
	void AnimateObjects();//애니메이션 처리
	void FrameAdvance();//프레임 진행

	void WaitForGpuComplete();//GPU와 CPU동기화 함수

	//윈도우 마우스 메세지 를 처리하는 함수
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;

	int m_nWndClientWidth;
	int m_nWndClientHeight;

	IDXGIFactory4* m_pdxgiFactory; //DXGI 팩토리에 대한 포인터
	IDXGISwapChain3* m_pdxgiSwapChain; //스왑체인에 대한 포인터(스왑체인 = 프레임 버퍼 교체해주는 거)
	ID3D12Device* m_pd3dDevice; //디바이스에 대한 포인터 리소스 생성에 필요

	bool m_bMsaa4xEnable = false; //다중 샘플링 플래그
	UINT m_nMsaa4xQualityLevels = 0;//다중 샘플링 품질 레벨

	static constexpr UINT m_nSwapChainBufferCount = 2; //스왑체인 백버퍼 개수
	UINT m_nSwapChainBufferIndex = 0; //현재 백버퍼 인덱스

	ID3D12Resource* m_ppd3dRenderTargetBuffers[m_nSwapChainBufferCount]; //렌더 타겟 버퍼에 대한 포인터
	ID3D12DescriptorHeap* m_pd3dRtvDescriptorHeap; //렌더 타겟 뷰 디스크립터 힙에 대한 포인터
	UINT m_nRtvDescriptorIncrementSize; //렌더 타겟 서술자의 원소의 크기

	ID3D12Resource* m_pd3dDepthStencilBuffer; //깊이 스텐실 버퍼에 대한 포인터
	ID3D12DescriptorHeap* m_pd3dDsvDescriptorHeap; //깊이 스텐실 뷰 디스크립터 힙에 대한 포인터
	UINT m_nDsvDescriptorIncrementSize; //깊이 스텐실 서술자의 원소의 크기

	ID3D12CommandQueue* m_pd3dCommandQueue; //명령 큐에 대한 포인터
	ID3D12CommandAllocator* m_pd3dCommandAllocator; //명령 할당자에 대한 포인터
	ID3D12GraphicsCommandList* m_pd3dCommandList; //명령 리스트에 대한 포인터

	ID3D12PipelineState* m_pd3dPipelineState; //파이프라인 상태에 대한 포인터

	ID3D12Fence* m_pd3dFence; //펜스에 대한 포인터
	UINT64 m_nFenceValue; //펜스 값
	HANDLE m_hFenceEvent; //펜스 이벤트

	D3D12_VIEWPORT m_d3dViewport; //뷰포트 구조체
	D3D12_RECT m_d3dScissorRect; //시저(가위) 구조체

	CGameTimer m_GameTimer;		//게임 프레임워크에서 사용 할 타이머이다.
	_TCHAR m_pszFrameRate[50];	//프레임 레이트를 표시하기 위한 문자열 버퍼이다.

};

