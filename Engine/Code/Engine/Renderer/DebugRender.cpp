#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Renderer/DebugRenderGeometry.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/Clock.hpp"

std::vector<DebugRenderGeometry*>	g_debugRenderGeometries;
DebugRenderConfig					g_theDebugRenderConfig;
bool								g_debugRenderVisibility = true;
Vec3								g_debugOrientation;

extern	Clock*						g_theGameClock;

void DebugRenderSystemStartup(DebugRenderConfig const& config)
{
	g_theDebugRenderConfig.m_renderer = config.m_renderer;
	g_theDebugRenderConfig.m_fontName = config.m_fontName;
	g_theDebugRenderConfig.m_font = config.m_font;

	g_debugRenderGeometries.reserve(3);
}

void DebugRenderSystemShutDown()
{
	for ( int i = 0; i < (int)(g_debugRenderGeometries.size()); ++i)
	{
		if (g_debugRenderGeometries[i])
		{
			delete g_debugRenderGeometries[i];
		}
	}
	g_debugRenderGeometries.clear();
}

void SetDebugRenderVisibility(bool newVisibilityStatus)
{
	g_debugRenderVisibility = newVisibilityStatus;
}

bool GetDebugRenderVisibility()
{
	return g_debugRenderVisibility;
}

void DebugRenderSetHidden()
{
	g_debugRenderVisibility = false;
}

void DebugRenderBeginFrame()
{
	// update each geometry's info
	for (int i = 0; i < (int)(g_debugRenderGeometries.size()); ++i)
	{
		if (g_debugRenderGeometries[i])
		{
			// if the debug geometry is a billboard, then we updating its tracking target transform info every second
			if (g_debugRenderGeometries[i]->m_type == DebugRenderGeometryType::BILLBOARD)
			{
				g_debugRenderGeometries[i]->m_trackingTargetTransform = g_theDebugRenderConfig.m_camera->GetModelMatrix();
			}
			
			g_debugRenderGeometries[i]->UpdateGeometry();
		}
	}
}

void DebugRenderEndFrame()
{
	// check every geometry's time duration and delete the aged ones
	for (int i = 0; i < (int)(g_debugRenderGeometries.size()); ++i)
	{
		if (g_debugRenderGeometries[i])
		{
			// if the timer is past or the frames left is 
			if (g_debugRenderGeometries[i]->m_timer->HasPeroidElapsed() || g_debugRenderGeometries[i]->m_framesLeft < 0 || g_debugRenderGeometries[i]->m_vertices.empty())
			{
				delete g_debugRenderGeometries[i];
				g_debugRenderGeometries.erase(g_debugRenderGeometries.begin() + i);
			}
		}
	}
}

void DebugRenderWorld(const Camera& camera)
{
#ifdef SHIPPING
	return;
#endif

	// if the visibility is turned off, do not render
	if (!g_debugRenderVisibility)
	{
		return;
	}

	g_theDebugRenderConfig.m_renderer->BeginCamera(camera);
	Renderer*& renderer = g_theDebugRenderConfig.m_renderer;

	for (int i = 0; i < (int)(g_debugRenderGeometries.size()); ++i)
	{
		if (g_debugRenderGeometries[i])
		{
			if (g_debugRenderGeometries[i]->m_showOnlyOneFrame)
			{
				if (g_debugRenderGeometries[i]->m_framesLeft <= 0)
				{
					// delete the debug render geometries
					auto iter = g_debugRenderGeometries.begin() + i;
					delete* iter;
					g_debugRenderGeometries.erase(g_debugRenderGeometries.begin() + i);
					-- i;
					continue;
				}
				// else
				// {
				// 	g_debugRenderGeometries[i]->m_framesLeft
				// }
			}

			DebugRenderGeometry*& geometry = g_debugRenderGeometries[i];
			if ((int)geometry->m_vertices.size() == 0)
			{
				continue;
			}

			switch (geometry->m_debugRendermode)
			{
			case DebugRenderMode::ALWAYS: // always draw with depth disabled
			{
				renderer->BindShader(nullptr);
				renderer->BindTexture(geometry->m_texture);
				renderer->SetBlendMode(BlendMode::ALPHA);
				renderer->SetModelConstants(geometry->m_modelMatrix, geometry->m_currentColor);
				renderer->SetDepthMode(DepthMode::DISABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());
			}break;
			case DebugRenderMode::USE_DEPTH:
			{
				renderer->BindShader(nullptr);
				renderer->BindTexture(geometry->m_texture);
				renderer->SetBlendMode(BlendMode::ALPHA);
				renderer->SetModelConstants(geometry->m_modelMatrix, geometry->m_currentColor);
				renderer->SetDepthMode(DepthMode::ENABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());
			}break;
			case DebugRenderMode::X_RAY:
			{
				renderer->BindShader(nullptr);
				renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
				// The first pass should be rendered with alpha blending and with depth set to read only 
				renderer->BindTexture(geometry->m_texture);
				// the color is based on the current color but slightly lightened and made slightly transparent.
				Rgba8 newColor;
				newColor.r = DenormalizeByte((NormalizeByte((geometry->m_currentColor).r)) * 1.05f);
				newColor.g = DenormalizeByte((NormalizeByte((geometry->m_currentColor).g)) * 1.05f);
				newColor.b = DenormalizeByte((NormalizeByte((geometry->m_currentColor).b)) * 1.05f);
				newColor.a = DenormalizeByte((NormalizeByte((geometry->m_currentColor).a)) * 0.3f);
				
				renderer->SetBlendMode(BlendMode::ALPHA);
				renderer->SetModelConstants(geometry->m_modelMatrix, newColor);
				renderer->SetDepthMode(DepthMode::DISABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());

				// The second pass should be rendered opaque and with depth enabled and set to read write
				// and use the current color
				renderer->BindTexture(geometry->m_texture);
				renderer->SetBlendMode(BlendMode::OPAQUE);
				renderer->SetModelConstants(geometry->m_modelMatrix, geometry->m_currentColor);
				renderer->SetDepthMode(DepthMode::ENABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());
			}break;
			}
		}
	}
	g_theDebugRenderConfig.m_renderer->EndCamera(camera);
}

void DebugRenderWorldForVR()
{
#ifdef SHIPPING
	return;
#endif

	Renderer*& renderer = g_theDebugRenderConfig.m_renderer;

	for (int i = 0; i < (int)(g_debugRenderGeometries.size()); ++i)
	{
		if (g_debugRenderGeometries[i])
		{
			if (g_debugRenderGeometries[i]->m_showOnlyOneFrame)
			{
				if (g_debugRenderGeometries[i]->m_framesLeft <= 0)
				{
					// delete the debug render geometries
					auto iter = g_debugRenderGeometries.begin() + i;
					delete* iter;
					g_debugRenderGeometries.erase(g_debugRenderGeometries.begin() + i);
					--i;
					continue;
				}
				// else
				// {
				// 	g_debugRenderGeometries[i]->m_framesLeft
				// }
			}

			DebugRenderGeometry*& geometry = g_debugRenderGeometries[i];
			if ((int)geometry->m_vertices.size() == 0)
			{
				continue;
			}

			switch (geometry->m_debugRendermode)
			{
			case DebugRenderMode::ALWAYS: // always draw with depth disabled
			{
				renderer->BindShader(nullptr);
				renderer->BindTexture(geometry->m_texture);
				renderer->SetBlendMode(BlendMode::ALPHA);
				renderer->SetModelConstants(geometry->m_modelMatrix, geometry->m_currentColor);
				renderer->SetDepthMode(DepthMode::DISABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());
			}break;
			case DebugRenderMode::USE_DEPTH:
			{
				renderer->BindShader(nullptr);
				renderer->BindTexture(geometry->m_texture);
				renderer->SetBlendMode(BlendMode::ALPHA);
				renderer->SetModelConstants(geometry->m_modelMatrix, geometry->m_currentColor);
				renderer->SetDepthMode(DepthMode::ENABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());
			}break;
			case DebugRenderMode::X_RAY:
			{
				renderer->BindShader(nullptr);
				renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_NONE);
				// The first pass should be rendered with alpha blending and with depth set to read only 
				renderer->BindTexture(geometry->m_texture);
				// the color is based on the current color but slightly lightened and made slightly transparent.
				Rgba8 newColor;
				newColor.r = DenormalizeByte((NormalizeByte((geometry->m_currentColor).r)) * 1.05f);
				newColor.g = DenormalizeByte((NormalizeByte((geometry->m_currentColor).g)) * 1.05f);
				newColor.b = DenormalizeByte((NormalizeByte((geometry->m_currentColor).b)) * 1.05f);
				newColor.a = DenormalizeByte((NormalizeByte((geometry->m_currentColor).a)) * 0.3f);

				renderer->SetBlendMode(BlendMode::ALPHA);
				renderer->SetModelConstants(geometry->m_modelMatrix, newColor);
				renderer->SetDepthMode(DepthMode::DISABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());

				// The second pass should be rendered opaque and with depth enabled and set to read write
				// and use the current color
				renderer->BindTexture(geometry->m_texture);
				renderer->SetBlendMode(BlendMode::OPAQUE);
				renderer->SetModelConstants(geometry->m_modelMatrix, geometry->m_currentColor);
				renderer->SetDepthMode(DepthMode::ENABLED);
				renderer->SetRasterizerMode(geometry->m_rasterizerMode);
				renderer->DrawVertexArray((int)geometry->m_vertices.size(), geometry->m_vertices.data());
			}break;
			}
		}
	}
}

void DebugAddScreenText(std::string const& text, Vec2 const& position, float size, Vec2 const& alignment,
	float duration, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/)
{
	DebugRenderGeometry* message = new DebugRenderGeometry(text, position, size, alignment, duration, startColor, endColor);
	g_debugRenderGeometries.push_back(message);
}

void DebugAddMessage(std::string const& text, float duration, float size/* = 10.f*/, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/)
{
	DebugRenderGeometry* message = new DebugRenderGeometry(text, duration, size, startColor, endColor);
	g_debugRenderGeometries.push_back(message);
}

bool Command_DebugRenderClear(EventArgs& args)
{
	UNUSED(args);
	DebugRenderSystemShutDown();
	return true;
}

bool Command_DebugRenderToggle(EventArgs& args)
{
	UNUSED(args);
	if (g_debugRenderVisibility)
	{
		g_debugRenderVisibility = false;
	}
	else
	{
		g_debugRenderVisibility = true;
	}
	return true;
}

void DebugRenderScreen(const Camera& camera)
{
	// if the visibility is turned off, do not render
	if (!g_debugRenderVisibility)
	{
		return;
	}

	AABB2 bounds = camera.GetCameraBounds();
	Vec2 start = bounds.GetPointAtUV(Vec2(0.004f, 0.004f));
	Vec2 end = bounds.GetPointAtUV(Vec2(0.996f, 0.996f));
	bounds = AABB2(start, end);

	float screenHeigt = bounds.GetDimensions().y;
	float textLineHeight = screenHeigt / g_theDebugRenderConfig.m_numMessageOnScreen;
	float fontHeight = g_theDebugRenderConfig.m_lineHeightAndTextBoxRatio * textLineHeight;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	std::vector<Vertex_PCU> debugMessageVerts;
	int	  linesLeftToDrawOnScreen = g_theDebugRenderConfig.m_numMessageOnScreen;

	// draw the infinite lines first
	// g_debugRenderGeometriesMutex.lock;
	if (!g_debugRenderGeometries.empty())
	{
		// from the start to the end
		for (int i = 0; i < ((int)g_debugRenderGeometries.size()); ++i)
		{
			DebugRenderGeometry* textLine = g_debugRenderGeometries[i];
			if (textLine->m_type == DebugRenderGeometryType::TEXT) // if the geometry type is text
			{
				if (textLine->m_timer->m_period < 0.f) // if the text is a screen message could show on screen wherever they want
				{
					g_theDebugRenderConfig.m_font->AddVertsForTextInBox2D(debugMessageVerts, textLine->m_text, bounds, 
						textLine->m_textSize, textLine->m_alignment, textLine->m_currentColor, 0.5f, g_theDebugRenderConfig.m_fontAspect);

					// once added to screen, delete this message cause the infinite will be added every frame
					// delete the debug render geometries
					auto iter = g_debugRenderGeometries.begin() + i;
					delete* iter;
					g_debugRenderGeometries.erase(g_debugRenderGeometries.begin() + i);
					--i;
					// move the text box for the next line
					// currentLineBox.Translate(Vec2(0.f, (textLineHeight * -1.f)));
				}
			}
		}
	}

	// get the bounds for first debug message
	// leave the first line on top for screen message
	AABB2 currentLineBox(Vec2(bounds.m_mins.x, (bounds.m_maxs.y - textLineHeight * 2.f)), 
						Vec2(bounds.m_maxs.x, (bounds.m_maxs.y - textLineHeight)));

	// the reset of the screen we are drawing new non-infinite messages
	if (!g_debugRenderGeometries.empty())
	{
		// draw the line from bottom to up
		for (int i = ((int)g_debugRenderGeometries.size() - 1); i >= 0; --i)
		{
			// stop drawing when there is no space
			if (linesLeftToDrawOnScreen == 0)
			{
				break;
			}

			DebugRenderGeometry* textLine = g_debugRenderGeometries[i];
			if (textLine->m_type == DebugRenderGeometryType::TEXT && textLine->m_timer->m_period > 0.f) // not infinite screen message
			{

				g_theDebugRenderConfig.m_font->AddVertsForTextInBox2D(debugMessageVerts, textLine->m_text, currentLineBox, fontHeight,
					Vec2(0.f, 0.5f), textLine->m_currentColor, 0.5f, g_theDebugRenderConfig.m_fontAspect, TextDrawMode::OVERRUN);

				// if the text line display time ends, delete the message
				if (textLine->m_timer->HasPeroidElapsed() || textLine->m_framesLeft == 0) // life ends
				{
					// delete the debug render geometries
					auto iter = g_debugRenderGeometries.begin() + i;
					delete* iter;
					g_debugRenderGeometries.erase(g_debugRenderGeometries.begin() + i);
				}
				else
				{
					if (textLine->m_showOnlyOneFrame)
					{
						--(textLine->m_framesLeft);
					}
				}

				--linesLeftToDrawOnScreen;
				// move the text box for the next line
				currentLineBox.Translate(Vec2(0.f, (textLineHeight * -1.f)));
			}		
		}
	}

	// render messages
	Renderer*& debugRenderer = g_theDebugRenderConfig.m_renderer;
	if ((int)debugMessageVerts.size() == 0)
	{
		return;
	}
	debugRenderer->BindShader(nullptr);
	debugRenderer->BindTexture(&g_theDebugRenderConfig.m_font->GetTexture());
	debugRenderer->SetBlendMode(BlendMode::ALPHA);
	debugRenderer->SetModelConstants();
	debugRenderer->SetDepthMode(DepthMode::DISABLED);
	debugRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	debugRenderer->DrawVertexArray((int)debugMessageVerts.size(), debugMessageVerts.data());
}

void DebugAddWorldPoint(Vec3 const& pos, float radius, float duration, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	DebugRenderGeometry* sphere = new DebugRenderGeometry(mode, duration, startColor, endColor, pos, DebugRenderGeometryType::SPHERE);
	AddVertsForSphere3D(sphere->m_vertices, pos, radius, sphere->m_currentColor);
	
	// add to list for update and render
	g_debugRenderGeometries.push_back(sphere);

}

void DebugAddWorldLine(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	// set new DebugRenderGeometry
	DebugRenderGeometry* raycastLine = new DebugRenderGeometry(mode, duration, startColor, endColor, start, DebugRenderGeometryType::LINE);
	AddVertsForCylinder3D(raycastLine->m_vertices, start, end, radius, startColor);

	// add to list for update and render
	g_debugRenderGeometries.push_back(raycastLine);
}

void DebugAddWorldWireCylinder(Vec3 const& base, Vec3 const& top, float radius, float duration, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	// set new DebugRenderGeometry
	DebugRenderGeometry* cylinder = new DebugRenderGeometry(mode, duration, startColor, endColor, base, DebugRenderGeometryType::CYLINDER);
	cylinder->m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;
	AddVertsForCylinder3D(cylinder->m_vertices, base, top, radius, startColor, AABB2::ZERO_TO_ONE, 12);

	// add to list for update and render
	g_debugRenderGeometries.push_back(cylinder);
}

void DebugAddWorldText(std::string const& text, Mat44 const& transform, float textHeight, float duration, Vec2 const& alignment /*= Vec2(0.5f, 0.5f)*/, 
	Rgba8 const startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	// set new DebugRenderGeometry
	DebugRenderGeometry* textLine = new DebugRenderGeometry(mode, duration, startColor, endColor, transform.GetTranslation3D(), DebugRenderGeometryType::TEXT);
	g_theDebugRenderConfig.m_font->AddVertsForText3DAtOriginXForward(textLine->m_vertices, textHeight, text, textLine->m_currentColor, g_theDebugRenderConfig.m_fontAspect, alignment);
	textLine->m_debugRendermode = mode;
	textLine->m_modelMatrix = transform;
	textLine->m_texture = &g_theDebugRenderConfig.m_font->GetTexture();
	textLine->m_type = DebugRenderGeometryType::TEXT;

	// yaw the billboard 180 degrees to face the player
	Mat44 transformMat = Mat44::CreateZRotationDegrees(180.f);
	TransformVertexArray3D(textLine->m_vertices, transformMat);

	// add to list for update and render
	g_debugRenderGeometries.push_back(textLine);
}

void DebugAddWorldBillboardText(std::string const& text, Vec3 const& origin, float textHeight, Vec2 const& alignment, 
	float duration, BillboardType const& type /*= BillboardType::FULL_CAMER_FACING*/,
	Rgba8 const startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	// set new DebugRenderGeometry
	DebugRenderGeometry* textLine = new DebugRenderGeometry(DebugRenderMode::USE_DEPTH, duration, startColor, endColor, origin, DebugRenderGeometryType::LINE);
	g_theDebugRenderConfig.m_font->AddVertsForText3DAtOriginXForward(textLine->m_vertices, textHeight, text, textLine->m_currentColor, g_theDebugRenderConfig.m_fontAspect, alignment);
	textLine->m_debugRendermode = mode;
	textLine->m_billboardType = type;
	textLine->m_type = DebugRenderGeometryType::BILLBOARD;
	textLine->m_texture = &g_theDebugRenderConfig.m_font->GetTexture();
	textLine->m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;

	// no need to rotate 180, we will update its model matrix every frame to track target
	textLine->m_modelMatrix = g_theDebugRenderConfig.m_camera->GetModelMatrix();
	textLine->m_trackingTargetTransform = g_theDebugRenderConfig.m_camera->GetModelMatrix();

	// add to list for update and render
	g_debugRenderGeometries.push_back(textLine);
}

void DebugAddWorldWireSphere(Vec3 const& center, float radius, float duration, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	DebugRenderGeometry* wireSphere = new DebugRenderGeometry(mode, duration, startColor, endColor, center, DebugRenderGeometryType::SPHERE);
	wireSphere->m_rasterizerMode = RasterizerMode::WIREFRAME_CULL_BACK;
	AddVertsForSphere3D(wireSphere->m_vertices, center, radius, startColor);

	// add to list for update and render
	g_debugRenderGeometries.push_back(wireSphere);
}

void DebugAddWorldArrow(Vec3 const& start, Vec3 const& end, float radius, float duration, Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/,
	DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	// the length of the arrow is unit 1 in the world
	// first construct a cylinder facing x forward
	Vec3 disp = end - start;
	float cylinderFraction = 0.6f;
	Vec3 cylinderEnd = start + disp * cylinderFraction;
	DebugRenderGeometry* cylinder = new DebugRenderGeometry(mode, duration, startColor, endColor, start, DebugRenderGeometryType::CYLINDER);
	AddVertsForCylinder3D(cylinder->m_vertices, start, cylinderEnd, radius, cylinder->m_currentColor);

	// add to list for update and render
	g_debugRenderGeometries.push_back(cylinder);
	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// cone
	DebugRenderGeometry* cone = new DebugRenderGeometry(mode, duration, startColor, endColor, start, DebugRenderGeometryType::CONE);
	AddVertsForCone3D(cone->m_vertices, cylinderEnd, end, radius * 1.5f, cone->m_currentColor);

	// add to list for update and render
 	g_debugRenderGeometries.push_back(cone);
}

void AddRicochetVFX(Vec3 const& impactPos, Vec3 const& reflectNormal, float radius, float maxConeHeight, float duration /*= 0.2f*/, 
	Rgba8 const& startColor /*= Rgba8::WHITE*/, Rgba8 const& endColor /*= Rgba8::WHITE*/, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/)
{
	DebugRenderGeometry* cone = new DebugRenderGeometry(impactPos, reflectNormal, radius, maxConeHeight, duration, startColor, endColor, mode, DebugRenderGeometryType::CHANGINGCONE);
	cone->m_height = maxConeHeight;
	g_debugRenderGeometries.push_back(cone);
}

void DebugAddWorldBasis(Mat44 const& transform, float duration, float arrowRadius, DebugRenderMode mode /*= DebugRenderMode::USE_DEPTH*/, float length /*= 1.f*/)
{
	Vec3 iBasis = transform.GetIBasis3D();
	Vec3 jBasis = transform.GetJBasis3D();
	Vec3 kBasis = transform.GetKBasis3D();
	Vec3 center = transform.GetTranslation3D();
	DebugAddWorldArrow(center, center + (iBasis * length), arrowRadius, duration, Rgba8::RED, Rgba8::RED, mode);
	DebugAddWorldArrow(center, center + (jBasis * length), arrowRadius, duration, Rgba8::GREEN, Rgba8::GREEN, mode);
	DebugAddWorldArrow(center, center + (kBasis * length), arrowRadius, duration, Rgba8::BLUE, Rgba8::BLUE, mode);
}
