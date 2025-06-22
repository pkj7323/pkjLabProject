// Bullet.h

#pragma once
#include "GameObject.h"
CMaterial* CreateBulletMaterial();
class CBullet : public CGameObject
{
public:
	// 생성자는 최소한의 초기화만 담당합니다.
	CBullet(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual ~CBullet();

	// 풀에서 총알을 가져와 재사용할 때 호출할 함수입니다.
	void Reset(CGameObject* pOwner, const XMFLOAT3& xmf3Position, const XMFLOAT3& xmf3Direction, const XMFLOAT3& xmf3Up);

	virtual void Animate(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera) override;

	CGameObject* GetOwner() { return m_pOwner; }

protected:
	CGameObject*	m_pOwner = NULL; // 이 총알을 발사한 객체
	XMFLOAT3		m_xmf3FirePosition;
	XMFLOAT3		m_xmf3MovingDirection;
	float			m_fSpeed = 750.0f;
	float			m_fLifeTime = 3.0f; // 3초 후에 사라짐
	float			m_fElapsedTime = 0.0f;
};