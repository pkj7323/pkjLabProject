// Bullet.cpp

#include "stdafx.h"
#include "Bullet.h"
#include "Mesh.h"

// 총알 재질 생성을 위한 헬퍼 함수
CMaterial* CreateBulletMaterial()
{
	CMaterial* pMaterial = new CMaterial();
	MATERIALLOADINFO materialInfo;
	materialInfo.m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f); // 노란색
	materialInfo.m_xmf4EmissiveColor = XMFLOAT4(0.8f, 0.8f, 0.2f, 1.0f); // 스스로 빛나는 효과
	materialInfo.m_xmf4SpecularColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	pMaterial->SetMaterialColors(new CMaterialColors(&materialInfo));
	pMaterial->SetIlluminatedShader();
	return pMaterial;
}

CBullet::CBullet(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) : CGameObject(1)
{
	// 생성자에서는 아무것도 하지 않습니다.
	// 메쉬와 재질은 Scene에서 오브젝트 풀을 만들 때 한 번만 설정해 줄 것입니다.
}

CBullet::~CBullet()
{}

void CBullet::Reset(CGameObject* pOwner, const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction,
	const XMFLOAT3& xmf3Up)
{
	m_pOwner = pOwner;
	m_xmf3FirePosition = xmf3Position;
	m_xmf3MovingDirection = Vector3::Normalize(xmf3Direction);
	m_fElapsedTime = 0.0f;
	SetActive(true);

	// up 방향으로 4, forward 방향(발사 방향)으로 10만큼 이동
	XMFLOAT3 upOffset = Vector3::ScalarProduct(xmf3Up, 4.0f, false);
	XMFLOAT3 forwardOffset = Vector3::ScalarProduct(m_xmf3MovingDirection, 10.0f, false);
	XMFLOAT3 pos = Vector3::Add(xmf3Position, upOffset);
	pos = Vector3::Add(pos, forwardOffset);
	SetPosition(pos);

	// 총알의 방향을 발사 방향으로 설정
	XMVECTOR xmvLook = XMLoadFloat3(&m_xmf3MovingDirection);
	XMVECTOR xmvUp = XMLoadFloat3(&xmf3Up);
	XMMATRIX mtxRot = XMMatrixLookToLH(XMVectorSet(0, 0, 0, 1), xmvLook, xmvUp);
	m_xmf4x4Transform = Matrix4x4::Multiply(Matrix4x4::Identity(), mtxRot);
}


void CBullet::Animate(float fTimeElapsed)
{
	m_fElapsedTime += fTimeElapsed;

	// 수명이 다하면 비활성화합니다.
	if (m_fElapsedTime > m_fLifeTime)
	{
		SetActive(false);
	}

	// 자신의 이동 방향으로 계속 움직입니다.
	XMFLOAT3 xmf3Position = GetPosition();
	xmf3Position = Vector3::Add(xmf3Position, m_xmf3MovingDirection, m_fSpeed * fTimeElapsed);
	SetPosition(xmf3Position);

	// GameObject의 Animate를 호출하여 자식들을 업데이트합니다 (현재는 자식 없음).
	CGameObject::Animate(fTimeElapsed);
}

void CBullet::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	// 활성화 상태일 때만 렌더링합니다.
	if (IsActive())
	{
		CGameObject::Render(pd3dCommandList, pCamera);
	}
}