// Bullet.cpp

#include "stdafx.h"
#include "Bullet.h"
#include "Mesh.h"

CMaterial* CreateBulletMaterial()
{
	CMaterial* pMaterial = new CMaterial();
	MATERIALLOADINFO materialInfo;
	materialInfo.m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	materialInfo.m_xmf4EmissiveColor = XMFLOAT4(0.8f, 0.8f, 0.2f, 1.0f);
	materialInfo.m_xmf4SpecularColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	pMaterial->SetMaterialColors(new CMaterialColors(&materialInfo));
	pMaterial->SetIlluminatedShader();
	return pMaterial;
}

CBullet::CBullet(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) : CGameObject(1)
{
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

	XMFLOAT3 upOffset = Vector3::ScalarProduct(xmf3Up, 4.0f, false);
	XMFLOAT3 forwardOffset = Vector3::ScalarProduct(m_xmf3MovingDirection, 10.0f, false);
	XMFLOAT3 pos = Vector3::Add(xmf3Position, upOffset);
	pos = Vector3::Add(pos, forwardOffset);
	SetPosition(pos);

	XMVECTOR xmvLook = XMLoadFloat3(&m_xmf3MovingDirection);
	XMVECTOR xmvUp = XMLoadFloat3(&xmf3Up);
	XMMATRIX mtxRot = XMMatrixLookToLH(XMVectorSet(0, 0, 0, 1), xmvLook, xmvUp);
	m_xmf4x4Transform = Matrix4x4::Multiply(Matrix4x4::Identity(), mtxRot);
}

void CBullet::Animate(float fTimeElapsed)
{
	m_fElapsedTime += fTimeElapsed;
	if (m_fElapsedTime > m_fLifeTime)
	{
		SetActive(false);
	}
	XMFLOAT3 xmf3Position = GetPosition();
	xmf3Position = Vector3::Add(xmf3Position, m_xmf3MovingDirection, m_fSpeed * fTimeElapsed);
	SetPosition(xmf3Position);
	CGameObject::Animate(fTimeElapsed);
}

void CBullet::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	if (IsActive())
	{
		CGameObject::Render(pd3dCommandList, pCamera);
	}
}