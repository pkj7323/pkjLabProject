// Mesh.cpp (최종 수정본)

#include "stdafx.h"
#include "Mesh.h"
#include "GameObject.h" // CHeightMapTerrain이 CGameObject와 CMaterial을 사용하므로 포함

/////////////////////////////////////////////////////////////////////////////////////////////////
// CMesh 구현부
/////////////////////////////////////////////////////////////////////////////////////////////////

CMesh::CMesh(int nMeshes)
{
	m_nStride = 0;
	m_nOffset = 0;
	m_pd3dVertexBuffer = NULL;
	m_pd3dVertexUploadBuffer = NULL;

	m_pd3dIndexBuffer = NULL;
	m_pd3dIndexUploadBuffer = NULL;
	m_nIndices = 0;

	m_nReferences = 0;
}

CMesh::~CMesh()
{
	if (m_pd3dVertexBuffer) m_pd3dVertexBuffer->Release();
	if (m_pd3dIndexBuffer) m_pd3dIndexBuffer->Release();
}

void CMesh::ReleaseUploadBuffers()
{
	if (m_pd3dVertexUploadBuffer) m_pd3dVertexUploadBuffer->Release();
	if (m_pd3dIndexUploadBuffer) m_pd3dIndexUploadBuffer->Release();
	m_pd3dVertexUploadBuffer = NULL;
	m_pd3dIndexUploadBuffer = NULL;
}

void CMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dVertexBufferView);
	if (m_pd3dIndexBuffer)
	{
		pd3dCommandList->IASetIndexBuffer(&m_d3dIndexBufferView);
		pd3dCommandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

void CMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	// 이 함수는 CMeshFromFile 에서 재정의하여 사용합니다.
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// 파일 로딩 관련 클래스 구현부
/////////////////////////////////////////////////////////////////////////////////////////////////

CMeshLoadInfo::~CMeshLoadInfo()
{
	if (m_pxmf3Positions) delete[] m_pxmf3Positions;
	if (m_pxmf3Normals) delete[] m_pxmf3Normals;
	if (m_pnIndices) delete[] m_pnIndices;
	if (m_pnSubSetIndices) delete[] m_pnSubSetIndices;
	for (int i = 0; i < m_nSubMeshes; i++) if (m_ppnSubSetIndices[i]) delete[] m_ppnSubSetIndices[i];
	if (m_ppnSubSetIndices) delete[] m_ppnSubSetIndices;
}

CMeshFromFile::CMeshFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CMeshLoadInfo *pMeshInfo) : CMesh(pMeshInfo->m_nSubMeshes)
{
	m_nVertices = pMeshInfo->m_nVertices;
	m_nType = pMeshInfo->m_nType;
	m_nStride = sizeof(XMFLOAT3);

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMeshInfo->m_pxmf3Positions, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	m_nSubMeshes = pMeshInfo->m_nSubMeshes;
	if (m_nSubMeshes > 0)
	{
		m_ppd3dSubSetIndexBuffers = new ID3D12Resource * [m_nSubMeshes];
		m_ppd3dSubSetIndexUploadBuffers = new ID3D12Resource * [m_nSubMeshes];
		m_pd3dSubSetIndexBufferViews = new D3D12_INDEX_BUFFER_VIEW[m_nSubMeshes];
		m_pnSubSetIndices = new int[m_nSubMeshes];

		for (int i = 0; i < m_nSubMeshes; i++)
		{
			m_pnSubSetIndices[i] = pMeshInfo->m_pnSubSetIndices[i];
			m_ppd3dSubSetIndexBuffers[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMeshInfo->m_ppnSubSetIndices[i], sizeof(UINT) * m_pnSubSetIndices[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_ppd3dSubSetIndexUploadBuffers[i]);
			m_pd3dSubSetIndexBufferViews[i].BufferLocation = m_ppd3dSubSetIndexBuffers[i]->GetGPUVirtualAddress();
			m_pd3dSubSetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
			m_pd3dSubSetIndexBufferViews[i].SizeInBytes = sizeof(UINT) * pMeshInfo->m_pnSubSetIndices[i];
		}
	}
}

CMeshFromFile::~CMeshFromFile()
{
	if (m_nSubMeshes > 0)
	{
		for (int i = 0; i < m_nSubMeshes; i++)
		{
			if (m_ppd3dSubSetIndexBuffers[i]) m_ppd3dSubSetIndexBuffers[i]->Release();
		}
		if (m_ppd3dSubSetIndexBuffers) delete[] m_ppd3dSubSetIndexBuffers;
		if (m_pd3dSubSetIndexBufferViews) delete[] m_pd3dSubSetIndexBufferViews;
		if (m_pnSubSetIndices) delete[] m_pnSubSetIndices;
	}
}

void CMeshFromFile::ReleaseUploadBuffers()
{
	CMesh::ReleaseUploadBuffers();
	if ((m_nSubMeshes > 0) && m_ppd3dSubSetIndexUploadBuffers)
	{
		for (int i = 0; i < m_nSubMeshes; i++)
		{
			if (m_ppd3dSubSetIndexUploadBuffers[i]) m_ppd3dSubSetIndexUploadBuffers[i]->Release();
		}
		if (m_ppd3dSubSetIndexUploadBuffers) delete[] m_ppd3dSubSetIndexUploadBuffers;
		m_ppd3dSubSetIndexUploadBuffers = NULL;
	}
}

void CMeshFromFile::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dVertexBufferView);
	if ((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_pnSubSetIndices[nSubSet], 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

CMeshIlluminatedFromFile::CMeshIlluminatedFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CMeshLoadInfo *pMeshInfo) : CMeshFromFile(pd3dDevice, pd3dCommandList, pMeshInfo)
{
	m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pMeshInfo->m_pxmf3Normals, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);
	m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
	m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
}

CMeshIlluminatedFromFile::~CMeshIlluminatedFromFile()
{
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
}

void CMeshIlluminatedFromFile::ReleaseUploadBuffers()
{
	CMeshFromFile::ReleaseUploadBuffers();
	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;
}

void CMeshIlluminatedFromFile::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nSubSet)
{
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[2] = { m_d3dVertexBufferView, m_d3dNormalBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 2, pVertexBufferViews);

	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	if (m_nSubMeshes > 0 && nSubSet < m_nSubMeshes)
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_pnSubSetIndices[nSubSet], 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////
// 지형 메쉬 생성 클래스 구현부
////////////////////////////////////////////////////////////////////////////////////////////////

CHeightMapGridMesh::CHeightMapGridMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int xStart, int zStart, int nWidth, int nLength, XMFLOAT3 xmf3Scale, void* pContext) : CMesh()
{
	m_nVertices = nWidth * nLength;
	m_nStride = sizeof(CIlluminatedVertex); // 위치 + 법선
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	std::vector<CIlluminatedVertex> pVertices(m_nVertices);
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;

	for (int i = 0, z = zStart; z < (zStart + nLength); z++)
	{
		for (int x = xStart; x < (xStart + nWidth); x++, i++)
		{
			pVertices[i].m_xmf3Position = XMFLOAT3((x * xmf3Scale.x), pHeightMapImage->GetHeight((float)x, (float)z), (z * xmf3Scale.z));
			pVertices[i].m_xmf3Normal = pHeightMapImage->GetHeightMapNormal(x, z);
		}
	}

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices.data(), m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	m_nIndices = ((nWidth * 2) * (nLength - 1)) + (nLength - 2);
	std::vector<UINT> pnIndices(m_nIndices);
	for (int j = 0, z = 0; z < nLength - 1; z++)
	{
		if ((z % 2) == 0) // 왼쪽에서 오른쪽으로
		{
			for (int x = 0; x < nWidth; x++)
			{
				if ((x == 0) && (z > 0)) pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else // 오른쪽에서 왼쪽으로
		{
			for (int x = nWidth - 1; x >= 0; x--)
			{
				if (x == (nWidth - 1)) pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
	}
	m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pnIndices.data(), sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CHeightMapGridMesh::~CHeightMapGridMesh()
{}

////////////////////////////////////////////////////////////////////////////////////////////////
// 지형 관련 클래스 구현부
////////////////////////////////////////////////////////////////////////////////////////////////

CHeightMapImage::CHeightMapImage(LPCTSTR pFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale)
{
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;
	m_pHeightMapPixels = new BYTE[m_nWidth * m_nLength];

	HANDLE hFile = ::CreateFile(pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD dwBytesRead;
	::ReadFile(hFile, m_pHeightMapPixels, (m_nWidth * m_nLength), &dwBytesRead, NULL);
	::CloseHandle(hFile);
}

CHeightMapImage::~CHeightMapImage()
{
	if (m_pHeightMapPixels) delete[] m_pHeightMapPixels;
}

float CHeightMapImage::GetHeight(float fx, float fz)
{
	if (!m_pHeightMapPixels) return 0.0f;
	if ((fx < 0.0f) || (fz < 0.0f) || (fx >= m_nWidth - 1) || (fz >= m_nLength - 1)) return 0.0f;

	int x = (int)fx;
	int z = (int)fz;
	float fxPercent = fx - x;
	float fzPercent = fz - z;

	float fBottomLeft = (float)m_pHeightMapPixels[x + (z * m_nWidth)];
	float fBottomRight = (float)m_pHeightMapPixels[(x + 1) + (z * m_nWidth)];
	float fTopLeft = (float)m_pHeightMapPixels[x + ((z + 1) * m_nWidth)];
	float fTopRight = (float)m_pHeightMapPixels[(x + 1) + ((z + 1) * m_nWidth)];

	float fTopHeight = fTopLeft * (1 - fxPercent) + fTopRight * fxPercent;
	float fBottomHeight = fBottomLeft * (1 - fxPercent) + fBottomRight * fxPercent;
	float fHeight = fBottomHeight * (1 - fzPercent) + fTopHeight * fzPercent;

	return fHeight;
}

XMFLOAT3 CHeightMapImage::GetHeightMapNormal(int x, int z)
{
	if (!m_pHeightMapPixels) return(XMFLOAT3(0.0f, 1.0f, 0.0f));
	if ((x < 0.0f) || (z < 0.0f) || (x >= m_nWidth) || (z >= m_nLength)) return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	int nHeightMapIndex = x + (z * m_nWidth);
	int xHeightMapAdd = (x < (m_nWidth - 1)) ? 1 : -1;
	int zHeightMapAdd = (z < (m_nLength - 1)) ? m_nWidth : -m_nWidth;

	float y1 = (float)m_pHeightMapPixels[nHeightMapIndex] * m_xmf3Scale.y;
	float y2 = (float)m_pHeightMapPixels[nHeightMapIndex + xHeightMapAdd] * m_xmf3Scale.y;
	float y3 = (float)m_pHeightMapPixels[nHeightMapIndex + zHeightMapAdd] * m_xmf3Scale.y;

	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, m_xmf3Scale.z);
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(m_xmf3Scale.x, y2 - y1, 0.0f);
	XMFLOAT3 xmf3Normal = Vector3::CrossProduct(xmf3Edge1, xmf3Edge2, true);

	return(xmf3Normal);
}


CHeightMapTerrain::CHeightMapTerrain(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, int nBlockWidth, int nBlockLength, XMFLOAT3 xmf3Scale) : CGameObject(1)
{
	m_nWidth = nWidth;
	m_nLength = nLength;
	m_xmf3Scale = xmf3Scale;

	m_pHeightMapImage = new CHeightMapImage(pFileName, nWidth, nLength, xmf3Scale);

	CHeightMapGridMesh* pMesh = new CHeightMapGridMesh(pd3dDevice, pd3dCommandList, 0, 0, nWidth, nLength, xmf3Scale, m_pHeightMapImage);
	SetMesh(pMesh);

	CMaterial* pMaterial = new CMaterial();
	MATERIALLOADINFO materialInfo;
	materialInfo.m_xmf4AlbedoColor = XMFLOAT4(0.4f, 0.6f, 0.2f, 1.0f);
	materialInfo.m_xmf4SpecularColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 2.0f);
	pMaterial->SetMaterialColors(new CMaterialColors(&materialInfo));
	pMaterial->SetIlluminatedShader();
	SetMaterial(0, pMaterial);
}

CHeightMapTerrain::~CHeightMapTerrain()
{
	if (m_pHeightMapImage) delete m_pHeightMapImage;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// CCubeMeshDiffused 구현부 (조명 지원 버전)
////////////////////////////////////////////////////////////////////////////////////////////////

CCubeMeshDiffused::CCubeMeshDiffused(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth) : CMesh()
{
	// 조명을 위해 각 면이 고유한 법선 벡터를 갖도록 24개의 정점으로 큐브를 정의합니다.
	m_nVertices = 24;
	m_nStride = sizeof(CIlluminatedVertex); // 위치와 법선을 갖는 정점 구조체
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;

	CIlluminatedVertex pVertices[24];

	// 앞면 (Normal: 0,0,-1)
	pVertices[0] = CIlluminatedVertex(XMFLOAT3(-fx, +fy, -fz), XMFLOAT3(0.0f, 0.0f, -1.0f));
	pVertices[1] = CIlluminatedVertex(XMFLOAT3(+fx, +fy, -fz), XMFLOAT3(0.0f, 0.0f, -1.0f));
	pVertices[2] = CIlluminatedVertex(XMFLOAT3(-fx, -fy, -fz), XMFLOAT3(0.0f, 0.0f, -1.0f));
	pVertices[3] = CIlluminatedVertex(XMFLOAT3(+fx, -fy, -fz), XMFLOAT3(0.0f, 0.0f, -1.0f));
	// 뒷면 (Normal: 0,0,1)
	pVertices[4] = CIlluminatedVertex(XMFLOAT3(-fx, +fy, +fz), XMFLOAT3(0.0f, 0.0f, +1.0f));
	pVertices[5] = CIlluminatedVertex(XMFLOAT3(+fx, +fy, +fz), XMFLOAT3(0.0f, 0.0f, +1.0f));
	pVertices[6] = CIlluminatedVertex(XMFLOAT3(-fx, -fy, +fz), XMFLOAT3(0.0f, 0.0f, +1.0f));
	pVertices[7] = CIlluminatedVertex(XMFLOAT3(+fx, -fy, +fz), XMFLOAT3(0.0f, 0.0f, +1.0f));
	// 윗면 (Normal: 0,1,0)
	pVertices[8] = CIlluminatedVertex(XMFLOAT3(-fx, +fy, +fz), XMFLOAT3(0.0f, +1.0f, 0.0f));
	pVertices[9] = CIlluminatedVertex(XMFLOAT3(+fx, +fy, +fz), XMFLOAT3(0.0f, +1.0f, 0.0f));
	pVertices[10] = CIlluminatedVertex(XMFLOAT3(-fx, +fy, -fz), XMFLOAT3(0.0f, +1.0f, 0.0f));
	pVertices[11] = CIlluminatedVertex(XMFLOAT3(+fx, +fy, -fz), XMFLOAT3(0.0f, +1.0f, 0.0f));
	// 아랫면 (Normal: 0,-1,0)
	pVertices[12] = CIlluminatedVertex(XMFLOAT3(-fx, -fy, -fz), XMFLOAT3(0.0f, -1.0f, 0.0f));
	pVertices[13] = CIlluminatedVertex(XMFLOAT3(+fx, -fy, -fz), XMFLOAT3(0.0f, -1.0f, 0.0f));
	pVertices[14] = CIlluminatedVertex(XMFLOAT3(-fx, -fy, +fz), XMFLOAT3(0.0f, -1.0f, 0.0f));
	pVertices[15] = CIlluminatedVertex(XMFLOAT3(+fx, -fy, +fz), XMFLOAT3(0.0f, -1.0f, 0.0f));
	// 왼쪽면 (Normal: -1,0,0)
	pVertices[16] = CIlluminatedVertex(XMFLOAT3(-fx, +fy, +fz), XMFLOAT3(-1.0f, 0.0f, 0.0f));
	pVertices[17] = CIlluminatedVertex(XMFLOAT3(-fx, +fy, -fz), XMFLOAT3(-1.0f, 0.0f, 0.0f));
	pVertices[18] = CIlluminatedVertex(XMFLOAT3(-fx, -fy, +fz), XMFLOAT3(-1.0f, 0.0f, 0.0f));
	pVertices[19] = CIlluminatedVertex(XMFLOAT3(-fx, -fy, -fz), XMFLOAT3(-1.0f, 0.0f, 0.0f));
	// 오른쪽면 (Normal: 1,0,0)
	pVertices[20] = CIlluminatedVertex(XMFLOAT3(+fx, +fy, -fz), XMFLOAT3(+1.0f, 0.0f, 0.0f));
	pVertices[21] = CIlluminatedVertex(XMFLOAT3(+fx, +fy, +fz), XMFLOAT3(+1.0f, 0.0f, 0.0f));
	pVertices[22] = CIlluminatedVertex(XMFLOAT3(+fx, -fy, -fz), XMFLOAT3(+1.0f, 0.0f, 0.0f));
	pVertices[23] = CIlluminatedVertex(XMFLOAT3(+fx, -fy, +fz), XMFLOAT3(+1.0f, 0.0f, 0.0f));

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	m_nIndices = 36;
	UINT pnIndices[36];
	// 앞면
	pnIndices[0] = 0; pnIndices[1] = 1; pnIndices[2] = 2;
	pnIndices[3] = 2; pnIndices[4] = 1; pnIndices[5] = 3;
	// 뒷면
	pnIndices[6] = 4; pnIndices[7] = 6; pnIndices[8] = 5;
	pnIndices[9] = 5; pnIndices[10] = 6; pnIndices[11] = 7;
	// 윗면
	pnIndices[12] = 8; pnIndices[13] = 9; pnIndices[14] = 10;
	pnIndices[15] = 10; pnIndices[16] = 9; pnIndices[17] = 11;
	// 아랫면
	pnIndices[18] = 12; pnIndices[19] = 14; pnIndices[20] = 13;
	pnIndices[21] = 13; pnIndices[22] = 14; pnIndices[23] = 15;
	// 왼쪽면
	pnIndices[24] = 16; pnIndices[25] = 17; pnIndices[26] = 18;
	pnIndices[27] = 18; pnIndices[28] = 17; pnIndices[29] = 19;
	// 오른쪽면
	pnIndices[30] = 20; pnIndices[31] = 23; pnIndices[32] = 21;
	pnIndices[33] = 20; pnIndices[34] = 22; pnIndices[35] = 23;

	m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pnIndices, sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CCubeMeshDiffused::~CCubeMeshDiffused()
{}

// Mesh.cpp 파일 맨 아래에 추가

// 화면 좌표계에 직접 그릴 삼각형 메쉬
CScreenAlignedTriangleMesh::CScreenAlignedTriangleMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList) : CMesh()
{
	m_nVertices = 3;
	m_nStride = sizeof(XMFLOAT4) * 2; // 위치(float4) + 색상(float4)
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 화면 좌표계(NDC)의 정점 위치와 색상입니다.
	float pVertices[][8] =
	{
		{ 0.0f, 0.5f, 0.5f, 1.0f,  1.0f, 0.0f, 0.0f, 1.0f }, // x,y,z,w, r,g,b,a
		{ 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f },
		{ -0.5f, -0.5f, 0.5f, 1.0f,0.0f, 0.0f, 1.0f, 1.0f }
	};

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;
}

CScreenAlignedTriangleMesh::~CScreenAlignedTriangleMesh() {}
