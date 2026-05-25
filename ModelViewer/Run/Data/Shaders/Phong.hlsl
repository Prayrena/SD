float GetFractionWithinRange(float value, float rangeStart, float rangeEnd)
{
	float disp = rangeEnd - rangeStart;
	float proportion = (value - rangeStart) / disp;
	return proportion;
}

float Interpolate(float start, float end, float fractionTowardEnd)
{
	float disp = end - start;
	float distWithinRange = disp * fractionTowardEnd;
	float interpolatedPosition = start + distWithinRange;
	return interpolatedPosition;
}

float RangeMap(float inValue, float inStart, float inEnd, float outStart, float outEnd)
{
	float proportion = GetFractionWithinRange(inValue, inStart, inEnd);

	float outValue = Interpolate(outStart, outEnd, proportion);
	return outValue;
}


//----------------------------------------------------------------------------------------------------------------------------------------------------
struct vs_input_t
	{
		float3 localPosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
		float3 localTangent : TANGENT;
		float3 localBitangent : BITANGENT;
		float3 localNormal : NORMAL;
	};

//----------------------------------------------------------------------------------------------------------------------------------------------------
	struct v2p_t
	{
		float4 clipPosition : SV_Position;
		float4 worldPosition : POSITION;
		float4 color : COLOR;
		float2 uv : TEXCOORD;
		float4 worldTangent : TANGENT;
		float4 worldBitangent : BITANGENT;
		float4 worldNormal : NORMAL;
	};

//----------------------------------------------------------------------------------------------------------------------------------------------------
	struct ps_output_t
	{
		float4 colorRenderTarget : SV_Target0;
		float4 emissiveRenderTarget : SV_Target1;
	};

//----------------------------------------------------------------------------------------------------------------------------------------------------
cbuffer PhongLightingConstants : register(b1)
{
	float3  SunDirection;
	float	SunIntensity;
	float	AmbientIntensity;

	float3  WorldEyePosition;	

	float MinFalloff;
	float MaxFalloff;	
	float MinFalloffMultiplier;
	float MaxFalloffMultiplier;

	// PhongLightingDebug (as part of PhongLightingConstants)
    int    RenderAmbient;     // int
    int    RenderDiffuse;     // int
    int    RenderSpecular;    // int
    int    RenderEmissive;    // int
    int    UseDiffuseMap;     // int
    int    UseNormalMap;      // int
    int    UseSpecularMap;    // int
    int    UseGlossinessMap;  // int
    int    UseEmissiveMap;    // int
    float3 Padding;           // float[3] for padding
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
	cbuffer CameraConstants : register(b2)
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
	cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

//----------------------------------------------------------------------------------------------------------------------------------------------------
Texture2D diffuseTexture : register(t0);
Texture2D specGlossEmitTexture : register(t1);
Texture2D normalTexture : register(t2);

SamplerState samplerState : register(s0);


//----------------------------------------------------------------------------------------------------------------------------------------------------
	v2p_t VertexMain(vs_input_t input)
	{
		float4 localPosition = float4(input.localPosition, 1);
		float4 worldPosition = mul(ModelMatrix, localPosition);
		float4 viewPosition = mul(ViewMatrix, worldPosition);
		float4 clipPosition = mul(ProjectionMatrix, viewPosition);

		float4 localTangent = float4(input.localTangent, 0);
		float4 worldTangent = mul(ModelMatrix, localTangent);

		float4 localBitangent = float4(input.localBitangent, 0);
		float4 worldBitangent = mul(ModelMatrix, localBitangent);

		float4 localNormal = float4(input.localNormal, 0);
		float4 worldNormal = mul(ModelMatrix, localNormal);

		v2p_t v2p;
		v2p.clipPosition = clipPosition;
		v2p.worldPosition = worldPosition;
		v2p.color = input.color;
		v2p.uv = input.uv;
		v2p.worldTangent = worldTangent;
		v2p.worldBitangent = worldBitangent;
		v2p.worldNormal = worldNormal;
		return v2p;
	}

//----------------------------------------------------------------------------------------------------------------------------------------------------
	ps_output_t PixelMain(v2p_t input)
	{
		float4 modelColor = ModelColor;
		float4 vertexColor = input.color;
		float4 textureColor = float4(1.f, 1.f, 1.f, 1.f);
		if (UseDiffuseMap)
		{
			textureColor = diffuseTexture.Sample(samplerState, input.uv);
		}
		
		// ambient
		// multiply ambient by 1 or 0 depending on the value of the render debug flag
		float ambient = AmbientIntensity;
		if (!RenderAmbient)
		{
			ambient = 0.f;
		}
		
		// normal
		float3 vertexWorldNormal = normalize(input.worldNormal.xyz);
		float3 pixelWorldNormal = float3(0.f, 0.f, 0.f);
		float falloffMultiplier = 1.f;

		float normalDotLight;
		float vertexNormalDotLight = dot(vertexWorldNormal, -SunDirection);
		if (UseNormalMap)
		{
			float3 tangentNormal = 2.f * normalTexture.Sample(samplerState, input.uv).rgb - 1.f; 
			float3x3 tbnMatrix = float3x3(normalize(input.worldTangent.xyz), normalize(input.worldBitangent.xyz), normalize(input.worldNormal.xyz));
			pixelWorldNormal = mul(tangentNormal, tbnMatrix);
			// fall off of the normal
			normalDotLight = dot(pixelWorldNormal, -SunDirection);
		
			float falloff = clamp(vertexNormalDotLight, MinFalloff, MaxFalloff);
			float falloff_T = (falloff - MinFalloff) / (MaxFalloff - MinFalloff);
			falloffMultiplier = lerp(MinFalloffMultiplier, MaxFalloffMultiplier, falloff_T);
		}
		else
		{
			normalDotLight = vertexNormalDotLight; // if we do not use the normal map, then just use vertex world normal do lignt
		}	


		// diffuse
		// multiply diffuse by 1 or 0 depending on the value of the render debug flag
		float  diffuse = 0.f;
		if (RenderDiffuse)
		{
			diffuse	= SunIntensity * falloffMultiplier * saturate(normalDotLight);
		}

		// specular
		// multiply specular by 1 or 0 depending on the value of the render debug flag
		float4 specGlossEmit = specGlossEmitTexture.Sample(samplerState, input.uv);

		float  specularIntensity = 0.f;
		if (UseSpecularMap)
		{
			specularIntensity = specGlossEmit.r;
		}
		if (!RenderSpecular)
		{
			specularIntensity = 0.f;
		}

		// Glossiness
		float  specularPower = 1.f;
		if (UseGlossinessMap)
		{
			specularPower = 1.f + 31.f * specGlossEmit.g;
		}

		// emissive
		float  emissive = 0.f;
		if (UseEmissiveMap)
		{
			emissive = specGlossEmit.b;
		}
		if (!RenderEmissive)
		{
			emissive = 0.f;
		}

		float3 worldViewDirection = normalize(WorldEyePosition - input.worldPosition.xyz);
		float3 worldHalfVector = normalize(-SunDirection + worldViewDirection);
		float  nDotH = saturate(dot(vertexWorldNormal, worldHalfVector));	
		float  specular = specularIntensity * pow(nDotH, specularPower);
		
		// add specular to the following sum and saturate
		float4 lightColor = float4((ambient + diffuse + specular + emissive).xxx, 1);

		ps_output_t output;
		output.colorRenderTarget = lightColor * textureColor * vertexColor * modelColor;
		output.emissiveRenderTarget = float4(emissive.xxx, 1) * textureColor * vertexColor * modelColor;
 
		clip(output.colorRenderTarget.a - 0.01f);
		return output;
	}
