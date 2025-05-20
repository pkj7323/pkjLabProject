// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"

// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <iostream>
#include <string>
#include <wrl.h>
#include <shellapi.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <DXGIDebug.h>
using namespace DirectX;
using namespace DirectX::PackedVector;
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

#pragma comment(lib, "winmm.lib")


#define FRAME_BUFFER_WIDTH 640
#define FRAME_BUFFER_HEIGHT 480

#define HR_TO_STRING(x) #x

#ifdef _DEBUG
inline HRESULT DXCall(HRESULT hr)
{
	if (FAILED(hr))
	{
		char line_num[32];
		sprintf_s(line_num, "%u", __LINE__);
		OutputDebugStringA("Error in :");
		OutputDebugStringA(__FILE__);
		OutputDebugStringA("\nLine: ");
		OutputDebugStringA(line_num);
		OutputDebugStringA("\n");
		OutputDebugStringA(HR_TO_STRING(hr));
		OutputDebugStringA("\n");
		__debugbreak();
	}
	return hr;
}
#else
#ifndef DXCall
#define DXCall(x) x
#endif
#endif // DEBUG
//#define _WITH_SWAPCHAIN_FULLSCREEN_STATE