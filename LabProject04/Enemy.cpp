// Enemy.cpp

#include "stdafx.h"
#include "Enemy.h"
#include "Mesh.h" // CObjMesh를 사용하기 위해 포함
#include "Scene.h"

// 탱크 재질 생성을 위한 헬퍼 함수
CMaterial* CreateEnemyTankMaterial(XMFLOAT4 xmf4Color)
{
	CMaterial* pMaterial = new CMaterial();
	MATERIALLOADINFO materialInfo;
	materialInfo.m_xmf4AlbedoColor = xmf4Color;
	materialInfo.m_xmf4SpecularColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 10.0f);
	pMaterial->SetMaterialColors(new CMaterialColors(&materialInfo));
	pMaterial->SetIlluminatedShader();
	return pMaterial;
}

CEnemyTank::CEnemyTank(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, const XMFLOAT3& xmf3Position)
{

	m_pContext = pContext; // 지형 포인터 저장

	m_xmf3Position = xmf3Position;
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	// 몸체와 포탑을 조립 (CTankPlayer와 동일한 방식)
	m_pBody = new CGameObject(1);
	strcpy_s(m_pBody->m_pstrFrameName, "EnemyBody");
	m_pBody->SetMesh(new CObjMesh(pd3dDevice, pd3dCommandList, "../Assets/M26 Body.obj"));
	m_pBody->SetMaterial(0, CreateEnemyTankMaterial(XMFLOAT4(0.5f, 0.2f, 0.2f, 1.0f))); // 적 탱크는 붉은색 계열
	SetChild(m_pBody, true);

	m_pTurret = new CGameObject(1);
	strcpy_s(m_pTurret->m_pstrFrameName, "EnemyTurret");
	m_pTurret->SetMesh(new CObjMesh(pd3dDevice, pd3dCommandList, "../Assets/M26 Turret.obj"));
	m_pTurret->SetMaterial(0, CreateEnemyTankMaterial(XMFLOAT4(0.6f, 0.25f, 0.25f, 1.0f)));
	m_pTurret->SetPosition(0.0f, 0.f, 0.0f);
	m_pBody->SetChild(m_pTurret, true);

	SetScale(30.0f, 30.0f, 30.0f);
}

CEnemyTank::~CEnemyTank()
{}

void CEnemyTank::Animate(float fTimeElapsed, CScene* pScene)
{
	// 1. 플레이어의 위치를 가져옵니다.
	CPlayer* pPlayer = pScene->GetPlayer();
	XMFLOAT3 xmf3PlayerPosition = pPlayer->GetPosition();

	// 2. 적 탱크에서 플레이어로 향하는 방향 벡터(목표 방향)를 계산합니다.
	XMFLOAT3 xmf3TargetDirection = Vector3::Normalize(Vector3::Subtract(xmf3PlayerPosition, m_xmf3Position));

	// 3. 현재 Look 벡터에서 목표 방향으로 부드럽게 회전(보간)합니다.
	float fRotationLerpRatio = 3.0f * fTimeElapsed; // 회전 속도. 이 값을 조절하여 반응 속도를 바꿀 수 있습니다.
	if (fRotationLerpRatio > 1.0f) fRotationLerpRatio = 1.0f;
	m_xmf3Look = Vector3::Normalize(Vector3::Lerp(m_xmf3Look, xmf3TargetDirection, fRotationLerpRatio));

	// 4. 수평 방향으로만 앞으로 이동합니다.
	XMFLOAT3 xmf3MoveVector = m_xmf3Look;
	xmf3MoveVector.y = 0.0f; // Y값은 무시하여 경사면에서 속도가 빨라지는 것을 방지
	m_xmf3Position = Vector3::Add(m_xmf3Position, Vector3::ScalarProduct(xmf3MoveVector, m_fSpeed * fTimeElapsed, false));

	// 5. 지형 높이 및 경사 맞추기 (플레이어와 동일한 로직)
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pContext;
	float fHeight = pTerrain->GetHeight(m_xmf3Position.x, m_xmf3Position.z) + 5.0f;
	m_xmf3Position.y = fHeight;

	XMFLOAT3 xmf3TerrainNormal = pTerrain->GetNormal(m_xmf3Position.x, m_xmf3Position.z);
	m_xmf3Up = xmf3TerrainNormal;
	m_xmf3Look = Vector3::Normalize(Vector3::Subtract(m_xmf3Look, Vector3::ScalarProduct(xmf3TerrainNormal, Vector3::DotProduct(m_xmf3Look, xmf3TerrainNormal))));
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);

	// 6. 플레이어가 가까이 있으면 포탑도 플레이어를 향하도록 회전시킵니다.
	if (Vector3::Length(xmf3TargetDirection) < 800.0f && m_pTurret)
	{
		XMMATRIX mtxLookAt = XMMatrixLookAtLH(XMLoadFloat3(&m_pTurret->GetPosition()), XMLoadFloat3(&xmf3PlayerPosition), XMLoadFloat3(&m_xmf3Up));
		XMMATRIX mtxInverseLookAt = XMMatrixInverse(NULL, mtxLookAt);

		// 부모(몸체)의 회전을 고려하여 포탑의 로컬 회전값을 계산합니다.
		CGameObject *pBody = GetParent();
		if (pBody)
		{
			XMMATRIX mtxBodyWorld = XMLoadFloat4x4(&pBody->m_xmf4x4World);
			XMMATRIX mtxInvBodyWorld = XMMatrixInverse(NULL, mtxBodyWorld);
			XMStoreFloat4x4(&m_pTurret->m_xmf4x4Transform, mtxInvBodyWorld * mtxInverseLookAt);
		}
	}

	// 7. 발사 AI (이전과 동일)
	m_fFireTimer += fTimeElapsed;
	if (m_fFireTimer > m_fFireDelay)
	{
		if (Vector3::Length(xmf3TargetDirection) < 800.0f)
		{
			pScene->Fire(this);
			m_fFireTimer = 0.0f;
			m_fFireDelay = (rand() % 3) + 3.0f;
		}
	}

	// 부모의 Animate 함수 호출 (자식 객체들도 함께 움직이도록)
	CGameObject::Animate(fTimeElapsed, NULL);
}

void CEnemyTank::OnPrepareRender()
{
	// CPlayer와 동일하게, 렌더링 직전에 최종 위치와 방향으로 행렬을 업데이트합니다.
	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4Transform._11 = m_xmf3Right.x;    m_xmf4x4Transform._12 = m_xmf3Right.y;    m_xmf4x4Transform._13 = m_xmf3Right.z;
	m_xmf4x4Transform._21 = m_xmf3Up.x;       m_xmf4x4Transform._22 = m_xmf3Up.y;       m_xmf4x4Transform._23 = m_xmf3Up.z;
	m_xmf4x4Transform._31 = m_xmf3Look.x;     m_xmf4x4Transform._32 = m_xmf3Look.y;     m_xmf4x4Transform._33 = m_xmf3Look.z;
	m_xmf4x4Transform._41 = m_xmf3Position.x; m_xmf4x4Transform._42 = m_xmf3Position.y; m_xmf4x4Transform._43 = m_xmf3Position.z;

	UpdateTransform(NULL);
}