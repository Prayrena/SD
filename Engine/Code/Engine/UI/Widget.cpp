#include "Engine/UI/Widget.hpp"
#include "Engine/core/VertexUtils.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Window.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"

#include <windows.h>

using namespace std;

extern Renderer*	g_theRenderer;
extern InputSystem* g_theInput;

Widget::Widget(AABB2 const& bounds, Camera* camera, Window* windows, BitmapFont* font, Widget* parentWidget /*= nullptr*/, 
			bool hasBG /*= true*/, Rgba8 BGColor /*= Rgba8::BLACK*/, float frameThickness /*= 0.f*/, Rgba8 frameColor /*= Rgba8::WHITE*/)
	: m_widgetBounds(bounds)
	, m_screenCamera(camera)
	, m_window(windows)
	, m_font(font)
	, m_parent(parentWidget)
	, m_BGIsOn(hasBG)
	, m_BGColor(BGColor)
	, m_frameThicknessPercentage(frameThickness)
	, m_frameColor(frameColor)
{

}

Widget::Widget(Vec2 const& dimensionPercentage, float WHRatio, Vec2 const& alignment, 
	Camera* camera, Window* windows, BitmapFont* font, Widget* parentWidget /*= nullptr*/, 
	bool hasBG /*= true*/, Rgba8 BGColor /*= Rgba8::BLACK*/, float frameThickness /*= 0.f*/, Rgba8 frameColor /*= Rgba8::WHITE*/)
	: m_aspectRatio(WHRatio)
	, m_alignment(alignment)
	, m_screenCamera(camera)
	, m_window(windows)
	, m_font(font)
	, m_parent(parentWidget)
	, m_BGIsOn(hasBG)
	, m_BGColor(BGColor)
	, m_frameThicknessPercentage(frameThickness)
	, m_frameColor(frameColor)
{
	float screenRatio = Window::s_theWindow->GetCurrentAspectRatio();
	if (dimensionPercentage.x == 0.f)
	{
		m_widgetHeightPercentage = dimensionPercentage.y;

		m_widgetWidthPercentage = (m_widgetHeightPercentage * m_aspectRatio) / screenRatio;

	}
	else if (dimensionPercentage.y == 0.f)
	{
		m_widgetWidthPercentage = dimensionPercentage.x;
		m_widgetHeightPercentage = (m_widgetWidthPercentage / m_aspectRatio) * screenRatio;
	}
	else
	{
		ERROR_AND_DIE("to define the size of the widget, use either x or y, the other is 0.f");
	}

	// calculate widget bounds
	float XPadding = 1.f - m_widgetWidthPercentage;
	float YPadding = 1.f - m_widgetHeightPercentage;
	m_widgetBounds.m_mins = Vec2(XPadding * alignment.x, YPadding * alignment.y);
	m_widgetBounds.m_maxs = m_widgetBounds.m_mins + Vec2(m_widgetWidthPercentage, m_widgetHeightPercentage);
}

Widget::~Widget()
{

}

void Widget::Update()
{
	// if this widget is not rendered/enabled, no need to update
	if (!m_enabled)
	{
		return;
	}

	UpdateClickStatus();
	UpdateWidgetScale();

	ResetButtonIndexWhenOutOfRange();
	UpdateButtonIndexByMouse();

	// only update the button when this widget is visible
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		m_textBoxes[i]->Update();
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Widget::UpdateClickStatus()
{
	m_clickedLastFrame = m_clickedThisFrame;

	// check if player click in the widget bounds
	Vec2 clickedPos = g_theInput->GetNormalizedCursorPos();
	if (g_theInput->m_keyStates[KEYCODE_LEFT_MOUSE].m_keyPressedThisFrame && g_theInput->IsCursorInThisNormalizedRegion(m_widgetBounds))
	{
		m_clickedThisFrame = true;
	}
	else
	{
		m_clickedThisFrame = false;
	}
}

bool Widget::WasJustClicked()
{
	if (m_clickedLastFrame == false &&
		m_clickedThisFrame == true)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Widget::WasJustReleased()
{
	if (m_clickedLastFrame == true &&
		m_clickedThisFrame == false)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------------------------------------------------------------------------------------------------------
void Widget::Render() const
{
	if (m_enabled)
	{
		std::vector<Vertex_PCU> BGVerts;

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// once have the scale, let's calculate the AABB2 we want to draw on the screen camera

		AABB2 widgetBounds = GetWidgetPhysicalScreenBounds();
		float widgetHeight = widgetBounds.m_maxs.y - widgetBounds.m_mins.y;
		float widgetWidth = widgetBounds.m_maxs.x - widgetBounds.m_mins.x;
		Vec2 widgetBounds_BL = widgetBounds.m_mins;
		Vec2 widgetBounds_TR = widgetBounds.m_maxs;

		//----------------------------------------------------------------------------------------------------------------------------------------------------
		// draw the back ground
		if (m_BGIsOn && m_frameThicknessPercentage != 0.f)
		{
			AddVertsForAABB2D(BGVerts, AABB2(widgetBounds_BL, widgetBounds_TR), m_BGColor);
		}

		// draw the frame (8 tris, because the BG can be transparent)
		if (m_frameThicknessPercentage != 0.f)
		{
			float thickness = widgetHeight * m_frameThicknessPercentage;
			AddVertsForAABB2DFrame(BGVerts, AABB2(widgetBounds_BL, widgetBounds_TR), thickness, m_frameColor);
		}

		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetDepthMode(DepthMode::DISABLED);
		g_theRenderer->SetModelConstants();
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray((int)BGVerts.size(), BGVerts.data());

		// draw the image/texture boxes
		for (int i = 0; i < (int)m_imageBoxes.size(); ++i)
		{
			std::vector<Vertex_PCU> imageVerts;

			ImageBox const& imageBox = m_imageBoxes[i];
			AABB2 relativeBox;
			Vec2 imageBox_BL;
			Vec2 imageBox_TR;

			if (imageBox.m_hidden)
			{
				continue;
			}

			if (imageBox.m_heightPercent == 0.f && imageBox.m_widthPercent == 0.f)	// the user did not define height but relative AABB area
			{
				relativeBox = imageBox.m_box;

				float imageBoxHeight = widgetHeight * (relativeBox.m_maxs.y - relativeBox.m_mins.y);
				float imageBoxWidth = widgetWidth * (relativeBox.m_maxs.x - relativeBox.m_mins.x);

				imageBox_BL = widgetBounds_BL + Vec2(widgetWidth * relativeBox.m_mins.x, widgetHeight * relativeBox.m_mins.y);
				imageBox_TR = imageBox_BL + Vec2(imageBoxWidth, imageBoxHeight);

				AddVertsForAABB2D(imageVerts, AABB2(imageBox_BL, imageBox_TR), Rgba8::WHITE);
			}
			else  // user define the center of the image and the percent its height/width takes
			{
				// get texture aspect ratio
				if (imageBox.m_texture)
				{
					IntVec2 textureDimensions = imageBox.m_texture->GetDimensions();
					float widthHeightAspectRatio = (float)textureDimensions.x / (float)textureDimensions.y;

					// get the image AABB dimensions
					float widthPercentage = 0.f;
					float heightPercentage = 0.f;
					float imageBoxHeight = 0.f;
					float imageBoxWidth = 0.f;
					if (imageBox.m_widthPercent == 0.f)	// user define the height percentage
					{
						imageBoxHeight = widgetHeight * imageBox.m_heightPercent;
						widthPercentage = imageBox.m_heightPercent * widthHeightAspectRatio;
						imageBoxWidth = widgetHeight * widthPercentage;
					}
					else if (imageBox.m_heightPercent == 0.f) // user define the width percentage
					{
						imageBoxWidth = widgetWidth * imageBox.m_widthPercent;
						heightPercentage = imageBox.m_widthPercent / widthHeightAspectRatio;
						imageBoxHeight = widgetWidth * heightPercentage;
					}

					imageBox_BL = widgetBounds_BL + Vec2(widgetWidth * imageBox.m_imageCenterPos.x, widgetHeight * imageBox.m_imageCenterPos.y) - 0.5f * Vec2(imageBoxWidth, imageBoxHeight);
					imageBox_TR = imageBox_BL + Vec2(imageBoxWidth, imageBoxHeight);
				}
				else // this is a color box
				{
					float imageBoxHeight = widgetHeight * imageBox.m_heightPercent;
					float imageBoxWidth = widgetWidth * imageBox.m_widthPercent;

					imageBox_BL = widgetBounds_BL + Vec2(widgetWidth * imageBox.m_imageCenterPos.x, widgetHeight * imageBox.m_imageCenterPos.y) - 0.5f * Vec2(imageBoxWidth, imageBoxHeight);
					imageBox_TR = imageBox_BL + Vec2(imageBoxWidth, imageBoxHeight);
				}
			}

			AddVertsForAABB2D(imageVerts, AABB2(imageBox_BL, imageBox_TR), Rgba8::WHITE);

			g_theRenderer->SetModelConstants(Mat44());
			g_theRenderer->BindTexture(imageBox.m_texture);
			g_theRenderer->DrawVertexArray((int)imageVerts.size(), imageVerts.data());
		}

		// draw the box bounds for the text line
		vector<Vertex_PCU> buttonVerts;
		for (int i = 0; i < (int)m_textBoxes.size(); ++i)
		{
			TextLine const& textLine = *m_textBoxes[i];
			if (textLine.m_hidden)
			{
				continue;
			}

			if (textLine.m_BGHoverColor != Rgba8::MAGENTA && m_BGColor != Rgba8::MAGENTA && textLine.m_isButton)
			{
				AABB2 relativeBox = textLine.m_normalizedScreenBounds;

				float textBoxHeight = widgetHeight * (relativeBox.m_maxs.y - relativeBox.m_mins.y);
				float textBoxWidth = widgetWidth * (relativeBox.m_maxs.x - relativeBox.m_mins.x);

				Vec2 textBox_BL = widgetBounds_BL + Vec2(widgetWidth * relativeBox.m_mins.x, widgetHeight * relativeBox.m_mins.y);
				Vec2 textBox_TR = textBox_BL + Vec2(textBoxWidth, textBoxHeight);

				Rgba8 boxColor;
				if (textLine.m_isButton && (textLine.IsMouseHoverOver() || m_currentButtonIndex == textLine.m_buttonIndex))
				{
					boxColor = textLine.m_BGHoverColor;
				}
				else
				{
					boxColor = textLine.m_BGColor;
				}

				if (textLine.m_orientation != 0.f)
				{
					// if there is already three verts in array
					int startIndex = (int)buttonVerts.size();	// this is 3
					AddVertsForAABB2D(buttonVerts, AABB2(textBox_BL, textBox_TR), boxColor);
					int numVerts = (int)buttonVerts.size() - startIndex;	// this is 6
					TransformVertexArrayOnXY(startIndex, numVerts, buttonVerts.data(), 1.f, (textBox_BL + textBox_TR) * 0.5f, textLine.m_orientation, Vec2(widgetWidth * textLine.m_offset.x, widgetHeight * textLine.m_offset.y)); // start at 3, loop through 3,4,5, ends 6
				}
				else
				{
					AddVertsForAABB2D(buttonVerts, AABB2(textBox_BL, textBox_TR), boxColor);
				}
			}
		}
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray((int)buttonVerts.size(), buttonVerts.data());

		// draw the text boxes
		std::vector<Vertex_PCU> textVerts;
		for (int i = 0; i < (int)m_textBoxes.size(); ++i)
		{
			TextLine const& textLine = *m_textBoxes[i];
			if (textLine.m_hidden)
			{
				continue;
			}

			AABB2 relativeBox = textLine.m_normalizedScreenBounds;

			float textBoxHeight = widgetHeight * (relativeBox.m_maxs.y - relativeBox.m_mins.y);
			float textBoxWidth = widgetWidth * (relativeBox.m_maxs.x - relativeBox.m_mins.x);

			Vec2 textBox_BL = widgetBounds_BL + Vec2(widgetWidth * relativeBox.m_mins.x, widgetHeight * relativeBox.m_mins.y);
			Vec2 textBox_TR = textBox_BL + Vec2(textBoxWidth, textBoxHeight);

			float cellHeight;
			if (textLine.m_numWidgetLines == 0)
			{
				// cellHeight = RangeMapClamped(screenHeight, 500.0f, 1000.0f, 0.5f * 128.0f, 128.0f);
				cellHeight = textLine.m_fontSize * m_scaleByResolution;
			}
			else
			{
				cellHeight = widgetHeight / textLine.m_numWidgetLines;
			}


			// based on whether
			Rgba8 fontColor;
			if ((textLine.m_isButton && textLine.IsMouseHoverOver()) || (m_currentButtonIndex != INVALID_BUTTON_INDEX && m_currentButtonIndex == textLine.m_buttonIndex))
			{
				fontColor = textLine.m_fontHoverColor;
			}
			else
			{
				fontColor = textLine.m_fontColor;
			}

			// add text verts and rotate if necessary
			if (textLine.m_orientation != 0.f)
			{
				int startIndex = (int)textVerts.size();	// this is 3
				m_font->AddVertsForTextInBox2D(textVerts, textLine.m_content, AABB2(textBox_BL, textBox_TR), cellHeight, textLine.m_alignment, fontColor, textLine.m_lineSpacing, textLine.m_fontAspect, TextDrawMode::OVERRUN);
				int numVerts = (int)textVerts.size() - startIndex;	// this is 6
				TransformVertexArrayOnXY(startIndex, numVerts, textVerts.data(), 1.f, (textBox_BL + textBox_TR) * 0.5f, textLine.m_orientation, Vec2(widgetWidth * textLine.m_offset.x, widgetHeight * textLine.m_offset.y)); // start at 3, loop through 3,4,5, ends 6
			}
			else
			{
				m_font->AddVertsForTextInBox2D(textVerts, textLine.m_content, AABB2(textBox_BL, textBox_TR), cellHeight, textLine.m_alignment, fontColor, textLine.m_lineSpacing, textLine.m_fontAspect, TextDrawMode::OVERRUN);
			}
		}



		g_theRenderer->BindTexture(&m_font->GetTexture());
		g_theRenderer->DrawVertexArray((int)textVerts.size(), textVerts.data());


		// todo: ??? how to implement the idea of anchor point?
	}
}

Vec2 Widget::GetWidgetDimensions() const
{
	// we need both screen camera and windows
	if (!m_screenCamera || !m_window)
	{
		ERROR_AND_DIE("missing camera and windows defined for this widget");
	}

	// first let us check if by default, how much resolution the widget is going to take
	Vec2 cameraViewportDimensions = m_screenCamera->GetVeiwportDimensions();

	float widgetHeight;
	float widgetWidth;
	if (m_widgetHeightPercentage != 1.f && m_widgetWidthPercentage != 1.f)
	{
		widgetHeight = cameraViewportDimensions.y * m_widgetHeightPercentage * m_scaleByResolution;
		widgetWidth = cameraViewportDimensions.x * m_widgetWidthPercentage * m_scaleByResolution;
	}
	else
	{
		widgetHeight = cameraViewportDimensions.y;
		widgetWidth = cameraViewportDimensions.x;
	}

	Vec2 dimensions(widgetWidth, widgetHeight);
	return dimensions;
}


AABB2 Widget::GetWidgetPhysicalScreenBounds() const
{
	Vec2 cameraViewportDimensions = m_screenCamera->GetVeiwportDimensions();
	Vec2 widgetDimensions = GetWidgetDimensions();
	float widgetHeight = widgetDimensions.y;
	float widgetWidth = widgetDimensions.x;

	Vec2 cameraViewport_BL = m_screenCamera->GetOrthoBottomLeft();
	Vec2 widgetBounds_BL;
	Vec2 widgetBounds_TR;

	if (m_alignment != Vec2())	// using padding
	{
		float padding_X = (cameraViewportDimensions.x - widgetWidth);
		float padding_Y = (cameraViewportDimensions.y - widgetHeight);
		widgetBounds_BL = cameraViewport_BL + Vec2(padding_X * m_alignment.x, padding_Y * m_alignment.y);
	}
	else    // directly define an area
	{
		widgetHeight = cameraViewportDimensions.y * (m_widgetBounds.m_maxs.y - m_widgetBounds.m_mins.y) * m_scaleByResolution;
		widgetWidth = cameraViewportDimensions.x * (m_widgetBounds.m_maxs.x - m_widgetBounds.m_mins.x) * m_scaleByResolution;

		widgetBounds_BL = cameraViewport_BL + Vec2(cameraViewportDimensions.x * m_widgetBounds.m_mins.x, cameraViewportDimensions.y * m_widgetBounds.m_mins.y);
	}

	widgetBounds_TR = widgetBounds_BL + Vec2(widgetWidth, widgetHeight);

	AABB2 bounds(widgetBounds_BL, widgetBounds_TR);
	return bounds;
}

void Widget::UpdateWidgetScale()
{
	// Fonts and HUD images were created to look best at a screen height of 1000 pixels.
	// If the screen height is less than 1000, apply uniform scaling from 100 % to 50 % as the screen height goes from 1000 to 500 pixels.
	// we need both screen camera and windows
	if (!m_screenCamera || !m_window)
	{
		ERROR_AND_DIE("missing camera and windows defined for this widget");
	}

	// first let us check if by default, how much resolution the widget is going to take
	IntVec2 screenDimension = m_window->GetWindowDimensions();

	// float widgetHeight = (float)screenDimension.y * (m_widgetBounds.m_maxs.y - m_widgetBounds.m_mins.y);
	// float widgetWidth = (float)screenDimension.x * (m_widgetBounds.m_maxs.x - m_widgetBounds.m_mins.x);
	float screenHeight = (float)screenDimension.y;
	// float screenWidth = (float)screenDimension.x;

	if (m_designedByHeight)
	{
		// if screen is way too large, the font may not looks clear
		// screen is way too small, we can't see the words clear, we are going to keep the widget at min size

		m_scaleByResolution = RangeMapClamped(screenHeight, (float)m_designedScreenHeight_min, (float)m_designedScreenHeight_max, 0.5f, 1.0f);
	}
}

void Widget::AddTextLineToWidget(TextLine* text)
{
	m_textBoxes.push_back(text);
	text->m_widget = this;
}

TextLine* Widget::GetTextButtonByName(std::string buttonName)
{
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		if (m_textBoxes[i]->m_buttonName == buttonName)
		{
			return m_textBoxes[i];
		}
	}

	return nullptr;
}

TextLine* Widget::GetTextButtonByCurrentButtonIndex()
{
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		if (m_textBoxes[i]->m_buttonIndex == m_currentButtonIndex && m_currentButtonIndex != INVALID_BUTTON_INDEX)
		{
			return m_textBoxes[i];
		}
	}

	return nullptr;
}

void Widget::HideTextAndImageOfThisName(std::string name)
{
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		if (m_textBoxes[i]->m_textLineName == name)
		{
			m_textBoxes[i]->m_hidden = true;
		}
	}

	for (int i = 0; i < (int)m_imageBoxes.size(); ++i)
	{
		if (m_imageBoxes[i].m_imageBoxName == name)
		{
			m_imageBoxes[i].m_hidden = true;
		}
	}
}

void Widget::ShowTextAndImageOfThisName(std::string name)
{
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		if (m_textBoxes[i]->m_textLineName == name)
		{
			m_textBoxes[i]->m_hidden = false;
		}
	}

	for (int i = 0; i < (int)m_imageBoxes.size(); ++i)
	{
		if (m_imageBoxes[i].m_imageBoxName == name)
		{
			m_imageBoxes[i].m_hidden = false;
		}
	}
}

bool Widget::CheckIfTextOrImageOfThisNameIsShown(std::string name)
{
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		if (m_textBoxes[i]->m_textLineName == name && !m_textBoxes[i]->m_hidden)
		{
			return true;
		}
	}

	for (int i = 0; i < (int)m_imageBoxes.size(); ++i)
	{
		if (m_imageBoxes[i].m_imageBoxName == name && !m_imageBoxes[i].m_hidden)
		{
			return true;
		}
	}

	return false;
}

void Widget::ResetButtonIndexWhenOutOfRange()
{
	int numButtons = 0;
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		if (m_textBoxes[i]->m_isButton)
		{
			++numButtons;
		}
	}

	if (m_currentButtonIndex != INVALID_BUTTON_INDEX)
	{
		if (m_currentButtonIndex >= numButtons)
		{
			m_currentButtonIndex = 0;
		}
		else if (m_currentButtonIndex < 0)
		{
			m_currentButtonIndex = (numButtons - 1);
		}
	}
}

// if the mouse is hover over a button, its index will become the current button index
// it is saying the mouse is overtaking the keyboard
void Widget::UpdateButtonIndexByMouse()
{
	for (int i = 0; i < (int)m_textBoxes.size(); ++i)
	{
		TextLine* button = m_textBoxes[i];
		if (button->m_isButton && button->IsMouseHoverOver())
		{
			m_currentButtonIndex = button->m_buttonIndex;
		}
	}
}

//TextLine::TextLine(std::string words, Vec2 const& pos, int num, AABB2 const& bounds /*= AABB2()*/, 
//			Rgba8 fontColor /*= Rgba8::WHITE*/, Rgba8 hoverColor /*= Rgba8::MAGENTA*/, float lineSpacing /*= 0.5f*/, float fontAspect /*= 0.5f*/)
//	: m_content(words)
//	, m_alignment(pos)
//	, m_normalizedScreenBounds(bounds)
//	, m_numWidgetLines(num)
//	, m_fontColor(fontColor)
//	, m_BGHoverColor(hoverColor)
//	, m_lineSpacing(lineSpacing)
//	, m_fontAspect(fontAspect)
//{
//
//}

TextLine::TextLine(std::string words, Vec2 const& pos, float fontSize, AABB2 const& bounds /*= AABB2()*/, float rotation /*= 0.f*/, Vec2 offset /*= Vec2()*/, 
					Rgba8 fontColor /*= Rgba8::WHITE*/, Rgba8 fontHoverColor /*= Rgba8::WHITE*/, std::string buttonName /*= "undefined"*/, int buttonIndex /*= 0*/, 
					Rgba8 BGColor /*= Rgba8::MAGENTA*/, Rgba8 hoverColor /*= Rgba8::MAGENTA*/, float lineSpacing /*= 0.5f*/, float fontAspect /*= 0.5f*/)
	: m_content(words)
	, m_alignment(pos)
	, m_normalizedScreenBounds(bounds)
	, m_orientation(rotation)
	, m_buttonName(buttonName)
	, m_buttonIndex(buttonIndex)
	, m_offset(offset)
	, m_fontSize(fontSize)
	, m_fontColor(fontColor)
	, m_fontHoverColor(fontHoverColor)
	, m_BGColor(BGColor)
	, m_BGHoverColor(hoverColor)
	, m_lineSpacing(lineSpacing)
	, m_fontAspect(fontAspect)
{
	if (m_buttonName != "undefined")
	{
		m_isButton = true;
	}
}

TextLine::TextLine(std::string words, AABB2 const& bounds, Vec2 const& pos, float fontSize, std::string textName /*= "undefined"*/)
	: m_content(words)
	, m_alignment(pos)
	, m_fontSize(fontSize)
	, m_normalizedScreenBounds(bounds)
	, m_textLineName(textName)
{
	m_isButton = false;
}

void TextLine::Update()
{
	UpdateButtonClickStatus();
}

void TextLine::UpdateButtonClickStatus()
{
	m_clickedLastFrame = m_clickedThisFrame;

	// check if player click in the widget bounds
	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE) && g_theInput->IsCursorInThisPhysicalScreenRegion(GetTextLinePhyscialScreenBounds(), Window::s_theWindow))
	{
		m_clickedThisFrame = true;
	}
	// only if user release mouse in the region, then it will be considered as released
	else if (g_theInput->WasKeyJustReleased(KEYCODE_LEFT_MOUSE) && 
			g_theInput->IsCursorInThisPhysicalScreenRegion(GetTextLinePhyscialScreenBounds(), Window::s_theWindow))
	{
		m_clickedThisFrame = false;
	}
	// if the player clicked the button but move the mouse outside the box - trying to cancel, it will trigger WasJustClicked() or WasJustReleased()
	if (m_clickedLastFrame && !g_theInput->IsCursorInThisPhysicalScreenRegion(GetTextLinePhyscialScreenBounds(), Window::s_theWindow))
	{
		m_clickedThisFrame = false;
		m_clickedLastFrame = false;
	}
}

bool TextLine::WasJustClicked()
{
	if (!m_isButton)
	{
		ERROR_AND_DIE("This is not a button");
	}

	if (m_clickedLastFrame == false &&
		m_clickedThisFrame == true)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool TextLine::WasJustReleased()
{
	if (!m_isButton)
	{
		ERROR_AND_DIE("This is not a button");
	}

	if (m_clickedLastFrame == true &&
		m_clickedThisFrame == false)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool TextLine::IsMouseHoverOver() const
{
	// check if the mouse is in this region
	if (g_theInput->IsCursorInThisPhysicalScreenRegion(GetTextLinePhyscialScreenBounds(), Window::s_theWindow))
	{
		return true;
	}
	else
	{
		return false;
	}
}

AABB2 TextLine::GetTextLinePhyscialScreenBounds() const
{
	AABB2 widgetBounds = m_widget->GetWidgetPhysicalScreenBounds();
	Vec2 widgetDimensions = m_widget->GetWidgetDimensions();

	AABB2 relativeBox = m_normalizedScreenBounds;

	float textBoxHeight = widgetDimensions.y * (relativeBox.m_maxs.y - relativeBox.m_mins.y);
	float textBoxWidth = widgetDimensions.x * (relativeBox.m_maxs.x - relativeBox.m_mins.x);

	Vec2 textBox_BL = widgetBounds.m_mins + Vec2(widgetDimensions.x * relativeBox.m_mins.x, widgetDimensions.y * relativeBox.m_mins.y);
	Vec2 textBox_TR = textBox_BL + Vec2(textBoxWidth, textBoxHeight);

	if (m_orientation != 0.f)
	{
		TransformPointOnXY2D(textBox_BL, 1.f, (textBox_BL + textBox_TR) * 0.5f, m_orientation, Vec2(widgetDimensions.x * m_offset.x, widgetDimensions.y * m_offset.y));
		TransformPointOnXY2D(textBox_TR, 1.f, (textBox_BL + textBox_TR) * 0.5f, m_orientation, Vec2(widgetDimensions.x * m_offset.x, widgetDimensions.y * m_offset.y));
	}

	AABB2 bounds(Vec2(min(textBox_BL.x, textBox_TR.x), min(textBox_BL.y, textBox_TR.y)), Vec2(max(textBox_BL.x, textBox_TR.x), max(textBox_BL.y, textBox_TR.y)));
	return bounds;
}

ImageBox::ImageBox(AABB2 const& box, Texture* texture)
		: m_box(box)
		, m_texture(texture)
{

}

ImageBox::ImageBox(Vec2 const& centerPos, float widthPercentage, float heightPercentage, Texture* texture, std::string name /*= "undefined"*/)
	: m_imageCenterPos(centerPos)
	, m_widthPercent(widthPercentage)
	, m_heightPercent(heightPercentage)
	, m_texture(texture)
	, m_imageBoxName(name)
{

}

void RenderWidgetIfIsValidAndEnabled(Widget* widget)
{
	if (widget)
	{
		if (widget->m_enabled)
		{
			widget->Render();
		}
	}
}

// if we don't use *&, it will just set the copied local pointer in the function to be nullptr
bool DeleteWidget(Widget*& widget)
{
	if (widget)
	{
		delete widget;
		widget = nullptr;
		return true;
	}
	else
	{
		return false;
	}
}
