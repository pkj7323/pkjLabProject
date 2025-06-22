// Bullet.h

#pragma once
#include "GameObject.h"
class CMaterial;
CMaterial* CreateBulletMaterial();
class CBullet : public CGameObject
{
public:
	CBullet(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CBullet();
	void Reset(CGameObject* pOwner, const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, const XMFLOAT3& xmf3Up);
	virtual void Animate(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;
	CGameObject* GetOwner() { return m_pOwner; }
protected:
	CGameObject* m_pOwner = NULL;
	XMFLOAT3 m_xmf3FirePosition;
	XMFLOAT3 m_xmf3MovingDirection;
	float m_fSpeed = 750.0f;
	float m_fLifeTime = 3.0f;
	float m_fElapsedTime = 0.0f;
};