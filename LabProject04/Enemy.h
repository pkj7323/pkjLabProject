#pragma once
#include "GameObject.h"
#include "Player.h"

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
	XMFLOAT3 m_xmf3Position;
	XMFLOAT3 m_xmf3Right;
	XMFLOAT3 m_xmf3Up;
	XMFLOAT3 m_xmf3Look;
	float m_fSpeed = 20.0f;
	void* m_pContext = NULL;
	CGameObject* m_pBody = NULL;
	CGameObject* m_pTurret = NULL;
	float m_fFireDelay = 2.0f;
	float m_fFireTimer = 0.0f;
};