// GameObject.h

#pragma once
#include "Camera.h"

// CMesh의 전체 내용을 아는 대신, 이런 클래스가 있다는 것만 알려줍니다. (전방 선언)
class CMesh;
class CMeshLoadInfo;
class CShader;

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 재질(Material) 관련 클래스 선언부
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct MATERIALLOADINFO
{
	XMFLOAT4						m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4						m_xmf4EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	float							m_fGlossiness = 0.0f;
	float							m_fSmoothness = 0.0f;
	float							m_fSpecularHighlight = 0.0f;
	float							m_fMetallic = 0.0f;
	float							m_fGlossyReflection = 0.0f;
	UINT							m_nType = 0x00;
};

struct MATERIALSLOADINFO
{
	int								m_nMaterials = 0;
	MATERIALLOADINFO				*m_pMaterials = NULL;
};

class CMaterialColors
{
public:
	CMaterialColors();
	CMaterialColors(MATERIALLOADINFO *pMaterialInfo);
	virtual ~CMaterialColors();
private:
	int								m_nReferences = 0;
public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	XMFLOAT4						m_xmf4Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	XMFLOAT4						m_xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); //(r,g,b,a=power)
	XMFLOAT4						m_xmf4Emissive = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
};

class CMaterial
{
public:
	CMaterial();
	virtual ~CMaterial();
private:
	int								m_nReferences = 0;
public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	CShader							*m_pShader = NULL;
	CMaterialColors					*m_pMaterialColors = NULL;

	void SetMaterialColors(CMaterialColors *pMaterialColors);
	void SetShader(CShader *pShader);
	void SetIlluminatedShader();

	void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList);
protected:
	static CShader					*m_pIlluminatedShader;
public:
	static void PrepareShaders(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 게임 객체(GameObject) 관련 클래스 선언부
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CGameObject
{
private:
	int								m_nReferences = 0;
public:
	void AddRef();
	void Release();

public:
	CGameObject(int nMeshes = 0);
	virtual ~CGameObject();
public:
	char							m_pstrFrameName[64];
	CMesh							*m_pMesh = NULL;
	int								m_nMaterials = 0;
	CMaterial						**m_ppMaterials = NULL;

	XMFLOAT4X4						m_xmf4x4Transform;
	XMFLOAT4X4						m_xmf4x4World;

	CGameObject 					*m_pParent = NULL;
	CGameObject 					*m_pChild = NULL;
	CGameObject 					*m_pSibling = NULL;

	void SetMesh(CMesh *pMesh);
	void SetShader(CShader *pShader);
	void SetShader(int nMaterial, CShader *pShader);
	void SetMaterial(int nMaterial, CMaterial *pMaterial);

	void SetChild(CGameObject* pChild, bool bReferenceUpdate = false);

	virtual void OnInitialize() {}
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);

	virtual void OnPrepareRender();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World);
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial);

	virtual void ReleaseUploadBuffers();

	virtual XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetScale(float x, float y, float z);

	virtual void Move(const XMFLOAT3& xmf3Shift, bool bVelocity = false);
	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	virtual void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 1.0f);
	void Rotate(XMFLOAT3 *pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4 *pxmf4Quaternion);

	CGameObject *GetParent() { return(m_pParent); }
	void UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent = NULL);
	CGameObject *FindFrame(const char* pstrFrameName);

	UINT GetMeshType();

public:
	static MATERIALSLOADINFO *LoadMaterialsInfoFromFile(std::ifstream& file);
	static CMeshLoadInfo *LoadMeshInfoFromFile(std::ifstream& file);
	static CGameObject *LoadFrameHierarchyFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, std
	                                               ::ifstream& file);
	static CGameObject *LoadGeometryFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, const
	                                         char* pstrFileName);
};

class CRotatingObject : public CGameObject
{
public:
	CRotatingObject();
	virtual ~CRotatingObject();
private:
	XMFLOAT3					m_xmf3RotationAxis;
	float						m_fRotationSpeed;
public:
	void SetRotationSpeed(float fRotationSpeed) { m_fRotationSpeed = fRotationSpeed; }
	void SetRotationAxis(XMFLOAT3 xmf3RotationAxis) { m_xmf3RotationAxis = xmf3RotationAxis; }
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);
};

class CHellicopterObject : public CGameObject
{
public:
	CHellicopterObject();
	virtual ~CHellicopterObject();
protected:
	CGameObject					*m_pMainRotorFrame = NULL;
	CGameObject					*m_pTailRotorFrame = NULL;
public:
	virtual void OnInitialize();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4 *pxmf4x4Parent = NULL);
};

class CApacheObject : public CHellicopterObject
{
public:
	CApacheObject();
	virtual ~CApacheObject();
public:
	virtual void OnInitialize();
};

class CSuperCobraObject : public CHellicopterObject
{
public:
	CSuperCobraObject();
	virtual ~CSuperCobraObject();
public:
	virtual void OnInitialize();
};

class CTankObject : public CGameObject
{
public:
	CTankObject();
	virtual ~CTankObject();
protected:
	CGameObject* m_pTurretFrame = NULL;
	CGameObject* m_pCannonFrame = NULL;
	CGameObject* m_pGunFrame = NULL;
public:
	virtual void OnInitialize();
	virtual void Animate(float fTimeElapsed, XMFLOAT4X4* pxmf4x4Parent = NULL);
};

class CM26Object : public CTankObject
{
public:
	CM26Object();
	virtual ~CM26Object();
	virtual void OnInitialize();
};