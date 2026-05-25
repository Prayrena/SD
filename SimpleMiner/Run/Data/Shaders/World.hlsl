float3 DiminishingAddComponents(float3 a, float3 b)
{
	return (1.f - (1.f - a) * (1.f - b)); 
};

// for the map lighting and fog
struct vs_input_t
{
	float3	a_position	: POSITION;
	float4	a_color		: COLOR; 
	float2	a_uv		: TEXCOORD; 
};

struct v2p_t // vertex to fragment
{
	float4 v_position : SV_Position;
	float4 v_color : COLOR; 
	float2 v_uv : TEXCOORD;
    float3 v_worldPosition : POSITION;
};

// b0 = system constants (e.g. toggle debug mode) - rare
// b1 = frame constants (e.g. time) - once per frame
// b2 = camera constants (e.g. view/proj matrices) - per camera begin
// b3 = model constants (e.g. model matrix & tint) - per draw call
// b4-7 = other engine-reserved slots
// b8-15 = game-specific / free slots
// NOTE: constant buffers MUST be 16B-aligned (size is multiple of 16B)
// Also, primitives may not cross 16B boundaries (unless they are 16B aligned, like Mat44)

Texture2D		t_diffuseTexture : register(t0);	// Texture bound in texture constant slot #0
SamplerState	s_diffuseSampler : register(s0);	// Sampler is bound in sampler constant slot #0

cbuffer CameraConstants : register(b2)
{
	float4x4 c_viewMatrix;			// world-to-gameCamera
	float4x4 c_projectionMatrix;	// gameCamera-to-clip (includes gameCamera->screenCamera axis swaps)
};

	cbuffer ModelConstants : register(b3)
{
	float4x4 ModelMatrix;
	float4 ModelColor;
};

cbuffer SimpleMinerWorldConstants : register(b8)
{
	float4	c_indoorLightColor;		// gameCamera-to-clip (includes gameCamera->screenCamera axis swaps)
	float4	c_outdoorLightColor;	// gameCamera-to-clip (includes gameCamera->screenCamera axis swaps)
    float4	c_cameraWorldPos;		// world position of camera being used to render
	float4	c_fog_Sky_Color;		// Fog and sky color to blend in
	float   c_fogStartDistance;		// World units away where fog begins (0%)
	float	c_fogEndDistance;		// World units away where fog maxes out (100%)
	float	c_fogMaxAlpha;			// fog max alpha 
	float	c_dummyPadding1;
};

//------------------------------------------------------------------------------------------------
// Main Entry Point for the vertex stage
// which for graphical shaders is usually a main entry point
// and will get information from the game
v2p_t VertexMain( vs_input_t input )
{
	v2p_t v2f;
	
	float4 localPos = float4( input.a_position, 1 );
	float4 worldPos = mul(ModelMatrix, localPos);
	float4 cameraPos = mul( c_viewMatrix, worldPos );
	float4 clipPos = mul( c_projectionMatrix, cameraPos );

	// we defined the position as a 3 dimensional coordinate, but SV_Position expects a clip/perspective space coordinate (4D).  More on this later.  For now, just pass 1 for w; 
	v2f.v_position = clipPos;
	v2f.v_color = input.a_color;
	v2f.v_uv = input.a_uv;
    v2f.v_worldPosition = worldPos.xyz;

	return v2f; // pass it on to the raster stage
}

//------------------------------------------------------------------------------------------------

float4 PixelMain( v2p_t input ) : SV_Target0
{
	float2 uvCoords = input.v_uv;
	float4 texelColor = t_diffuseTexture.Sample( s_diffuseSampler, uvCoords );
	float4 rgbEncodedData = input.v_color;
	if( texelColor.a <= 0.001 ) 
	{
		discard; // Skip writing color AND especially depth (if depth-writing is enabled) for transparent pixels
	}

	//  compute lit pixel color
	float outdoorLightBrightness = rgbEncodedData.r;
	float3 outdoorLightTint = c_outdoorLightColor.rgb * outdoorLightBrightness;

	float indoorLightBrightness = rgbEncodedData.g;
	float3 indoorLightTint = c_indoorLightColor.rgb * indoorLightBrightness;	

	float3 diffuseLight = DiminishingAddComponents(indoorLightTint, outdoorLightTint);
	
	float3 diffuseColorRGB = diffuseLight * texelColor.rgb;
	
	// compute fog
	// get RGB by lerp the color from diffuse color to fog/sky color
    float distCamToPixel = distance(input.v_worldPosition.xyz, c_cameraWorldPos.xyz);
    float fogFraction = saturate((distCamToPixel - c_fogStartDistance) / (c_fogEndDistance - c_fogStartDistance));
	float fogDensity = c_fogMaxAlpha * fogFraction;
    float3 finalColorRGB = lerp(diffuseColorRGB, c_fog_Sky_Color.rgb, fogDensity);
	// get alpha
	float finalAlpha = saturate(texelColor.a + fogDensity); // or: saturate(texelColor.a * (1.f - fogFraction) + fogDensity)
	float4 finalColor = float4(finalColorRGB, finalAlpha);
	
	return finalColor;
}

