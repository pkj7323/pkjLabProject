#include "stdafx.h"
#include "Scene.h"

#include "Camera.h"
#include "Shader.h"

CScene::CScene()
{}

CScene::~CScene()
{}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
    return(false);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
    return(false);
}

ID3D12RootSignature* CScene::CreateGraphicsRootSignature(ID3D12Device* pd3dDevice)
{
    ID3D12RootSignature *pd3dGraphicsRootSignature = NULL;
    D3D12_ROOT_PARAMETER pd3dRootParameters[2];
    pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;//루트 상수
	pd3dRootParameters[0].Constants.Num32BitValues = 16;//16개의 32비트 정수값을 루트 상수로 사용한다.
	pd3dRootParameters[0].Constants.ShaderRegister = 0;//셰이더 레지스터 0번에 루트 상수를 사용한다.
    pd3dRootParameters[0].Constants.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//루트 상수는 버텍스 셰이더에서 사용한다.

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;//루트 상수
	pd3dRootParameters[1].Constants.Num32BitValues = 32;//32개의 32비트 정수값을 루트 상수로 사용한다.
	pd3dRootParameters[1].Constants.ShaderRegister = 1;//셰이더 레지스터 1번에 루트 상수를 사용한다.
    pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;//루트 상수는 버텍스 셰이더에서 사용한다.
    D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | //입력 어셈블러 입력 레이아웃을 허용한다.
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |//헐 셰이더 루트 접근을 거부한다.
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |//도메인 셰이더 루트 접근을 거부한다.
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |//지오메트리 셰이더 루트 접근을 거부한다.
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;//픽셀 셰이더 루트 접근을 거부한다.

    
    D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
    ::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
    d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);//2
    d3dRootSignatureDesc.pParameters = pd3dRootParameters;
    d3dRootSignatureDesc.NumStaticSamplers = 0;
    d3dRootSignatureDesc.pStaticSamplers = NULL;
    d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;
    ID3DBlob *pd3dSignatureBlob = NULL;
    ID3DBlob *pd3dErrorBlob = NULL;
    ::D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1,&pd3dSignatureBlob, &pd3dErrorBlob);
    if (pd3dErrorBlob)
    {
        OutputDebugStringA((char*)pd3dErrorBlob->GetBufferPointer());
		__debugbreak(); // 디버그 모드에서 오류를 출력하고 중단
    }
    pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(),
                                    pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pd3dGraphicsRootSignature);
    if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
    if (pd3dErrorBlob) pd3dErrorBlob->Release();
    return(pd3dGraphicsRootSignature);
}

ID3D12RootSignature* CScene::GetGraphicsRootSignature()
{
    return (m_pd3dGraphicsRootSignature);
}


void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
    m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
    m_nShaders = 1;
    m_pShaders = new CInstancingShader[m_nShaders];
    m_pShaders[0].CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
    m_pShaders[0].BuildObjects(pd3dDevice, pd3dCommandList);
}

void CScene::ReleaseObjects()
{
    if (m_pd3dGraphicsRootSignature)
    {
        m_pd3dGraphicsRootSignature->Release();
    }
    for (int i = 0; i < m_nShaders; i++)
    {
        m_pShaders[i].ReleaseShaderVariables();
        m_pShaders[i].ReleaseObjects();
    }

    delete[] m_pShaders;
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
    return(false);
}
void CScene::ReleaseUploadBuffers()
{
    for (int i = 0; i < m_nShaders; i++) 
        m_pShaders[i].ReleaseUploadBuffers();
}


void CScene::AnimateObjects(float fTimeElapsed)
{
    for (int i = 0; i < m_nShaders; i++)
    {
        m_pShaders[i].AnimateObjects(fTimeElapsed);
    }
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    pCamera->SetViewportsAndScissorRects(pd3dCommandList);
    pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
    pCamera->UpdateShaderVariables(pd3dCommandList);
    for (int i = 0; i < m_nShaders; i++)
    {
        m_pShaders[i].Render(pd3dCommandList, pCamera);
    }
}
