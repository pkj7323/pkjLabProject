// Light.hlsl

#define MAX_LIGHTS			16 

#define POINT_LIGHT			1
#define SPOT_LIGHT			2
#define DIRECTIONAL_LIGHT	3

// Scene.h의 LIGHT 구조체와 일치합니다.
struct LIGHT
{
    float4 m_cAmbient;
    float4 m_cDiffuse;
    float4 m_cSpecular;
    float3 m_vPosition;
    float m_fFalloff;
    float3 m_vDirection;
    float m_fTheta; //cos(m_fTheta)
    float3 m_vAttenuation;
    float m_fPhi; //cos(m_fPhi)
    bool m_bEnable;
    int m_nType;
    float m_fRange;
    float padding;
};

// Scene.h의 LIGHTS 구조체와 일치합니다.
// register(b4)는 루트 시그니처 4번 파라미터에 해당합니다. (우리 코드에서는 Lights)
cbuffer cbLights : register(b4)
{
    LIGHT gLights[MAX_LIGHTS];
    float4 gcGlobalAmbientLight; // 전역 주변광
    int gnLights; // 활성화된 조명의 개수
};

// 픽셀의 최종 색상을 계산하는 메인 함수
float4 Lighting(float3 vPosition, float3 vNormal)
{
    float3 vToCamera = normalize(gvCameraPosition - vPosition);
    float4 cColor = float4(1.0f, 1.0f, 1.0f, 1.0f);

	// 모든 조명을 순회하며 색상을 더함
	[unroll(MAX_LIGHTS)] 
    for (int i = 0; i < gnLights; i++)
    {
        if (gLights[i].m_bEnable)
        {
            if (gLights[i].m_nType == DIRECTIONAL_LIGHT)
            {
				// 방향성 조명 계산
                float3 vToLight = -gLights[i].m_vDirection;
                float fDiffuseFactor = dot(vToLight, vNormal);
                float fSpecularFactor = 0.0f;
                if (fDiffuseFactor > 0.0f)
                {
                    float3 vHalf = normalize(vToCamera + vToLight);
                    fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
                }
                cColor += (gLights[i].m_cAmbient * gMaterial.m_cAmbient)
                        + (gLights[i].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse)
                        + (gLights[i].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular);
            }
            else if (gLights[i].m_nType == POINT_LIGHT)
            {
				// 점 조명 계산 (생략 - 필요시 추가 가능)
            }
            else if (gLights[i].m_nType == SPOT_LIGHT)
            {
				// 스포트라이트 계산
                float3 vToLight = gLights[i].m_vPosition - vPosition;
                float fDistance = length(vToLight);
                if (fDistance <= gLights[i].m_fRange)
                {
                    vToLight /= fDistance;
                    float fDiffuseFactor = dot(vToLight, vNormal);
                    float fSpecularFactor = 0.0f;
                    if (fDiffuseFactor > 0.0f)
                    {
                        float3 vHalf = normalize(vToCamera + vToLight);
                        fSpecularFactor = pow(max(dot(vHalf, vNormal), 0.0f), gMaterial.m_cSpecular.a);
                    }
                    float fAlpha = max(dot(-vToLight, gLights[i].m_vDirection), 0.0f);
                    float fSpotFactor = pow(max(((fAlpha - gLights[i].m_fPhi) / (gLights[i].m_fTheta - gLights[i].m_fPhi)), 0.0f), gLights[i].m_fFalloff);
                    float fAttenuationFactor = 1.0f / dot(gLights[i].m_vAttenuation, float3(1.0f, fDistance, fDistance * fDistance));

                    cColor += ((gLights[i].m_cAmbient * gMaterial.m_cAmbient)
                            + (gLights[i].m_cDiffuse * fDiffuseFactor * gMaterial.m_cDiffuse)
                            + (gLights[i].m_cSpecular * fSpecularFactor * gMaterial.m_cSpecular)) * fAttenuationFactor * fSpotFactor;
                }
            }
        }
    }
	// 전역 주변광을 더하고, 재질의 알파값을 최종 알파값으로 사용
    cColor += (gcGlobalAmbientLight * gMaterial.m_cAmbient);
    cColor.a = gMaterial.m_cDiffuse.a;

    return (cColor);
}