// Shaders.hlsl (정점 색상 사용 버전)

struct MATERIAL
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular;
    float4 m_cEmissive;
};

cbuffer cbCameraInfo : register(b0)
{
    matrix gmtxView;
    matrix gmtxProjection;
    float3 gvCameraPosition;
};

cbuffer cbGameObjectInfo : register(b2)
{
    matrix gmtxGameObject; // 월드 변환 행렬
    MATERIAL gMaterial; // 재질 정보
};


#include "Light.hlsl"

// 입력 구조체에 COLOR 추가
struct VS_LIGHTING_INPUT
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR; // 정점 색상 입력
};

// 출력 구조체에 COLOR 추가
struct VS_LIGHTING_OUTPUT
{
    float4 positionH : SV_POSITION;
    float3 positionW : POSITION;
    float3 normalW : NORMAL;
    float4 color : COLOR; // 정점 색상 출력
};

VS_LIGHTING_OUTPUT VSLighting(VS_LIGHTING_INPUT input)
{
    VS_LIGHTING_OUTPUT output;

    output.normalW = mul(input.normal, (float3x3) gmtxGameObject);
    output.positionW = (float3) mul(float4(input.position, 1.0f), gmtxGameObject);
	
    output.positionH = mul(float4(output.positionW, 1.0f), gmtxView);
    output.positionH = mul(output.positionH, gmtxProjection);

    output.color = input.color; // 입력받은 정점 색상을 그대로 전달
	
    return (output);
}

float4 PSLighting(VS_LIGHTING_OUTPUT input) : SV_TARGET
{
    input.normalW = normalize(input.normalW);
    float4 cLitColor = Lighting(input.positionW, input.normalW);

	// 최종 색상 = 조명 계산 색상 * 정점에서 보간된 색상
    return cLitColor * input.color;
}