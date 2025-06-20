// Player.cpp

#include "stdafx.h"
#include "Player.h"
#include "GameObject.h" // CHeightMapTerrain 클래스를 알기 위해 포함
#include "Mesh.h"

// CPlayer 클래스 구현 (기본 이동 및 물리)
CPlayer::CPlayer(int nMeshes) : CGameObject(nMeshes)
{
	m_pCamera = NULL;
	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fMaxVelocityXZ = 0.0f;
	m_fMaxVelocityY = 0.0f;
	m_fFriction = 0.0f;
	m_fPitch = 0.0f;
	m_fRoll = 0.0f;
	m_fYaw = 0.0f;
	m_pPlayerUpdatedContext = NULL;
	m_pCameraUpdatedContext = NULL;
}
CPlayer::~CPlayer()
{
	if (m_pCamera) delete m_pCamera;
}

void CPlayer::SetPosition(const XMFLOAT3& xmf3Position)
{
	Move(XMFLOAT3(xmf3Position.x - m_xmf3Position.x, xmf3Position.y - m_xmf3Position.y, xmf3Position.z - m_xmf3Position.z), false);
}

void CPlayer::Move(DWORD dwDirection, float fDistance, bool bUpdateVelocity)
{
	if (dwDirection)
	{
		XMFLOAT3 xmf3Shift = XMFLOAT3(0, 0, 0);
		if (dwDirection & DIR_FORWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, fDistance);
		if (dwDirection & DIR_BACKWARD) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Look, -fDistance);
		if (dwDirection & DIR_RIGHT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, fDistance);
		if (dwDirection & DIR_LEFT) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Right, -fDistance);
		if (dwDirection & DIR_UP) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, fDistance);
		if (dwDirection & DIR_DOWN) xmf3Shift = Vector3::Add(xmf3Shift, m_xmf3Up, -fDistance);
		Move(xmf3Shift, bUpdateVelocity);
	}
}
void CPlayer::Move(const XMFLOAT3& xmf3Shift, bool bUpdateVelocity)
{
	if (bUpdateVelocity)
	{
		m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Shift);
	}
	else
	{
		m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);
		if (m_pCamera) m_pCamera->Move(xmf3Shift);
	}
}

void CPlayer::Rotate(float x, float y, float z)
{
	DWORD nCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if ((nCameraMode == FIRST_PERSON_CAMERA) || (nCameraMode == THIRD_PERSON_CAMERA))
	{
		if (x != 0.0f)
		{
			m_fPitch += x;
			if (m_fPitch > +89.0f) { x -= (m_fPitch - 89.0f); m_fPitch = +89.0f; }
			if (m_fPitch < -89.0f) { x -= (m_fPitch + 89.0f); m_fPitch = -89.0f; }
		}
		if (y != 0.0f)
		{
			m_fYaw += y;
			if (m_fYaw > 360.0f) m_fYaw -= 360.0f;
			if (m_fYaw < 0.0f) m_fYaw += 360.0f;
		}
		if (z != 0.0f)
		{
			m_fRoll += z;
			if (m_fRoll > +20.0f) { z -= (m_fRoll - 20.0f); m_fRoll = +20.0f; }
			if (m_fRoll < -20.0f) { z -= (m_fRoll + 20.0f); m_fRoll = -20.0f; }
		}
		m_pCamera->Rotate(x, y, z);
		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}

	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);
	m_xmf3Up = Vector3::CrossProduct(m_xmf3Look, m_xmf3Right, true);
}

void CPlayer::Update(float fTimeElapsed)
{
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Gravity, fTimeElapsed, false));

	float fLength = sqrtf(m_xmf3Velocity.x * m_xmf3Velocity.x + m_xmf3Velocity.z * m_xmf3Velocity.z);
	float fMaxVelocityXZ = m_fMaxVelocityXZ * fTimeElapsed;
	if (fLength > m_fMaxVelocityXZ)
	{
		m_xmf3Velocity.x *= (fMaxVelocityXZ / fLength);
		m_xmf3Velocity.z *= (fMaxVelocityXZ / fLength);
	}

	float fMaxVelocityY = m_fMaxVelocityY * fTimeElapsed;
	fLength = sqrtf(m_xmf3Velocity.y * m_xmf3Velocity.y);
	if (fLength > m_fMaxVelocityY) m_xmf3Velocity.y *= (fMaxVelocityY / fLength);

	Move(m_xmf3Velocity, false);

	if (m_pPlayerUpdatedContext) OnPlayerUpdateCallback(fTimeElapsed);

	DWORD nCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCameraMode == THIRD_PERSON_CAMERA) m_pCamera->Update(m_xmf3Position, fTimeElapsed);
	if (m_pCameraUpdatedContext) OnCameraUpdateCallback(fTimeElapsed);
	if (nCameraMode == THIRD_PERSON_CAMERA) m_pCamera->SetLookAt(m_xmf3Position);
	m_pCamera->RegenerateViewMatrix();

	fLength = Vector3::Length(m_xmf3Velocity);
	float fDeceleration = (m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));
}

CCamera* CPlayer::OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode)
{
	CCamera* pNewCamera = NULL;
	switch (nNewCameraMode)
	{
		case FIRST_PERSON_CAMERA:
			pNewCamera = new CFirstPersonCamera(m_pCamera);
			break;
		case THIRD_PERSON_CAMERA:
			pNewCamera = new CThirdPersonCamera(m_pCamera);
			break;
	}

	if (pNewCamera)
	{
		pNewCamera->SetMode(nNewCameraMode);
		pNewCamera->SetPlayer(this);
	}

	if (m_pCamera) delete m_pCamera;
	return(pNewCamera);
}
void CPlayer::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pCamera) m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

// CTerrainPlayer 클래스 구현 (지형 상호작용 로직)
CTerrainPlayer::CTerrainPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext) : CPlayer(0)
{
	// 비워둠. 실제 구현은 CTankPlayer에서 진행
}

CTerrainPlayer::~CTerrainPlayer() {}

CCamera* CTerrainPlayer::ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed)
{
	DWORD nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);

	switch (nNewCameraMode)
	{
		case FIRST_PERSON_CAMERA:
			SetFriction(250.0f);
			SetGravity(XMFLOAT3(0.0f, -250.0f, 0.0f));
			SetMaxVelocityXZ(300.0f);
			SetMaxVelocityY(400.0f);
			m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.0f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 15.0f, 5.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			break;
		case THIRD_PERSON_CAMERA:
			SetFriction(250.0f);
			SetGravity(XMFLOAT3(0.0f, -250.0f, 0.0f));
			SetMaxVelocityXZ(300.0f);
			SetMaxVelocityY(400.0f);
			m_pCamera = OnChangeCamera(THIRD_PERSON_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.25f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 35.0f, -60.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			break;
	}
	m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
	Update(fTimeElapsed);
	return(m_pCamera);
}

void CTerrainPlayer::OnPlayerUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pPlayerUpdatedContext;
	XMFLOAT3 xmf3PlayerPosition = GetPosition();

	float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z) + 5.0f;

	if (xmf3PlayerPosition.y < fHeight)
	{
		XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
		xmf3PlayerVelocity.y = 0.0f;
		SetVelocity(xmf3PlayerVelocity);
		xmf3PlayerPosition.y = fHeight;
		SetPosition(xmf3PlayerPosition);
	}
}

void CTerrainPlayer::OnCameraUpdateCallback(float fTimeElapsed)
{
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)m_pCameraUpdatedContext;
	XMFLOAT3 xmf3CameraPosition = m_pCamera->GetPosition();

	float fHeight = pTerrain->GetHeight(xmf3CameraPosition.x, xmf3CameraPosition.z) + 5.0f;
	if (xmf3CameraPosition.y <= fHeight)
	{
		xmf3CameraPosition.y = fHeight;
		m_pCamera->SetPosition(xmf3CameraPosition);
		if (m_pCamera->GetMode() == THIRD_PERSON_CAMERA)
		{
			CThirdPersonCamera* p3rdPersonCamera = (CThirdPersonCamera*)m_pCamera;
			p3rdPersonCamera->SetLookAt(GetPosition());
		}
	}
}

// CTankPlayer 클래스 구현 (탱크 모델 로딩 및 설정)
//CTankPlayer::CTankPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext)
//	: CTerrainPlayer(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pContext)
//{
//	m_pCamera = ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
//
//	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)pContext;
//	float fHeight = pTerrain->GetHeight(pTerrain->GetWidth() * 0.5f, pTerrain->GetLength() * 0.5f) + 5.0f;
//	SetPosition(XMFLOAT3(pTerrain->GetWidth() * 0.5f, fHeight, pTerrain->GetLength() * 0.5f));
//
//	SetPlayerUpdatedContext(pContext);
//	SetCameraUpdatedContext(pContext);
//
//	CM26Object* pTankModel = new CM26Object();
//	// Player.cpp 의 CTankPlayer 생성자 안
//	pTankModel->SetChild(CGameObject::LoadGeometryFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/M26.bin"));
//	pTankModel->OnInitialize();
//	pTankModel->SetScale(0.2f, 0.2f, 0.2f);
//
//	SetChild(pTankModel, true);
//}

// Player.cpp

CTankPlayer::CTankPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext)
	: CTerrainPlayer(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pContext)
{
	m_pCamera = ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);

	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)pContext;
	float fHeight = pTerrain->GetHeight(pTerrain->GetWidth() * 0.5f, pTerrain->GetLength() * 0.5f) + 5.0f;
	SetPosition(XMFLOAT3(pTerrain->GetWidth() * 0.5f, fHeight, pTerrain->GetLength() * 0.5f));

	SetPlayerUpdatedContext(pContext);
	SetCameraUpdatedContext(pContext);

	// --- 탱크 모델을 파일에서 로드하는 대신, 코드로 직접 생성합니다 ---

	// 1. 탱크 몸체 (Body) 만들기
	CCubeMeshDiffused* pBodyMesh = new CCubeMeshDiffused(pd3dDevice, pd3dCommandList, 10.0f, 5.0f, 18.0f);
	CGameObject* pBody = new CGameObject(1);
	pBody->SetMesh(pBodyMesh);

	// 몸체 재질 설정 (짙은 녹색)
	CMaterial* pBodyMaterial = new CMaterial();
	MATERIALLOADINFO bodyMaterialInfo;
	bodyMaterialInfo.m_xmf4AlbedoColor = XMFLOAT4(0.2f, 0.4f, 0.2f, 1.0f);
	bodyMaterialInfo.m_xmf4SpecularColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 10.0f);
	pBodyMaterial->SetMaterialColors(new CMaterialColors(&bodyMaterialInfo));
	pBodyMaterial->SetIlluminatedShader();
	pBody->SetMaterial(0, pBodyMaterial);

	// 플레이어(보이지 않는 컨트롤러)의 자식으로 몸체를 붙입니다.
	SetChild(pBody, true);

	// 2. 탱크 포탑 (Turret) 만들기
	CCubeMeshDiffused* pTurretMesh = new CCubeMeshDiffused(pd3dDevice, pd3dCommandList, 8.0f, 4.0f, 8.0f);
	// 포탑은 회전해야 하므로 CRotatingObject로 생성합니다.
	CRotatingObject* pTurret = new CRotatingObject();
	pTurret->SetMesh(pTurretMesh);
	pTurret->SetPosition(0.0f, 4.5f, 0.0f); // 몸체 위에 위치하도록 y좌표 조절

	// 포탑 재질 설정 (조금 더 밝은 녹색)
	CMaterial* pTurretMaterial = new CMaterial();
	MATERIALLOADINFO turretMaterialInfo;
	turretMaterialInfo.m_xmf4AlbedoColor = XMFLOAT4(0.25f, 0.45f, 0.25f, 1.0f);
	turretMaterialInfo.m_xmf4SpecularColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 10.0f);
	pTurretMaterial->SetMaterialColors(new CMaterialColors(&turretMaterialInfo));
	pTurretMaterial->SetIlluminatedShader();
	pTurret->SetMaterial(0, pTurretMaterial);

	// 포탑을 몸체의 자식으로 붙입니다.
	pBody->SetChild(pTurret, true);

	// 3. 탱크 포신 (Cannon) 만들기
	CCubeMeshDiffused* pCannonMesh = new CCubeMeshDiffused(pd3dDevice, pd3dCommandList, 1.5f, 1.5f, 12.0f);
	CGameObject* pCannon = new CGameObject(1);
	pCannon->SetMesh(pCannonMesh);
	pCannon->SetPosition(0.0f, 1.0f, 10.0f); // 포탑 앞에 위치하도록 z좌표 조절

	// 포신 재질 설정 (회색)
	CMaterial* pCannonMaterial = new CMaterial();
	MATERIALLOADINFO cannonMaterialInfo;
	cannonMaterialInfo.m_xmf4AlbedoColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	cannonMaterialInfo.m_xmf4SpecularColor = XMFLOAT4(0.5f, 0.5f, 0.5f, 5.0f);
	pCannonMaterial->SetMaterialColors(new CMaterialColors(&cannonMaterialInfo));
	pCannonMaterial->SetIlluminatedShader();
	pCannon->SetMaterial(0, pCannonMaterial);

	// 포신을 포탑의 자식으로 붙입니다.
	pTurret->SetChild(pCannon, true);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CTankPlayer::~CTankPlayer() {}