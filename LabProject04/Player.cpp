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

	
	XMStoreFloat4(&m_xmf4Quaternion, XMQuaternionIdentity()); // 회전 없음(기본값)으로 초기화
	m_fRotationSpeed = 10.0f; // 회전 속도 기본값 설정 (값을 키우면 더 빨리 반응)
	
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


	// 누적된 회전값을 이용해 현재 방향 벡터에 "상대적으로" 회전 적용
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(
		XMConvertToRadians(x), // 변화량만큼만 회전
		XMConvertToRadians(y),
		XMConvertToRadians(z)
	);
	m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, mtxRotate);
	m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, mtxRotate);
	m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, mtxRotate);


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
	// 플레이어 위치를 출력창(디버그 출력)에 출력
	OutputDebugStringA(
		(std::string("Player Position: x=") +
		 std::to_string(m_xmf3Position.x) + ", y=" +
		 std::to_string(m_xmf3Position.y) + ", z=" +
		 std::to_string(m_xmf3Position.z) + "\n").c_str()
	);
	//플레이어 회전 출력
	OutputDebugStringA(
		(std::string("Player Rotation: x=") +
		 std::to_string(m_fPitch) + ", y=" +
		 std::to_string(m_fYaw) + ", z=" +
		 std::to_string(m_fRoll) + "\n").c_str()
	);
	//플레이어 회전 벡터 출력
	OutputDebugStringA(
		(std::string("Player Rotation Vector: Right=") +
		 std::to_string(m_xmf3Right.x) + ", " + std::to_string(m_xmf3Right.y) + "," + std::to_string(m_xmf3Right.z) 
		 + ", Up=" + std::to_string(m_xmf3Up.x) + ", " + std::to_string(m_xmf3Up.y) + "," + std::to_string(m_xmf3Up.z)
		 + ", Look=" + std::to_string(m_xmf3Look.x) + ", " + std::to_string(m_xmf3Look.y) + "," + std::to_string(m_xmf3Look.z) + "\n").c_str()
	);
	
	if (m_pCamera)
	{//카메라 회전 출력
		OutputDebugStringA(
			(std::string("Camera Rotation: Pitch=") +
			 std::to_string(m_pCamera->m_fPitch) + ", Yaw=" +
			 std::to_string(m_pCamera->m_fYaw) + ", Roll=" +
			 std::to_string(m_pCamera->m_fRoll) + "\n").c_str()
		);
		OutputDebugStringA(
			(std::string("Camera Vector: Right=(") +
			 std::to_string(m_pCamera->m_xmf3Right.x) + ", " + std::to_string(m_pCamera->m_xmf3Right.y) + ", " + std::to_string(m_pCamera->m_xmf3Right.z) +
			 "), Up=(" + std::to_string(m_pCamera->m_xmf3Up.x) + ", " + std::to_string(m_pCamera->m_xmf3Up.y) + ", " + std::to_string(m_pCamera->m_xmf3Up.z) +
			 "), Look=(" + std::to_string(m_pCamera->m_xmf3Look.x) + ", " + std::to_string(m_pCamera->m_xmf3Look.y) + ", " + std::to_string(m_pCamera->m_xmf3Look.z) + ")\n").c_str()
		);
	}
	if (m_pPlayerUpdatedContext) OnPlayerUpdateCallback(fTimeElapsed);

	fLength = Vector3::Length(m_xmf3Velocity);
	float fDeceleration = (m_fFriction * fTimeElapsed);
	if (fDeceleration > fLength) fDeceleration = fLength;
	m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, Vector3::ScalarProduct(m_xmf3Velocity, -fDeceleration, true));
	DWORD nCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCameraMode == THIRD_PERSON_CAMERA) m_pCamera->Update(m_xmf3Position, fTimeElapsed);
	if (m_pCameraUpdatedContext) OnCameraUpdateCallback(fTimeElapsed);
	if (nCameraMode == THIRD_PERSON_CAMERA) m_pCamera->SetLookAt(m_xmf3Position);
	m_pCamera->RegenerateViewMatrix();
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
// Player.cpp 파일에 추가

void CPlayer::OnPrepareRender()
{
	// 부모인 CGameObject의 OnPrepareRender를 먼저 호출합니다. (현재는 내용이 비어있음)
	CGameObject::OnPrepareRender();

	// 플레이어의 방향 벡터(Right, Up, Look)와 위치 벡터(Position)로
	// m_xmf4x4Transform 행렬을 새로 만듭니다.
	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4Transform._11 = m_xmf3Right.x;    m_xmf4x4Transform._12 = m_xmf3Right.y;    m_xmf4x4Transform._13 = m_xmf3Right.z;
	m_xmf4x4Transform._21 = m_xmf3Up.x;       m_xmf4x4Transform._22 = m_xmf3Up.y;       m_xmf4x4Transform._23 = m_xmf3Up.z;
	m_xmf4x4Transform._31 = m_xmf3Look.x;     m_xmf4x4Transform._32 = m_xmf3Look.y;     m_xmf4x4Transform._33 = m_xmf3Look.z;
	m_xmf4x4Transform._41 = m_xmf3Position.x; m_xmf4x4Transform._42 = m_xmf3Position.y; m_xmf4x4Transform._43 = m_xmf3Position.z;

	UpdateTransform(NULL);
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

	// 1. 지형 높이 보정 (이전과 동일)
	float fHeight = pTerrain->GetHeight(xmf3PlayerPosition.x, xmf3PlayerPosition.z) + 5.0f;
	if (xmf3PlayerPosition.y < fHeight)
	{
		XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
		xmf3PlayerVelocity.y = 0.0f;
		SetVelocity(xmf3PlayerVelocity);
		xmf3PlayerPosition.y = fHeight;
		SetPosition(xmf3PlayerPosition);
	}

	//// --- 지형 경사에 맞춰 부드럽게 회전하는 로직 ---
	//// 2. 목표 Up 벡터 (지형의 법선)와 목표 Look 벡터를 계산합니다.
	//XMFLOAT3 xmf3TargetUp = pTerrain->GetNormal(xmf3PlayerPosition.x, xmf3PlayerPosition.z);
	//XMFLOAT3 xmf3TargetLook = Vector3::Subtract(m_xmf3Look, Vector3::ScalarProduct(xmf3TargetUp, Vector3::DotProduct(m_xmf3Look, xmf3TargetUp)));
	//xmf3TargetLook = Vector3::Normalize(xmf3TargetLook);

	//// 3. 목표 자세에 해당하는 회전 행렬을 만듭니다.
	//XMMATRIX mtxTargetRot;
	//mtxTargetRot.r[0] = XMLoadFloat3(&Vector3::CrossProduct(xmf3TargetUp, xmf3TargetLook, true));
	//mtxTargetRot.r[1] = XMLoadFloat3(&xmf3TargetUp);
	//mtxTargetRot.r[2] = XMLoadFloat3(&xmf3TargetLook);
	//mtxTargetRot.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	//// 4. 목표 회전 행렬을 목표 쿼터니언으로 변환합니다.
	//XMVECTOR xmvTargetQuaternion = XMQuaternionRotationMatrix(mtxTargetRot);

	//// 5. 현재 쿼터니언에서 목표 쿼터니언으로 부드럽게 회전(Slerp)합니다.
	//XMVECTOR xmvCurrentQuaternion = XMLoadFloat4(&m_xmf4Quaternion);
	//XMVECTOR xmvNewQuaternion = XMQuaternionSlerp(xmvCurrentQuaternion, xmvTargetQuaternion, m_fRotationSpeed * fTimeElapsed);

	//// 6. 새로 계산된 쿼터니언을 다음 프레임을 위해 저장합니다.
	//XMStoreFloat4(&m_xmf4Quaternion, xmvNewQuaternion);

	//// 7. 최종 쿼터니언으로 회전 행렬을 만들어 플레이어의 방향 벡터들을 업데이트합니다.
	//XMMATRIX mtxRot = XMMatrixRotationQuaternion(xmvNewQuaternion);
	//m_xmf3Right = Vector3::TransformNormal(XMFLOAT3(1, 0, 0), mtxRot);
	//m_xmf3Up = Vector3::TransformNormal(XMFLOAT3(0, 1, 0), mtxRot);
	//m_xmf3Look = Vector3::TransformNormal(XMFLOAT3(0, 0, 1), mtxRot);
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

//////////////////////////////////////////
///

CMaterial* CreateTankMaterial(XMFLOAT4 xmf4Color)
{
	CMaterial* pMaterial = new CMaterial();
	MATERIALLOADINFO materialInfo;
	materialInfo.m_xmf4AlbedoColor = xmf4Color;
	materialInfo.m_xmf4SpecularColor = XMFLOAT4(0.3f, 0.3f, 0.3f, 10.0f);
	pMaterial->SetMaterialColors(new CMaterialColors(&materialInfo));
	pMaterial->SetIlluminatedShader();
	return pMaterial;
}

CTankPlayer::CTankPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext)
	: CTerrainPlayer(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pContext)
{
	// 카메라, 플레이어 위치 등 기본 설정
	m_pCamera = CTerrainPlayer::ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
	CHeightMapTerrain* pTerrain = (CHeightMapTerrain*)pContext;
	float fHeight = pTerrain->GetHeight(pTerrain->GetWidth() * 0.5f, pTerrain->GetLength() * 0.5f) + 5.0f;
	SetPosition(XMFLOAT3(pTerrain->GetWidth() * 0.5f, fHeight, pTerrain->GetLength() * 0.5f));
	SetPlayerUpdatedContext(pContext);
	SetCameraUpdatedContext(pContext);

	// --- 탱크를 OBJ 파일들로 직접 조립하는 부분 ---

	CGameObject* pBody = new CGameObject(1);
	pBody->SetMesh(new CObjMesh(pd3dDevice, pd3dCommandList, "../Assets/M26 Body.obj"));
	pBody->SetMaterial(0, CreateTankMaterial(XMFLOAT4(0.2f, 0.4f, 0.2f, 1.0f))); // 짙은 녹색
	SetChild(pBody, true); // 플레이어의 자식으로 몸체를 붙임
	pBody->Rotate(0, 180, 0);

	m_pTurret = new CGameObject(1);
	m_pTurret->SetMesh(new CObjMesh(pd3dDevice, pd3dCommandList, "../Assets/M26 Turret.obj"));
	m_pTurret->SetMaterial(0, CreateTankMaterial(XMFLOAT4(0.25f, 0.45f, 0.25f, 1.0f))); // 조금 밝은 녹색
	m_pTurret->SetPosition(0.0f, 0.f, 0.0f); // 몸체 위의 위치
	pBody->SetChild(m_pTurret, true); // 포탑을 몸체의 자식으로 붙임

	//// 2. 테스트용 큐브를 위한 게임 오브젝트를 생성합니다.
	//CGameObject* pCubeObject = new CGameObject(1);

	//// 3. 우리가 렌더링에 성공했던 CCubeMeshDiffused 메쉬를 생성하여 설정합니다.
	//pCubeObject->SetMesh(new CCubeMeshDiffused(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 20.0f));

	//// 4. 큐브에 기본 재질(흰색)을 입힙니다.
	//pCubeObject->SetMaterial(0, CreateTankMaterial(XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)));

	//// 5. 이 큐브 객체를 플레이어의 자식으로 설정합니다.
	//SetChild(pCubeObject, true);

	// --- 테스트 끝 ---
	SetScale(30.0f, 30.0f, 30.0f);

	CPlayer::CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CTankPlayer::~CTankPlayer() {}
void CTankPlayer::RotateTurret(float fYaw)
{
	// m_pTurret 포인터가 유효할 때만 실행
	if (m_pTurret)
	{
		// 포탑의 로컬 Y축을 기준으로 회전 행렬을 생성합니다.
		XMMATRIX mtxRotate = XMMatrixRotationY(XMConvertToRadians(fYaw));
		// 기존 포탑의 로컬 변환 행렬에 새로운 회전 행렬을 곱합니다.
		m_pTurret->m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_pTurret->m_xmf4x4Transform);
	}
}
