// Shaders.hlsl

// C++의 CMaterialColors 구조체와 일치하는 재질 정보
struct MATERIAL
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular; // a = power (반사광의 세기)
    float4 m_cEmissive;
};

// C++의 Camera.h에 있는 VS_CB_CAMERA_INFO 구조체와 일치합니다.
// register(b0)는 루트 시그니처 0번 파라미터에 해당합니다. (우리 코드에서는 Camera)
cbuffer cbCameraInfo : register(b0)
{
    matrix gmtxView;
    matrix gmtxProjection;
    float3 gvCameraPosition;
};

// C++의 CGameObject, CMaterial에서 넘겨주는 정보와 일치합니다.
// register(b2)는 루트 시그니처 2번 파라미터에 해당합니다. (우리 코드에서는 GameObject)
cbuffer cbGameObjectInfo : register(b2)
{
    matrix gmtxGameObject; // 월드 변환 행렬
    MATERIAL gMaterial; // 재질 정보
};

// 조명 계산 로직이 들어있는 파일을 포함합니다.
#include "Light.hlsl"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 정점 셰이더의 입력 구조체 (Mesh.h의 CIlluminatedVertex와 일치)
struct VS_LIGHTING_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

// 정점 셰이더의 출력 구조체 (픽셀 셰이더의 입력으로 전달됨)
struct VS_LIGHTING_OUTPUT
{
    float4 positionH : SV_POSITION; // H: Homogeneous (동차좌표계), 최종 변환된 좌표
    float3 positionW : POSITION; // W: World (월드좌표계)
    float3 normalW : NORMAL; // W: World (월드좌표계)
};

// 정점 셰이더 메인 함수
VS_LIGHTING_OUTPUT VSLighting(VS_LIGHTING_INPUT input)
{
    VS_LIGHTING_OUTPUT output;

	// 모델의 법선 벡터를 월드 변환 행렬을 이용해 월드 공간 법선 벡터로 변환
    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
	// 모델의 정점 위치를 월드 변환 행렬을 이용해 월드 공간 위치로 변환
    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
	// 월드 공간 위치를 뷰와 프로젝션 행렬을 이용해 최종 클립 공간 위치로 변환
    output.positionH = mul(float4(output.positionW, 1.0f), gmtxView);
    output.positionH = mul(output.positionH, gmtxProjection);
	
    return (output);
}

// 픽셀 셰이더 메인 함수
float4 PSLighting(VS_LIGHTING_OUTPUT input) : SV_TARGET
{
	// 정점 셰이더에서 보간된 법선 벡터를 정규화
    input.normalW = normalize(input.normalW);

    // Light.hlsl에 있는 Lighting 함수를 호출하여 최종 색상을 계산
    float4 cColor = Lighting(input.positionW, input.normalW);

    return (cColor);
}