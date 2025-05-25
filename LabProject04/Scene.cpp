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
    return(m_pd3dGraphicsRootSignature);
}


void CScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
    m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);
    //가로x세로x깊이가 12x12x12인 정육면체 메쉬를 생성한다.
    CCubeMeshDiffused *pCubeMesh = new CCubeMeshDiffused(pd3dDevice, pd3dCommandList,
                                                         12.0f, 12.0f, 12.0f);
    m_nObjects = 1;
    m_ppObjects = new CGameObject * [m_nObjects];
    CRotatingObject *pRotatingObject = new CRotatingObject();
    pRotatingObject->SetMesh(pCubeMesh);
    CDiffusedShader *pShader = new CDiffusedShader();
    pShader->CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
    pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
    pRotatingObject->SetShader(pShader);
    m_ppObjects[0] = pRotatingObject;
}

void CScene::ReleaseObjects()
{
    if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
    if (m_ppObjects)
    {
        for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) delete m_ppObjects[j];
        delete[] m_ppObjects;
    }
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
    return(false);
}
void CScene::ReleaseUploadBuffers()
{
    if (m_ppObjects)
    {
        for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j])
            m_ppObjects[j]->ReleaseUploadBuffers();
    }
}


void CScene::AnimateObjects(float fTimeElapsed)
{
    for (int j = 0; j < m_nObjects; j++)
    {
        m_ppObjects[j]->Animate(fTimeElapsed);
    }
}

void CScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    pCamera->SetViewportsAndScissorRects(pd3dCommandList);
    pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
    if (pCamera) pCamera->UpdateShaderVariables(pd3dCommandList);
    //씬을 렌더링하는 것은 씬을 구성하는 게임 객체(셰이더를 포함하는 객체)들을 렌더링하는 것이다.
    for (int j = 0; j < m_nObjects; j++)
    {
        if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, pCamera);
    }
}
