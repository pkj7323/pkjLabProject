// Scene.cpp

#include "stdafx.h"
#include "Scene.h"
#include "Mesh.h" // CHeightMapTerrain 클래스를 사용하기 위해 포함

CScene::CScene()
{
	m_pd3dGraphicsRootSignature = NULL;
	m_pPlayer = NULL;
	m_pTerrain = NULL;
	m_pLights = NULL;
	m_nLights = 0;
	m_xmf4GlobalAmbient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_pd3dcbLights = NULL;
	m_pcbMappedLights = NULL;
	m_fElapsedTime = 0.0f;
}

CScene::~CScene()
{}

void CScene::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	m_pd3dGraphicsRootSignature = CreateGraphicsRootSignature(pd3dDevice);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	m_pTerrain = new CHeightMapTerrain(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, _T("../Assets/Image/Terrain/HeightMap.raw"), 257, 257, 17, 17, XMFLOAT3(8.0f, 2.0f, 8.0f));
	//m_pTestCubeMesh = new CCubeMeshDiffused(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 20.0f);

	m_nEnemyTanks = 10; // 5대의 적 탱크를 생성
	m_ppEnemyTanks = new CEnemyTank * [m_nEnemyTanks];
	for (int i = 0; i < m_nEnemyTanks; i++)
	{
		// 플레이어 앞쪽에 일렬로 배치합니다.
		XMFLOAT3 xmf3EnemyPos = XMFLOAT3(1000.0f + (i * 60.0f) - 120.0f, 0.0f, 1200.0f);
		m_ppEnemyTanks[i] = new CEnemyTank(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pTerrain, xmf3EnemyPos);
	}
	// 1. 모든 총알이 함께 사용할 메쉬와 재질을 한 번만 만듭니다.
	m_pBulletMesh = new CObjMesh(pd3dDevice, pd3dCommandList,"../Assets/Box.obj",5.f,false);
	m_pBulletMaterial = CreateBulletMaterial(); // Bullet.cpp에 만들었던 Helper 함수 사용

	// 2. 플레이어 총알 풀을 생성하고 초기화합니다.
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
	{
		m_pPlayerBullets[i] = new CBullet(pd3dDevice, pd3dCommandList);
		m_pPlayerBullets[i]->SetMesh(m_pBulletMesh);
		m_pPlayerBullets[i]->SetMaterial(0, m_pBulletMaterial);
		m_pPlayerBullets[i]->SetActive(false); // 처음에는 모두 비활성화
	}

	// 3. 적 탱크 총알 풀을 생성하고 초기화합니다.
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
	{
		m_pEnemyBullets[i] = new CBullet(pd3dDevice, pd3dCommandList);
		m_pEnemyBullets[i]->SetMesh(m_pBulletMesh);
		m_pEnemyBullets[i]->SetMaterial(0, m_pBulletMaterial);
		m_pEnemyBullets[i]->SetActive(false);
	}
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 4;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = POINT_LIGHT;
	m_pLights[0].m_fRange = 1000.0f;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.1f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(30.0f, 30.0f, 30.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_pLights[0].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.001f, 0.0001f);
	m_pLights[1].m_bEnable = true;
	m_pLights[1].m_nType = SPOT_LIGHT;
	m_pLights[1].m_fRange = 500.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-50.0f, 20.0f, -5.0f);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[1].m_fFalloff = 8.0f;
	m_pLights[1].m_fPhi = (float)cos(XMConvertToRadians(40.0f));
	m_pLights[1].m_fTheta = (float)cos(XMConvertToRadians(20.0f));
	m_pLights[2].m_bEnable = true;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 0.0f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_pLights[3].m_bEnable = true;
	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(50.0f, 30.0f, 30.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));
}

void CScene::ReleaseObjects()
{
	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();
	if (m_pTerrain) m_pTerrain->Release();
	delete[] m_pLights;

	if (m_ppEnemyTanks)
	{
		for (int i = 0; i < m_nEnemyTanks; i++)
		{
			if (m_ppEnemyTanks[i]) delete m_ppEnemyTanks[i];
		}
		delete[] m_ppEnemyTanks;
	}
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++) if (m_pPlayerBullets[i]) delete m_pPlayerBullets[i];
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++) if (m_pEnemyBullets[i]) delete m_pEnemyBullets[i];


	//if (m_pTestCubeMesh) m_pTestCubeMesh->Release(); // <-- 이 줄 추가
	ReleaseShaderVariables();
}

ID3D12RootSignature *CScene::CreateGraphicsRootSignature(ID3D12Device *pd3dDevice)
{
	ID3D12RootSignature *pd3dGraphicsRootSignature = NULL;

	D3D12_ROOT_PARAMETER pd3dRootParameters[3];

	// Camera (HLSL의 register(b0)와 일치)
	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 0; // 1에서 0으로 수정
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// GameObject (HLSL의 register(b2)와 일치)
	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pd3dRootParameters[1].Constants.Num32BitValues = 32;
	pd3dRootParameters[1].Constants.ShaderRegister = 2; // 그대로 2
	pd3dRootParameters[1].Constants.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// Lights (HLSL의 register(b4)와 일치)
	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; // 그대로 4
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = 0;
	d3dRootSignatureDesc.pStaticSamplers = NULL;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob *pd3dSignatureBlob = NULL;
	ID3DBlob *pd3dErrorBlob = NULL;
	D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void **)&pd3dGraphicsRootSignature);
	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();

	return(pd3dGraphicsRootSignature);
}

void CScene::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); // 256의 배수
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	m_pd3dcbLights->Map(0, NULL, (void **)&m_pcbMappedLights);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}
}

void CScene::ReleaseUploadBuffers()
{
	if (m_pTerrain) m_pTerrain->ReleaseUploadBuffers();
}

void CScene::Fire(CGameObject* pFiredBy)
{
	CBullet** ppBullets = NULL;
	int nMaxPoolSize = 0;

	// 발사 주체가 플레이어인지 적인지 확인하여 사용할 풀을 결정
	if (dynamic_cast<CTankPlayer*>(pFiredBy))
	{
		ppBullets = m_pPlayerBullets;
		nMaxPoolSize = MAX_PLAYER_BULLETS;
	}
	else if (dynamic_cast<CEnemyTank*>(pFiredBy))
	{
		ppBullets = m_pEnemyBullets;
		nMaxPoolSize = MAX_ENEMY_BULLETS;
	}
	else
	{
		return;
	}

	// 해당 풀에서 비활성화된 총알을 찾습니다.
	for (int i = 0; i < nMaxPoolSize; i++)
	{
		if (!ppBullets[i]->IsActive())
		{
			// 비활성화된 총알을 찾으면, Reset 함수를 호출하여 재사용합니다.
			CTankPlayer* pTank = dynamic_cast<CTankPlayer*>(pFiredBy);
			CGameObject* pTurret = (pTank) ? pTank->GetTurret() : ((CEnemyTank*)pFiredBy)->GetTurret();

			if (pTurret)
			{
				ppBullets[i]->Reset(pFiredBy, pTurret->GetPosition(), pTurret->GetLook(), pTurret->GetUp());
			}
			break;
		}
	}
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;

	// 플레이어가 있으면, 플레이어를 따라다니는 조명의 위치와 방향을 갱신합니다.
	if (m_pPlayer)
	{
		// m_pLights[1]이 유효한지 확인하는 방어 코드 추가
		if (m_nLights > 1 && m_pLights)
		{
			m_pLights[1].m_xmf3Position = m_pPlayer->GetPosition();
			m_pLights[1].m_xmf3Direction = m_pPlayer->GetLookVector();
		}
	}

	// 모든 적 탱크들의 Animate 함수 호출
	for (int i = 0; i < m_nEnemyTanks; i++)
	{
		if (m_ppEnemyTanks[i]->IsActive())
			m_ppEnemyTanks[i]->Animate(fTimeElapsed, this);
	}

	// 모든 플레이어 총알들의 Animate 함수 호출
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
	{
		if (m_pPlayerBullets[i]->IsActive()) m_pPlayerBullets[i]->Animate(fTimeElapsed);
	}

	// 모든 적 총알들의 Animate 함수 호출
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
	{
		if (m_pEnemyBullets[i]->IsActive()) m_pEnemyBullets[i]->Animate(fTimeElapsed);
	}

	// 1. 플레이어의 총알과 적 탱크의 충돌을 검사합니다.
	for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
	{
		// 총알이 활성화 상태일 때만 검사합니다.
		if (m_pPlayerBullets[i]->IsActive())
		{
			for (int j = 0; j < m_nEnemyTanks; j++)
			{
				// 적 탱크가 활성화 상태일 때만 검사합니다.
				if (m_ppEnemyTanks[j]->IsActive())
				{
					// CheckCollision 함수로 두 객체의 바운딩 박스가 겹치는지 확인합니다.
					if (m_ppEnemyTanks[j]->CheckCollision(m_pPlayerBullets[i]))
					{
						// 충돌이 발생하면, 총알과 적 탱크를 모두 비활성화시켜 화면에서 사라지게 합니다.
						m_pPlayerBullets[i]->SetActive(false);
						m_ppEnemyTanks[j]->SetActive(false);
						// OutputDebugStringA("Hit");
						// 한 총알이 여러 탱크를 동시에 파괴하지 않도록 break로 내부 루프를 빠져나옵니다.
						break;
					}
				}
			}
		}
	}

	// 2. 여기에 "적 총알과 플레이어의 충돌" 로직도 나중에 추가할 수 있습니다.
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
	{
		if (m_pEnemyBullets[i]->IsActive())
		{
			// 플레이어가 활성화 상태일 때만 검사합니다.
			if (m_pPlayer && m_pPlayer->IsActive())
			{
				if (m_pEnemyBullets[i]->CheckCollision(m_pPlayer))
				{
					m_pEnemyBullets[i]->SetActive(false);
					// 여기에 플레이어 HP 감소 또는 게임 오버 로직을 추가할 수 있습니다.
					// 예: m_pPlayer->OnDamaged(10);
				}
			}
		}
	}
}

void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress); // Lights (register b4)

	if (m_pTerrain) m_pTerrain->Render(pd3dCommandList, pCamera);

	for (int i = 0; i < m_nEnemyTanks; i++)
	{
		m_ppEnemyTanks[i]->Render(pd3dCommandList, pCamera);
	}

	for (int i = 0; i < MAX_PLAYER_BULLETS; i++)
	{
		if (m_pPlayerBullets[i]->IsActive()) m_pPlayerBullets[i]->Render(pd3dCommandList, pCamera);
	}
	// 적 총알 Render
	for (int i = 0; i < MAX_ENEMY_BULLETS; i++)
	{
		if (m_pEnemyBullets[i]->IsActive()) m_pEnemyBullets[i]->Render(pd3dCommandList, pCamera);
	}
	//if (m_pTestCubeMesh)
	//{
	//	// 임시 재질을 만들고 조명 셰이더를 설정합니다.
	//	CMaterial tempMaterial;
	//	tempMaterial.SetIlluminatedShader();
	//	MATERIALLOADINFO matInfo; // 기본 흰색 재질 정보
	//	CMaterialColors* colors = new CMaterialColors(&matInfo);
	//	tempMaterial.SetMaterialColors(colors);

	//	// 파이프라인 상태(PSO)를 설정합니다.
	//	tempMaterial.m_pShader->OnPrepareRender(pd3dCommandList);

	//	// 큐브의 위치를 카메라 앞 100 유닛으로 설정하는 월드 행렬을 만듭니다.
	//	XMFLOAT4X4 mtxWorld = Matrix4x4::Identity();
	//	XMFLOAT3 look = pCamera->GetLookVector();
	//	XMFLOAT3 pos = pCamera->GetPosition();
	//	mtxWorld._41 = pos.x + look.x * 100.0f;
	//	mtxWorld._42 = pos.y + look.y * 100.0f;
	//	mtxWorld._43 = pos.z + look.z * 100.0f;

	//	// 월드 행렬과 재질 정보를 셰이더로 넘깁니다.
	//	CGameObject tempObject;
	//	tempObject.UpdateShaderVariable(pd3dCommandList, &mtxWorld);
	//	tempMaterial.UpdateShaderVariable(pd3dCommandList);

	//	// 큐브 메쉬를 렌더링합니다.
	//	m_pTestCubeMesh->Render(pd3dCommandList);
	//}
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) { return(false); }
bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) { return(false); }