// Scene.h (수정본)

#pragma once

#include "Shader.h"
#include "Player.h"

// CHeightMapTerrain 클래스가 Mesh.h에 있으므로 포함합니다.
#include "Mesh.h" 

#define MAX_LIGHTS			16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

struct LIGHT
{
	XMFLOAT4				m_xmf4Ambient;
	XMFLOAT4				m_xmf4Diffuse;
	XMFLOAT4				m_xmf4Specular;
	XMFLOAT3				m_xmf3Position;
	float 					m_fFalloff;
	XMFLOAT3				m_xmf3Direction;
	float 					m_fTheta; //cos(m_fTheta)
	XMFLOAT3				m_xmf3Attenuation;
	float					m_fPhi; //cos(m_fPhi)
	bool					m_bEnable;
	int						m_nType;
	float					m_fRange;
	float					padding;
};

struct LIGHTS
{
	LIGHT					m_pLights[MAX_LIGHTS];
	XMFLOAT4				m_xmf4GlobalAmbient;
	int						m_nLights;
};

class CScene
{
public:
	CScene();
	~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	
	void BuildDefaultLightsAndMaterials();
	void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }

	void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	void ReleaseShaderVariables();

	void AnimateObjects(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL);

	void ReleaseUploadBuffers();

	void SetPlayer(CPlayer *pPlayer) { m_pPlayer = pPlayer; }
	CHeightMapTerrain *GetTerrain() { return(m_pTerrain); }
	// Scene.h 의 CScene 클래스 public 영역에 추가
	D3D12_GPU_VIRTUAL_ADDRESS GetLightsGpuVirtualAddress() { return(m_pd3dcbLights->GetGPUVirtualAddress()); }
protected:
	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;

	CPlayer						*m_pPlayer = NULL;
	CHeightMapTerrain			*m_pTerrain = NULL;

	LIGHT						*m_pLights = NULL;
	int							m_nLights = 0;
	XMFLOAT4					m_xmf4GlobalAmbient;

	ID3D12Resource				*m_pd3dcbLights = NULL;
	LIGHTS						*m_pcbMappedLights = NULL;

	float						m_fElapsedTime = 0.0f;
	CMesh*						m_pTestCubeMesh = nullptr;
};