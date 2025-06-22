// GameObject.cpp

#include "stdafx.h"
#include "GameObject.h"
#include "Shader.h"
#include "Mesh.h" // .cpp 파일에서는 Mesh.h의 전체 내용이 필요합니다.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CMaterialColors, CMaterial 클래스 구현부
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CMaterialColors::CMaterialColors()
{}

CMaterialColors::CMaterialColors(MATERIALLOADINFO *pMaterialInfo)
{
	m_xmf4Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_xmf4Diffuse = pMaterialInfo->m_xmf4AlbedoColor;
	m_xmf4Specular = pMaterialInfo->m_xmf4SpecularColor; //(r,g,b,a=power)
	m_xmf4Specular.w = pMaterialInfo->m_fGlossiness;
	m_xmf4Emissive = pMaterialInfo->m_xmf4EmissiveColor;
}

CMaterialColors::~CMaterialColors()
{}

CShader *CMaterial::m_pIlluminatedShader = NULL;

CMaterial::CMaterial()
{
	m_pShader = NULL;
	m_pMaterialColors = NULL;
}

CMaterial::~CMaterial()
{
	if (m_pShader) m_pShader->Release();
	if (m_pMaterialColors) m_pMaterialColors->Release();
}

void CMaterial::SetShader(CShader *pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}

void CMaterial::SetMaterialColors(CMaterialColors *pMaterialColors)
{
	if (m_pMaterialColors) m_pMaterialColors->Release();
	m_pMaterialColors = pMaterialColors;
	if (m_pMaterialColors) m_pMaterialColors->AddRef();
}

void CMaterial::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList)
{
	// 루트 파라미터 번호를 [2]에서 [1]로 수정합니다. (오프셋은 16부터 시작)
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &(m_pMaterialColors->m_xmf4Ambient), 16);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &(m_pMaterialColors->m_xmf4Diffuse), 20);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &(m_pMaterialColors->m_xmf4Specular), 24);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &(m_pMaterialColors->m_xmf4Emissive), 28);
}

void CMaterial::PrepareShaders(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature)
{
	m_pIlluminatedShader = new CIlluminatedShader();
	m_pIlluminatedShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
}

void CMaterial::SetIlluminatedShader()
{
	SetShader(m_pIlluminatedShader);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CGameObject 클래스 구현부
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CGameObject::CGameObject(int nMeshes)
{
	m_pMesh = NULL;
	m_nMaterials = 0;
	m_ppMaterials = NULL;

	m_xmf4x4Transform = Matrix4x4::Identity();
	m_xmf4x4World = Matrix4x4::Identity();

	m_pParent = NULL;
	m_pChild = NULL;
	m_pSibling = NULL;

	m_nReferences = 0;
}

CGameObject::~CGameObject()
{
	if (m_pMesh) m_pMesh->Release();

	if (m_nMaterials > 0)
	{
		for (int i = 0; i < m_nMaterials; i++)
		{
			if (m_ppMaterials[i]) m_ppMaterials[i]->Release();
		}
	}
	if (m_ppMaterials) delete[] m_ppMaterials;
}

void CGameObject::AddRef()
{
	m_nReferences++;
	if (m_pSibling) m_pSibling->AddRef();
	if (m_pChild) m_pChild->AddRef();
}

void CGameObject::Release()
{
	if (m_pChild) m_pChild->Release();
	if (m_pSibling) m_pSibling->Release();
	if (--m_nReferences <= 0) delete this;
}

void CGameObject::SetChild(CGameObject* pChild, bool bReferenceUpdate)
{
	if (pChild)
	{
		pChild->m_pParent = this;
		if (bReferenceUpdate) pChild->AddRef();
	}
	if (m_pChild)
	{
		if (pChild) pChild->m_pSibling = m_pChild->m_pSibling;
		m_pChild->m_pSibling = pChild;
	}
	else
	{
		m_pChild = pChild;
	}
}



void CGameObject::SetMesh(CMesh* pMesh)
{
	if (m_pMesh) m_pMesh->Release();
	m_pMesh = pMesh;
	if (m_pMesh) m_pMesh->AddRef();
}

void CGameObject::SetShader(CShader *pShader)
{
	if (!m_ppMaterials)
	{
		m_nMaterials = 1;
		m_ppMaterials = new CMaterial * [m_nMaterials];
		m_ppMaterials[0] = new CMaterial();
	}
	m_ppMaterials[0]->SetShader(pShader);
}

void CGameObject::SetShader(int nMaterial, CShader *pShader)
{
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->SetShader(pShader);
}

void CGameObject::SetMaterial(int nMaterial, CMaterial *pMaterial)
{
	if (!m_ppMaterials)
	{
		m_nMaterials = (nMaterial < 1) ? 1 : nMaterial + 1;
		m_ppMaterials = new CMaterial * [m_nMaterials];
		for (int i = 0; i < m_nMaterials; ++i)
			m_ppMaterials[i] = nullptr;
	}

	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->Release();
	m_ppMaterials[nMaterial] = pMaterial;
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->AddRef();
}

void CGameObject::UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent)
{
	m_xmf4x4World = (pxmf4x4Parent) ? Matrix4x4::Multiply(m_xmf4x4Transform, *pxmf4x4Parent) : m_xmf4x4Transform;
	if (m_pMesh)
	{
		m_pMesh->m_OOBB.Transform(m_OOBB/*out*/, XMLoadFloat4x4(&m_xmf4x4World));
		XMStoreFloat4(&m_OOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_OOBB.Orientation)));
	}
	if (m_pSibling) m_pSibling->UpdateTransform(pxmf4x4Parent);
	if (m_pChild) m_pChild->UpdateTransform(&m_xmf4x4World);
}

void CGameObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (!m_bActive) return;
	UpdateTransform(pxmf4x4Parent);

	if (m_pSibling) m_pSibling->Animate(fTimeElapsed, pxmf4x4Parent);
	if (m_pChild) m_pChild->Animate(fTimeElapsed, &m_xmf4x4World);
}

void CGameObject::OnPrepareRender() {}

void CGameObject::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if (!m_bActive) return;
	OnPrepareRender();
	UpdateShaderVariable(pd3dCommandList, &m_xmf4x4World);

	if (m_nMaterials > 0)
	{
		for (int i = 0; i < m_nMaterials; i++)
		{
			if (m_ppMaterials[i])
			{
				if (m_ppMaterials[i]->m_pShader) m_ppMaterials[i]->m_pShader->Render(pd3dCommandList, pCamera);
				m_ppMaterials[i]->UpdateShaderVariable(pd3dCommandList);
			}
			if (m_pMesh) m_pMesh->Render(pd3dCommandList, i);
		}
	}
	if (m_pSibling) m_pSibling->Render(pd3dCommandList, pCamera);
	if (m_pChild) m_pChild->Render(pd3dCommandList, pCamera);
}

void CGameObject::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) {}
void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList) {}
void CGameObject::ReleaseShaderVariables() {}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World)
{
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(pxmf4x4World)));

	// 루트 파라미터 번호를 [2]에서 [1]로 수정합니다.
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &xmf4x4World, 0);
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial) {}
void CGameObject::UpdateBoundingBox()
{
	if (m_pMesh)
	{
		m_pMesh->m_OOBB.Transform(m_OOBB/*out*/, XMLoadFloat4x4(&m_xmf4x4World));
		XMStoreFloat4(&m_OOBB.Orientation, XMQuaternionNormalize(XMLoadFloat4(&m_OOBB.Orientation)));
	}
}

void CGameObject::ReleaseUploadBuffers()
{
	if (m_pMesh) m_pMesh->ReleaseUploadBuffers();
	if (m_pSibling) m_pSibling->ReleaseUploadBuffers();
	if (m_pChild) m_pChild->ReleaseUploadBuffers();
}

XMFLOAT3 CGameObject::GetPosition() { return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43)); }
XMFLOAT3 CGameObject::GetLook() { return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33))); }
XMFLOAT3 CGameObject::GetUp() { return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23))); }
XMFLOAT3 CGameObject::GetRight() { return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13))); }

void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4Transform._41 = x;
	m_xmf4x4Transform._42 = y;
	m_xmf4x4Transform._43 = z;
	UpdateTransform(m_pParent ? &m_pParent->m_xmf4x4World : NULL);
}
void CGameObject::SetPosition(XMFLOAT3 xmf3Position) { SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z); }

void CGameObject::Move(const XMFLOAT3& xmf3Shift, bool bVelocity)
{
	SetPosition(Vector3::Add(GetPosition(), xmf3Shift));
}

void CGameObject::SetScale(float x, float y, float z)
{
	XMMATRIX mtxScale = XMMatrixScaling(x, y, z);
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxScale, m_xmf4x4Transform);
	UpdateTransform(m_pParent ? &m_pParent->m_xmf4x4World : NULL);
}

void CGameObject::MoveStrafe(float fDistance) { Move(GetRight(), fDistance); }
void CGameObject::MoveUp(float fDistance) { Move(GetUp(), fDistance); }
void CGameObject::MoveForward(float fDistance) { Move(GetLook(), fDistance); }

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
	UpdateTransform(m_pParent ? &m_pParent->m_xmf4x4World : NULL);
}
void CGameObject::Rotate(XMFLOAT3* pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
	UpdateTransform(m_pParent ? &m_pParent->m_xmf4x4World : NULL);
}
void CGameObject::Rotate(XMFLOAT4* pxmf4Quaternion)
{
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(pxmf4Quaternion));
	m_xmf4x4Transform = Matrix4x4::Multiply(mtxRotate, m_xmf4x4Transform);
	UpdateTransform(m_pParent ? &m_pParent->m_xmf4x4World : NULL);
}

UINT CGameObject::GetMeshType() { return((m_pMesh) ? m_pMesh->GetType() : 0); }
// GameObject.cpp

bool CGameObject::CheckCollision(CGameObject *pOther)
{
	if (!pOther) return false;
	UpdateBoundingBox();
	pOther->UpdateBoundingBox();
	// --- 디버깅 코드 추가 ---
	// 충돌을 검사하기 직전에, 두 객체의 바운딩 박스 월드 좌표를 출력합니다.
	if (m_pMesh && pOther->m_pMesh) // 두 객체 모두 메쉬가 있을 때만 의미가 있습니다.
	{
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("충돌 검사: [ %hs (C:%.2f,%.2f,%.2f E:%.2f,%.2f,%.2f) ] vs [ %hs (C:%.2f,%.2f,%.2f E:%.2f,%.2f,%.2f) ]\n"),
					this->m_pstrFrameName,
					this->m_OOBB.Center.x, this->m_OOBB.Center.y, this->m_OOBB.Center.z,
					this->m_OOBB.Extents.x, this->m_OOBB.Extents.y, this->m_OOBB.Extents.z,
					pOther->m_pstrFrameName,
					pOther->m_OOBB.Center.x, pOther->m_OOBB.Center.y, pOther->m_OOBB.Center.z,
					pOther->m_OOBB.Extents.x, pOther->m_OOBB.Extents.y, pOther->m_OOBB.Extents.z
		);
		OutputDebugString(pstrDebug);
	}
	// --- 여기까지 ---

	// 1. 나 자신의 바운딩 박스와 상대방의 바운딩 박스가 충돌하는지 검사합니다.
	if (m_pMesh && m_OOBB.Intersects(pOther->m_OOBB)) return true;

	// 2. 나의 자식 객체들이 상대방과 충돌하는지 재귀적으로 검사합니다.
	if (m_pChild)
	{
		if (m_pChild->CheckCollision(pOther)) return true;
	}

	// 3. 나의 형제 객체들이 상대방과 충돌하는지 재귀적으로 검사합니다.
	if (m_pSibling)
	{
		if (m_pSibling->CheckCollision(pOther)) return true;
	}

	return false;
}

CGameObject *CGameObject::FindFrame(const char* pstrFrameName)
{
	CGameObject *pFrameObject = NULL;
	if (!strncmp(m_pstrFrameName, pstrFrameName, strlen(pstrFrameName))) return(this);

	if (m_pSibling) if (pFrameObject = m_pSibling->FindFrame(pstrFrameName)) return(pFrameObject);
	if (m_pChild) if (pFrameObject = m_pChild->FindFrame(pstrFrameName)) return(pFrameObject);

	return(NULL);
}

int ReadIntegerFromFile(std::ifstream& inFile)
{
	int nValue = 0;
	inFile.read(reinterpret_cast<char*>(&nValue), sizeof(int));
	return nValue;
}

float ReadFloatFromFile(std::ifstream& inFile)
{
	float fValue = 0.0f;
	inFile.read(reinterpret_cast<char*>(&fValue), sizeof(float));
	return fValue;
}

std::string ReadStringFromFile(std::ifstream& file)
{
	BYTE nStrLength = 0;
	file.read(reinterpret_cast<char*>(&nStrLength), sizeof(BYTE));

	// 파일 끝에 도달했거나 오류가 있으면 빈 문자열 반환
	if (file.eof() || file.fail()) return "";

	std::string str(nStrLength, '\0');
	file.read(&str[0], nStrLength);
	return str;
}


CMeshLoadInfo *CGameObject::LoadMeshInfoFromFile(std::ifstream& file)
{
	CMeshLoadInfo *pMeshInfo = new CMeshLoadInfo;
	pMeshInfo->m_nVertices = ReadIntegerFromFile(file);
	std::string meshName = ReadStringFromFile(file);
	strcpy_s(pMeshInfo->m_pstrMeshName, meshName.c_str());

	for (;;)
	{
		std::string token = ReadStringFromFile(file);
		if (token.empty() || token == "</Mesh>") break;

		if (token == "<Positions>:")
		{
			int nPositions = ReadIntegerFromFile(file);
			if (nPositions > 0)
			{
				pMeshInfo->m_nType |= VERTEXT_POSITION;
				pMeshInfo->m_pxmf3Positions = new XMFLOAT3[nPositions];
				file.read(reinterpret_cast<char*>(pMeshInfo->m_pxmf3Positions), sizeof(XMFLOAT3) * nPositions);
			}
		}
		else if (token == "<Normals>:")
		{
			int nNormals = ReadIntegerFromFile(file);
			if (nNormals > 0)
			{
				pMeshInfo->m_nType |= VERTEXT_NORMAL;
				pMeshInfo->m_pxmf3Normals = new XMFLOAT3[nNormals];
				file.read(reinterpret_cast<char*>(pMeshInfo->m_pxmf3Normals), sizeof(XMFLOAT3) * nNormals);
			}
		}
		else if (token == "<SubMeshes>:")
		{
			pMeshInfo->m_nSubMeshes = ReadIntegerFromFile(file);
			if (pMeshInfo->m_nSubMeshes > 0)
			{
				pMeshInfo->m_pnSubSetIndices = new int[pMeshInfo->m_nSubMeshes];
				pMeshInfo->m_ppnSubSetIndices = new UINT * [pMeshInfo->m_nSubMeshes];
				for (int i = 0; i < pMeshInfo->m_nSubMeshes; i++)
				{
					std::string subMeshToken = ReadStringFromFile(file);
					if (subMeshToken == "<SubMesh>:")
					{
						int nIndex = ReadIntegerFromFile(file);
						pMeshInfo->m_pnSubSetIndices[i] = ReadIntegerFromFile(file);
						if (pMeshInfo->m_pnSubSetIndices[i] > 0)
						{
							pMeshInfo->m_ppnSubSetIndices[i] = new UINT[pMeshInfo->m_pnSubSetIndices[i]];
							file.read(reinterpret_cast<char*>(pMeshInfo->m_ppnSubSetIndices[i]), sizeof(UINT) * pMeshInfo->m_pnSubSetIndices[i]);
						}
					}
				}
			}
		}
	}
	return(pMeshInfo);
}

MATERIALSLOADINFO *CGameObject::LoadMaterialsInfoFromFile(std::ifstream& file)
{
	MATERIALSLOADINFO *pMaterialsInfo = new MATERIALSLOADINFO;
	pMaterialsInfo->m_nMaterials = ReadIntegerFromFile(file);
	pMaterialsInfo->m_pMaterials = new MATERIALLOADINFO[pMaterialsInfo->m_nMaterials];

	for (;;)
	{
		std::string token = ReadStringFromFile(file);
		if (token.empty() || token == "</Materials>") break;

		if (token == "<Material>:")
		{
			int nMaterial = ReadIntegerFromFile(file);
			for (;;)
			{
				std::string matToken = ReadStringFromFile(file);
				if (matToken.empty() || matToken == "</Material>") break;

				if (matToken == "<AlbedoColor>:") file.read(reinterpret_cast<char*>(&pMaterialsInfo->m_pMaterials[nMaterial].m_xmf4AlbedoColor), sizeof(float) * 4);
				else if (matToken == "<EmissiveColor>:") file.read(reinterpret_cast<char*>(&pMaterialsInfo->m_pMaterials[nMaterial].m_xmf4EmissiveColor), sizeof(float) * 4);
				else if (matToken == "<SpecularColor>:") file.read(reinterpret_cast<char*>(&pMaterialsInfo->m_pMaterials[nMaterial].m_xmf4SpecularColor), sizeof(float) * 4);
				else if (matToken == "<Glossiness>:") file.read(reinterpret_cast<char*>(&pMaterialsInfo->m_pMaterials[nMaterial].m_fGlossiness), sizeof(float));
			}
		}
	}
	return(pMaterialsInfo);
}

CGameObject *CGameObject::LoadFrameHierarchyFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, std::ifstream& file)
{
	CGameObject* pGameObject = NULL;

	std::string token = ReadStringFromFile(file);
	if (token == "<Frame>:")
	{
		pGameObject = new CGameObject();
		int nFrame = ReadIntegerFromFile(file);
		std::string frameName = ReadStringFromFile(file);
		strcpy_s(pGameObject->m_pstrFrameName, frameName.c_str());

		for (;;)
		{
			token = ReadStringFromFile(file);
			if (token.empty() || token == "</Frame>") break;

			if (token == "<TransformMatrix>:")
			{
				file.read(reinterpret_cast<char*>(&pGameObject->m_xmf4x4Transform), sizeof(float) * 16);
			}
			else if (token == "<Mesh>:")
			{
				CMeshLoadInfo* pMeshInfo = CGameObject::LoadMeshInfoFromFile(file);
				if (pMeshInfo)
				{
					CMesh* pMesh = new CMeshIlluminatedFromFile(pd3dDevice, pd3dCommandList, pMeshInfo);
					pGameObject->SetMesh(pMesh);
					delete pMeshInfo;
				}
			}
			else if (token == "<Materials>:")
			{
				MATERIALSLOADINFO *pMaterialsInfo = CGameObject::LoadMaterialsInfoFromFile(file);
				if (pMaterialsInfo)
				{
					pGameObject->m_nMaterials = pMaterialsInfo->m_nMaterials;
					pGameObject->m_ppMaterials = new CMaterial * [pMaterialsInfo->m_nMaterials];
					for (int i = 0; i < pGameObject->m_nMaterials; ++i) pGameObject->m_ppMaterials[i] = nullptr;

					for (int i = 0; i < pMaterialsInfo->m_nMaterials; i++)
					{
						CMaterial *pMaterial = new CMaterial();
						pMaterial->SetMaterialColors(new CMaterialColors(&pMaterialsInfo->m_pMaterials[i]));
						pMaterial->SetIlluminatedShader();
						pGameObject->SetMaterial(i, pMaterial);
					}
					delete[] pMaterialsInfo->m_pMaterials;
					delete pMaterialsInfo;
				}
			}
			else if (token == "<Children>:")
			{
				int nChilds = ReadIntegerFromFile(file);
				if (nChilds > 0)
				{
					for (int i = 0; i < nChilds; i++)
					{
						CGameObject *pChild = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, file);
						if (pChild) pGameObject->SetChild(pChild);
					}
				}
			}
		}
	}
	return(pGameObject);
}

CGameObject *CGameObject::LoadGeometryFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, 
											   ID3D12RootSignature *pd3dGraphicsRootSignature, const char *pstrFileName)
{
	// 1. C++ 스타일로 파일 열기 (바이너리 모드)
	std::ifstream file(pstrFileName, std::ios::binary);

	// 2. 파일 열기 성공 여부 확인
	if (!file)
	{
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("모델 파일을 찾을 수 없습니다: %hs\n"), pstrFileName);
		OutputDebugString(pstrDebug);
		return(NULL);
	}

	CGameObject *pGameObject = NULL;
	std::string token = ReadStringFromFile(file);
	if (token == "<Hierarchy>:")
	{
		pGameObject = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, file);
	}

	// 3. fclose가 필요 없습니다! file 객체가 이 함수를 벗어날 때 자동으로 닫힙니다.
	return(pGameObject);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// 특수 목적용 게임 객체 클래스 구현부
////////////////////////////////////////////////////////////////////////////////////////////////////

CRotatingObject::CRotatingObject()
{
	m_xmf3RotationAxis = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_fRotationSpeed = 15.0f;
}
CRotatingObject::~CRotatingObject() {}

void CRotatingObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent)
{
	Rotate(&m_xmf3RotationAxis, m_fRotationSpeed * fTimeElapsed);
	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}

CHellicopterObject::CHellicopterObject()
{
	m_pMainRotorFrame = NULL;
	m_pTailRotorFrame = NULL;
}
CHellicopterObject::~CHellicopterObject() {}
void CHellicopterObject::OnInitialize() {}

void CHellicopterObject::Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent)
{
	if (m_pMainRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(360.0f * 2.0f) * fTimeElapsed);
		m_pMainRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pMainRotorFrame->m_xmf4x4Transform);
	}
	if (m_pTailRotorFrame)
	{
		XMMATRIX xmmtxRotate = XMMatrixRotationX(XMConvertToRadians(360.0f * 4.0f) * fTimeElapsed);
		m_pTailRotorFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTailRotorFrame->m_xmf4x4Transform);
	}
	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}


CApacheObject::CApacheObject() {}
CApacheObject::~CApacheObject() {}
void CApacheObject::OnInitialize()
{
	m_pMainRotorFrame = FindFrame("rotor");
	m_pTailRotorFrame = FindFrame("black_m_7");
}

CSuperCobraObject::CSuperCobraObject() {}
CSuperCobraObject::~CSuperCobraObject() {}
void CSuperCobraObject::OnInitialize()
{
	m_pMainRotorFrame = FindFrame("MainRotor_LOD0");
	m_pTailRotorFrame = FindFrame("TailRotor_LOD0");
}


CTankObject::CTankObject()
{
	m_pTurretFrame = NULL;
	m_pCannonFrame = NULL;
	m_pGunFrame = NULL;
}
CTankObject::~CTankObject() {}
void CTankObject::OnInitialize() {}

void CTankObject::Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent)
{
	if (m_pTurretFrame)
	{
		// 이 부분은 나중에 플레이어 입력에 따라 회전하도록 수정할 수 있습니다.
		// XMMATRIX xmmtxRotate = XMMatrixRotationY(XMConvertToRadians(30.0f) * fTimeElapsed);
		// m_pTurretFrame->m_xmf4x4Transform = Matrix4x4::Multiply(xmmtxRotate, m_pTurretFrame->m_xmf4x4Transform);
	}
	CGameObject::Animate(fTimeElapsed, pxmf4x4Parent);
}

CM26Object::CM26Object() {}
CM26Object::~CM26Object() {}
void CM26Object::OnInitialize()
{
	m_pTurretFrame = FindFrame("TURRET");
	m_pCannonFrame = FindFrame("cannon");
	m_pGunFrame = FindFrame("gun");
}