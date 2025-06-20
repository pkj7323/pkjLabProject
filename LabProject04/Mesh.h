// Mesh.h

#pragma once
#include "GameObject.h" // CHeightMapTerrain이 CGameObject를 상속받으므로 포함

/////////////////////////////////////////////////////////////////////////////////////////////////
// 정점 구조체 선언
/////////////////////////////////////////////////////////////////////////////////////////////////

struct CIlluminatedVertex
{
	XMFLOAT3 m_xmf3Position;
	XMFLOAT3 m_xmf3Normal;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// 기본 메쉬 클래스 선언
/////////////////////////////////////////////////////////////////////////////////////////////////

class CMesh
{
public:
	CMesh(int nMeshes = 1);
	virtual ~CMesh();

private:
	int m_nReferences = 0;
public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

protected:
	D3D12_PRIMITIVE_TOPOLOGY m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	UINT m_nSlot = 0;
	UINT m_nVertices = 0;
	UINT m_nOffset = 0;
	UINT m_nStride = 0;

	ID3D12Resource* m_pd3dVertexBuffer = NULL;
	ID3D12Resource* m_pd3dVertexUploadBuffer = NULL;
	D3D12_VERTEX_BUFFER_VIEW m_d3dVertexBufferView;

	ID3D12Resource* m_pd3dIndexBuffer = NULL;
	ID3D12Resource* m_pd3dIndexUploadBuffer = NULL;
	D3D12_INDEX_BUFFER_VIEW m_d3dIndexBufferView;
	UINT m_nIndices = 0;

	UINT m_nType = 0;
public:
	UINT GetType() { return(m_nType); }
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet);
};

/////////////////////////////////////////////////////////////////////////////////////////////////
// 파일 로딩 관련 클래스 선언
/////////////////////////////////////////////////////////////////////////////////////////////////

#define VERTEXT_POSITION			0x01
#define VERTEXT_COLOR				0x02
#define VERTEXT_NORMAL				0x04

class CMeshLoadInfo
{
public:
	CMeshLoadInfo() {}
	~CMeshLoadInfo();
public:
	char m_pstrMeshName[256] = { 0 };
	UINT m_nType = 0x00;
	int m_nVertices = 0;
	XMFLOAT3* m_pxmf3Positions = NULL;
	XMFLOAT3* m_pxmf3Normals = NULL;
	int m_nIndices = 0;
	UINT* m_pnIndices = NULL;
	int m_nSubMeshes = 0;
	int* m_pnSubSetIndices = NULL;
	UINT** m_ppnSubSetIndices = NULL;
};

class CMeshFromFile : public CMesh
{
public:
	CMeshFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CMeshLoadInfo *pMeshInfo);
	virtual ~CMeshFromFile();
protected:
	int m_nSubMeshes = 0;
	int* m_pnSubSetIndices = NULL;
	ID3D12Resource** m_ppd3dSubSetIndexBuffers = NULL;
	ID3D12Resource** m_ppd3dSubSetIndexUploadBuffers = NULL;
	D3D12_INDEX_BUFFER_VIEW* m_pd3dSubSetIndexBufferViews = NULL;
public:
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet);
};

class CMeshIlluminatedFromFile : public CMeshFromFile
{
public:
	CMeshIlluminatedFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CMeshLoadInfo *pMeshInfo);
	CMeshIlluminatedFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nWidth, int nLength, XMFLOAT3 xmf3Scale, void *pContext);
	virtual ~CMeshIlluminatedFromFile();
protected:
	ID3D12Resource* m_pd3dNormalBuffer = NULL;
	ID3D12Resource* m_pd3dNormalUploadBuffer = NULL;
	D3D12_VERTEX_BUFFER_VIEW m_d3dNormalBufferView;
public:
	virtual void ReleaseUploadBuffers();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet);
};

////////////////////////////////////////////////////////////////////////////////////////////////
// 지형 관련 클래스 선언부 (GameObject.h에서 이곳으로 이동)
////////////////////////////////////////////////////////////////////////////////////////////////

class CHeightMapImage
{
private:
	BYTE*						m_pHeightMapPixels;
	int							m_nWidth;
	int							m_nLength;
	XMFLOAT3					m_xmf3Scale;
public:
	CHeightMapImage(LPCTSTR pFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale);
	~CHeightMapImage();
	float GetHeight(float fx, float fz);
	XMFLOAT3 GetHeightMapNormal(int x, int z);
	int GetHeightMapWidth() { return(m_nWidth); }
	int GetHeightMapLength() { return(m_nLength); }
};

class CHeightMapTerrain : public CGameObject
{
private:
	CHeightMapImage*			m_pHeightMapImage;
	XMFLOAT3					m_xmf3Scale;
	int							m_nWidth;
	int							m_nLength;
public:
	CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale);
	virtual ~CHeightMapTerrain();
	float GetHeight(float x, float z) { return(m_pHeightMapImage->GetHeight(x / m_xmf3Scale.x, z / m_xmf3Scale.z) * m_xmf3Scale.y); }
	XMFLOAT3 GetNormal(float x, float z) { return(m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x), int(z / m_xmf3Scale.z))); }
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }
};