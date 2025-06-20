// Test.hlsl

struct VS_INPUT
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

VS_OUTPUT VSPassThrough(VS_INPUT input)
{
    VS_OUTPUT output;
    // 월드/뷰/프로젝션 변환 없이 좌표를 그대로 넘깁니다.
    output.position = input.position;
    output.color = input.color;
    return output;
}

float4 PSPassThrough(VS_OUTPUT input) : SV_TARGET
{
    // 정점에서 넘어온 색상을 그대로 출력합니다.
    return input.color;
}