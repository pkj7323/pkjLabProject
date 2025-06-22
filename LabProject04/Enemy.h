#pragma once

#include "GameObject.h"
#include "Player.h" // CHeightMapTerrain을 사용하기 위해 포함

class CEnemyTank : public CGameObject
{
public:
	CEnemyTank(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext, const XMFLOAT3& xmf3Position);

	virtual ~CEnemyTank();

	virtual void Animate(float fTimeElapsed, CScene* pScene);
	virtual void OnPrepareRender();

	CGameObject* GetBody() const { return m_pBody; }
	CGameObject* GetTurret() const { return m_pTurret; }
protected:
	// 적 탱크의 상태
	XMFLOAT3 m_xmf3Position;
	XMFLOAT3 m_xmf3Right;
	XMFLOAT3 m_xmf3Up;
	XMFLOAT3 m_xmf3Look;

	float m_fSpeed = 20.0f; // 이동 속도
	void* m_pContext = NULL; // 지형 정보를 담을 포인터

	// 탱크 부품
	CGameObject* m_pBody = NULL;
	CGameObject* m_pTurret = NULL;

	float m_fFireDelay = 2.0f; // 2초마다 발사
	float m_fFireTimer = 0.0f;
};