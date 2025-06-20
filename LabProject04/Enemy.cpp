// Enemy.cpp

#include "stdafx.h"
#include "Enemy.h"
#include "Mesh.h" // CObjMesh를 사용하기 위해 포함

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

CEnemyTank::CEnemyTank(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext) : CGameObject(2)
{
	m_pContext = pContext; // 지형 포인터 저장

	// 기본 방향 벡터 초기화
	m_xmf3Position = XMFLOAT3(rand() % 500, 0.0f, rand() % 500); // 맵의 랜덤한 위치에서 시작
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
	m_pTurret->SetPosition(0.0f, 4.5f, 0.0f);
	m_pBody->SetChild(m_pTurret, true);

	SetScale(30.0f, 30.0f, 30.0f);
}

CEnemyTank::~CEnemyTank()
{}

void CEnemyTank::Animate(float fTimeElapsed)
{
	// 1. 앞으로 이동
	m_xmf3Position = Vector3::Add(m_xmf3Position, Vector3::ScalarProduct(m_xmf3Look, m_fSpeed * fTimeElapsed));

	// 2. 맵 경계를 벗어나면 랜덤하게 회전 (간단한 AI)
	if (m_xmf3Position.x < 10.0f || m_xmf3Position.x > 2000.0f ||
		m_xmf3Position.z < 10.0f || m_xmf3Position.z > 2000.0f)
	{
		float fYaw = (rand() % 180) - 90.0f; // -90 ~ 90도 사이로 회전
		XMMATRIX mtxRotate = XMMatrixRotationY(XMConvertToRadians(fYaw));
		m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, mtxRotate);
	}

	// 3. 지형 높이 및 경사 맞추기
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pContext;
	float fHeight = pTerrain->GetHeight(m_xmf3Position.x, m_xmf3Position.z) + 5.0f;
	m_xmf3Position.y = fHeight;

	XMFLOAT3 xmf3TerrainNormal = pTerrain->GetNormal(m_xmf3Position.x, m_xmf3Position.z);
	m_xmf3Up = xmf3TerrainNormal;
	m_xmf3Look = Vector3::Normalize(Vector3::Subtract(m_xmf3Look, Vector3::ScalarProduct(xmf3TerrainNormal, Vector3::DotProduct(m_xmf3Look, xmf3TerrainNormal))));
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);

	// 부모의 Animate 함수 호출 (자식 객체들도 함께 움직이도록)
	CGameObject::Animate(fTimeElapsed);
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