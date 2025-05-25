#include "stdafx.h"
#include "GameObject.h"

#include "Shader.h"


CGameObject::CGameObject()
{
	XMStoreFloat4x4(&m_xmf4x4World, XMMatrixIdentity());
}
CGameObject::~CGameObject()
{
	if (m_pMesh) m_pMesh->Release();
	if (m_pShader)
	{
		m_pShader->ReleaseShaderVariables();
		m_pShader->Release();
	}
}

void CGameObject::SetShader(CShader* pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}
void CGameObject::SetMesh(CMesh* pMesh)
{
	if (m_pMesh) m_pMesh->Release();
	m_pMesh = pMesh;
	if (m_pMesh) m_pMesh->AddRef();
}

void CGameObject::ReleaseUploadBuffers()
{
	//정점 버퍼를 위한 업로드 버퍼를 소멸시킨다. 
	if (m_pMesh)
		m_pMesh->ReleaseUploadBuffers();
}

void CGameObject::Animate(float fTimeElapsed)
{}

void CGameObject::OnPrepareRender()
{}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList ,CCamera* pCamera)
{
	OnPrepareRender();
	if (m_pShader)
	{
		//게임 객체의 월드 변환 행렬을 셰이더의 상수 버퍼로 전달(복사)한다.
		m_pShader->UpdateShaderVariable(pd3dCommandList, &m_xmf4x4World);
		m_pShader->Render(pd3dCommandList, pCamera);
	}
	if (m_pMesh) m_pMesh->Render(pd3dCommandList);
}

void CGameObject::Rotate(XMFLOAT3* xmf3RotationAxis, float fAngle)
{
	// 회전축과 각도로 쿼터니언 생성
	XMVECTOR qRot = XMQuaternionRotationAxis(XMLoadFloat3(xmf3RotationAxis), XMConvertToRadians(fAngle));

	// 쿼터니언을 행렬로 변환
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(qRot);

	// 기존 월드 행렬을 불러옴
	XMMATRIX mtxWorld = XMLoadFloat4x4(&m_xmf4x4World);

	// 회전 행렬을 월드 행렬에 곱해서 누적
	mtxWorld = XMMatrixMultiply(mtxRotate, mtxWorld);
	// 결과를 다시 저장
	XMStoreFloat4x4(&m_xmf4x4World, mtxWorld);
}
///////////////////////////////////////
CRotatingObject::CRotatingObject()
{
	m_xmf3RotationAxis = XMFLOAT3(0.0f, 0.0f, 1.0f);
	m_fRotationSpeed = 90.0f;
}
CRotatingObject::~CRotatingObject() {}

void CRotatingObject::Animate(float fTimeElapsed)
{
	CGameObject::Rotate(&m_xmf3RotationAxis, m_fRotationSpeed * fTimeElapsed);
}
