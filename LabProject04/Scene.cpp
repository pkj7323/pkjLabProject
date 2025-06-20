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
    //지형을 확대할 스케일 벡터이다. x-축과 z-축은 8배, y-축은 2배 확대한다.
    XMFLOAT3 xmf3Scale(8.0f, 2.0f, 8.0f);
    XMFLOAT4 xmf4Color(0.0f, 0.2f, 0.0f, 0.0f);
    //지형을 높이 맵 이미지 파일(HeightMap.raw)을 사용하여 생성한다. 높이 맵의 크기는 가로x세로(257x257)이다.
#ifdef _WITH_TERRAIN_PARTITION
/*하나의 격자 메쉬의 크기는 가로x세로(17x17)이다. 지형 전체는 가로 방향으로 16개, 세로 방향으로 16의 격자 메
쉬를 가진다. 지형을 구성하는 격자 메쉬의 개수는 총 256(16x16)개가 된다.*/
    m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList,
                                       m_pd3dGraphicsRootSignature, _T("../Assets/Image/Terrain/HeightMap.raw"), 257, 257, 17,
                                       17, xmf3Scale, xmf4Color);
#else
//지형을 하나의 격자 메쉬(257x257)로 생성한다.  
    m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList,
                                       m_pd3dGraphicsRootSignature, _T("C:/Users/MSI/source/repos/pkjLabProject/LabProject04/Assets/Image/Terrain/HeightMap.raw"), 257, 257, 257,
                                       257, xmf3Scale, xmf4Color);
#endif
    m_nShaders = 1;
    m_pShaders = new CObjectsShader[m_nShaders];
    m_pShaders[0].CreateShader(pd3dDevice, m_pd3dGraphicsRootSignature);
    m_pShaders[0].BuildObjects(pd3dDevice, pd3dCommandList, m_pTerrain);
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
	delete m_pObjectsShaders;
    delete[] m_pShaders;
    if (m_pTerrain) delete m_pTerrain;
}

bool CScene::ProcessInput(UCHAR* pKeysBuffer)
{
    return(false);
}
void CScene::ReleaseUploadBuffers()
{
    if (m_pObjectsShaders) 
		m_pObjectsShaders->ReleaseUploadBuffers();
    for (int i = 0; i < m_nShaders; i++) 
        m_pShaders[i].ReleaseUploadBuffers();
    if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
}


void CScene::AnimateObjects(float fTimeElapsed)
{
	m_pObjectsShaders->AnimateObjects(fTimeElapsed);
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
	m_pObjectsShaders->Render(pd3dCommandList, pCamera);
    for (int i = 0; i < m_nShaders; i++)
    {
        m_pShaders[i].Render(pd3dCommandList, pCamera);
    }
    if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera);
}
