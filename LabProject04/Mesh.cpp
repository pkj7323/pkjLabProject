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
	// 기본 메쉬는 서브셋 개념이 없으므로, 전체 메쉬를 렌더링
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
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

CMeshIlluminatedFromFile::CMeshIlluminatedFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CMeshLoadInfo *pMeshInfo)
	: CMeshFromFile(pd3dDevice, pd3dCommandList, pMeshInfo)
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
// Mesh.cpp

// CHeightMapGridMesh 생성자 위에 이 Helper 함수를 추가합니다.
XMFLOAT4 OnGetColor(int x, int z, void* pContext)
{
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;
	XMFLOAT3 xmf3LightDirection = XMFLOAT3(-1.0f, 1.0f, 1.0f);
	xmf3LightDirection = Vector3::Normalize(xmf3LightDirection);
	XMFLOAT4 xmf4IncidentLightColor(0.9f, 0.8f, 0.4f, 1.0f);

	float fScale = Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x, z), xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x + 1, z), xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x + 1, z + 1), xmf3LightDirection);
	fScale += Vector3::DotProduct(pHeightMapImage->GetHeightMapNormal(x, z + 1), xmf3LightDirection);
	fScale = (fScale / 4.0f) + 0.05f;
	if (fScale > 1.0f) fScale = 1.0f;
	if (fScale < 0.25f) fScale = 0.25f;

	XMFLOAT4 xmf4Color = Vector4::Multiply(xmf4IncidentLightColor, fScale);
	return(xmf4Color);
}


CHeightMapGridMesh::CHeightMapGridMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, int xStart, int zStart, int nWidth, int nLength, XMFLOAT3 xmf3Scale, void* pContext) : CMesh()
{
	m_nVertices = nWidth * nLength;
	m_nStride = sizeof(CIlluminatedVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

	std::vector<CIlluminatedVertex> pVertices(m_nVertices);
	CHeightMapImage* pHeightMapImage = (CHeightMapImage*)pContext;

	for (int i = 0, z = zStart; z < (zStart + nLength); z++)
	{
		for (int x = xStart; x < (xStart + nWidth); x++, i++)
		{
			float height = pHeightMapImage->GetHeight((float)x, (float)z);

			// --- 수정된 부분 ---
			// 높이 값(height)에도 Y스케일(xmf3Scale.y)을 곱해줍니다.
			pVertices[i].m_xmf3Position = XMFLOAT3((x * xmf3Scale.x), height * xmf3Scale.y, (z * xmf3Scale.z));
			// --- 여기까지 ---

			pVertices[i].m_xmf3Normal = pHeightMapImage->GetHeightMapNormal(x, z);
			pVertices[i].m_xmf4Color = OnGetColor(x, z, pContext);
		}
	}

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices.data(), m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	// 인덱스 버퍼 생성 코드는 이전과 동일합니다.
	m_nIndices = ((nWidth * 2) * (nLength - 1)) + (nLength - 2);
	std::vector<UINT> pnIndices(m_nIndices);
	for (int j = 0, z = 0; z < nLength - 1; z++)
	{
		if ((z % 2) == 0)
		{
			for (int x = 0; x < nWidth; x++)
			{
				if ((x == 0) && (z > 0)) pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)(x + (z * nWidth));
				pnIndices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else
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

	// --- 수정된 부분: 파일 열기 및 읽기 오류 확인 ---
	HANDLE hFile = ::CreateFile(pFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		// 파일을 열지 못했다면, 디버그 창에 메시지를 출력합니다.
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("오류: 높이 맵 파일을 찾을 수 없습니다: %s\n"), pFileName);
		OutputDebugString(pstrDebug);
		// 메쉬가 이상하게 그려지는 것을 막기 위해 모든 높이를 0으로 설정합니다.
		memset(m_pHeightMapPixels, 0, sizeof(BYTE) * m_nWidth * m_nLength);
		return;
	}

	DWORD dwBytesRead;
	::ReadFile(hFile, m_pHeightMapPixels, (m_nWidth * m_nLength), &dwBytesRead, NULL);
	::CloseHandle(hFile);

	// 파일에서 읽은 데이터가 예상보다 적은 경우
	if (dwBytesRead < (DWORD)(m_nWidth * m_nLength))
	{
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("경고: 높이 맵 파일의 크기가 예상보다 작습니다.\n"));
		OutputDebugString(pstrDebug);
		// 남은 부분을 0으로 채웁니다.
		memset(m_pHeightMapPixels + dwBytesRead, 0, sizeof(BYTE) * ((m_nWidth * m_nLength) - dwBytesRead));
	}
	// --- 여기까지 ---
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
	// 더 밝은 색상으로 설정
	materialInfo.m_xmf4EmissiveColor = XMFLOAT4(0.1,0.1,0.1, 1.0f); // 기존 0.8 → 1.5
	materialInfo.m_xmf4AlbedoColor = XMFLOAT4(0.0,0.4,0.0, 1.0f);   // 기존 1.0 → 1.5
	materialInfo.m_xmf4SpecularColor = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f); // specular도 약간 증가
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
	// 8개의 정점만 사용하는 큐브
	m_nVertices = 8;
	m_nStride = sizeof(CIlluminatedVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;

	// 8개 꼭짓점 정의 (법선은 임시로 0,0,0, 실제 조명 계산시 보간됨)
	CIlluminatedVertex pVertices[8] = {
		CIlluminatedVertex(XMFLOAT3(-fx, +fy, -fz), XMFLOAT3(0,0,0)), // 0: 좌상앞
		CIlluminatedVertex(XMFLOAT3(+fx, +fy, -fz), XMFLOAT3(0,0,0)), // 1: 우상앞
		CIlluminatedVertex(XMFLOAT3(-fx, -fy, -fz), XMFLOAT3(0,0,0)), // 2: 좌하앞
		CIlluminatedVertex(XMFLOAT3(+fx, -fy, -fz), XMFLOAT3(0,0,0)), // 3: 우하앞
		CIlluminatedVertex(XMFLOAT3(-fx, +fy, +fz), XMFLOAT3(0,0,0)), // 4: 좌상뒤
		CIlluminatedVertex(XMFLOAT3(+fx, +fy, +fz), XMFLOAT3(0,0,0)), // 5: 우상뒤
		CIlluminatedVertex(XMFLOAT3(-fx, -fy, +fz), XMFLOAT3(0,0,0)), // 6: 좌하뒤
		CIlluminatedVertex(XMFLOAT3(+fx, -fy, +fz), XMFLOAT3(0,0,0)), // 7: 우하뒤
	};

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	// 인덱스(시계방향, CW)
	m_nIndices = 36;
	UINT pnIndices[36] = {
		// 앞면
		0, 2, 1, 1, 2, 3,
		// 뒷면
		4, 5, 6, 5, 7, 6,
		// 윗면
		4, 0, 5, 5, 0, 1,
		// 아랫면
		2, 6, 3, 3, 6, 7,
		// 왼쪽면
		4, 6, 0, 0, 6, 2,
		// 오른쪽면
		1, 3, 5, 5, 3, 7
	};

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


////////////////////////////////////////////////////////////////////////////////////////////////
// CObjMesh 구현부 (OBJ 파일 로더)
////////////////////////////////////////////////////////////////////////////////////////////////


CObjMesh::CObjMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const std::string& pstrFileName) : CMesh()
{
	// 파일에서 읽어온 데이터를 임시로 저장할 벡터들
	std::vector<XMFLOAT3> positions;
	std::vector<XMFLOAT3> normals;
	std::vector<XMFLOAT2> texcoords; // 텍스처 좌표(vt)를 읽기 위한 벡터 추가

	// 최종적으로 GPU에 보낼 정점과 인덱스 데이터
	std::vector<CIlluminatedVertex> finalVertices;
	std::vector<UINT> finalIndices;

	std::ifstream file {pstrFileName};
	if (! file)
	{
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("오류: OBJ 파일을 찾을 수 없습니다: %hs\n"), pstrFileName);
		OutputDebugString(pstrDebug);
		__debugbreak();
		return;
	}

	std::string line;
	while (std::getline(file, line))
	{
		if (line.substr(0, 2) == "v ") // 정점 위치 (v x y z)
		{
			std::istringstream iss(line.substr(2));
			XMFLOAT3 pos;
			iss >> pos.x >> pos.y >> pos.z;
			positions.push_back(pos);
		}
		else if (line.substr(0, 3) == "vn ") // 정점 법선 (vn x y z)
		{
			std::istringstream iss(line.substr(3));
			XMFLOAT3 normal;
			iss >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (line.substr(0, 3) == "vt ") // **추가된 부분**: 텍스처 좌표 (vt u v)
		{
			std::istringstream iss(line.substr(3));
			XMFLOAT2 texcoord;
			iss >> texcoord.x >> texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (line.substr(0, 2) == "f ") // **수정된 부분**: 면 정보 (f v/vt/vn ...)
		{
			std::istringstream iss(line.substr(2));
			std::string part;
			for (int i = 0; i < 3; ++i)
			{
				iss >> part;
				size_t first_slash = part.find('/');
				size_t second_slash = part.find('/', first_slash + 1);

				UINT posIndex = std::stoul(part.substr(0, first_slash));
				// 텍스처 인덱스는 읽지만 지금은 사용하지 않습니다.
				// UINT texIndex = std::stoul(part.substr(first_slash + 1, second_slash - (first_slash + 1)));
				UINT normalIndex = std::stoul(part.substr(second_slash + 1));

				CIlluminatedVertex vertex;
				vertex.m_xmf3Position = positions[posIndex - 1]; // OBJ 인덱스는 1부터 시작하므로 -1
				vertex.m_xmf3Normal = normals[normalIndex - 1]; // OBJ 인덱스는 1부터 시작하므로 -1
				vertex.m_xmf4Color = RANDOM_COLOR;

				finalVertices.push_back(vertex);
				finalIndices.push_back(finalVertices.size() - 1);
			}
		}
	}

	// 이하 버퍼 생성 코드는 이전과 동일합니다.
	m_nVertices = finalVertices.size();
	m_nStride = sizeof(CIlluminatedVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_pd3dVertexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, finalVertices.data(), m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dVertexUploadBuffer);
	m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
	m_d3dVertexBufferView.StrideInBytes = m_nStride;
	m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

	m_nIndices = finalIndices.size();
	m_pd3dIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, finalIndices.data(), sizeof(UINT) * m_nIndices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_pd3dIndexUploadBuffer);
	m_d3dIndexBufferView.BufferLocation = m_pd3dIndexBuffer->GetGPUVirtualAddress();
	m_d3dIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_d3dIndexBufferView.SizeInBytes = sizeof(UINT) * m_nIndices;
}

CObjMesh::~CObjMesh()
{}
