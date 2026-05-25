#pragma once
#include "Engine/Renderer/BitmapFont.hpp"
#include <unordered_map>

class Camera;
class Window;
class Widget;

constexpr int INVALID_BUTTON_INDEX = 999;

struct TextLine
{
	TextLine() = default;
	//TextLine(std::string words, Vec2 const& pos, int num, AABB2 const& bounds = AABB2(), 
	//	Rgba8 fontColor = Rgba8::WHITE, Rgba8 hoverColor = Rgba8::MAGENTA, float lineSpacing = 0.5f, float fontAspect = 0.5f);		
		
	TextLine(std::string words, AABB2 const& bounds, Vec2 const& pos, float fontSize, std::string textName = "undefined");

	TextLine(std::string words, Vec2 const& alignment, float fontSize, AABB2 const& bounds = AABB2(), float rotation = 0.f, Vec2 offset = Vec2(), 
				Rgba8 fontColor = Rgba8::WHITE,  Rgba8 fontHoverColor = Rgba8::WHITE, 
				std::string buttonName = "undefined", int buttonIndex = 0, Rgba8 BGColor = Rgba8::MAGENTA,  Rgba8 hoverColor = Rgba8::MAGENTA, float lineSpacing = 0.5f, float fontAspect = 0.5f);
	~TextLine() = default;

	void Update();

	AABB2		m_normalizedScreenBounds;
	AABB2		m_physicalScreenBounds;

	int			m_numWidgetLines = 0;			// the total number of lines that the Widget can stack up
	float		m_fontSize = 0.f;				// take either fontSize or numWidgetLines for calculation
	float		m_lineSpacing = 0.5f;
	float		m_fontAspect = 0.5f;

	float		m_orientation = 0.f;

	std::string m_content;
	Vec2		m_alignment = Vec2();
	Vec2		m_offset = Vec2();
	Rgba8		m_fontColor = Rgba8::WHITE;
	Rgba8		m_fontHoverColor = Rgba8::WHITE;
	Rgba8		m_BGHoverColor = Rgba8::MAGENTA;
	Rgba8		m_BGColor = Rgba8::MAGENTA;

	void	UpdateButtonClickStatus();
	bool	WasJustClicked();
	bool	WasJustReleased();
	bool	HasPressedOvertime();

	bool	IsMouseHoverOver() const;
	AABB2	GetTextLinePhyscialScreenBounds() const;

	int		m_buttonIndex = INVALID_BUTTON_INDEX;

	bool			m_isButton = false;
	bool			m_clickedLastFrame = false;
	bool			m_clickedThisFrame = false;
	std::string		m_buttonName = "undefined";
	std::string		m_textLineName = "undefined";

	bool			m_hidden = false;

	Widget*			m_widget = nullptr; // the widget that this textline is inside
};

struct ImageBox
{
	ImageBox() = default;
	explicit ImageBox(AABB2 const& box, Texture* texture);
	explicit ImageBox(Vec2 const& centerPos, float widthPercentage, float heightPercentage, Texture* texture, std::string name = "undefined");
	~ImageBox() = default;

	AABB2		m_box;
	Texture*	m_texture = nullptr;

	Vec2		m_imageCenterPos;			// the relative center position to the widget
	float		m_heightPercent = 0.f;		// the percentage that height of the image box cover the widget, keep the aspect of the texture
	float		m_widthPercent = 0.f;		// the percentage that height of the image box cover the widget, keep the aspect of the texture

	std::string	m_imageBoxName = "undefined";
	bool		m_hidden = false;

};

class Widget
{
public:
	Widget(AABB2 const& bounds, Camera* camera, Window* windows, BitmapFont* font, Widget* parentWidget = nullptr,
			bool hasBG = true, Rgba8 BGColor = Rgba8::BLACK, 
		float frameThickness = 0.f, Rgba8 frameColor = Rgba8::WHITE);	
	
	Widget(Vec2 const& dimensionPercentage, float WHRatio, Vec2 const& alignment, Camera* camera, Window* windows, BitmapFont* font, Widget* parentWidget = nullptr,
			bool hasBG = true, Rgba8 BGColor = Rgba8::BLACK,
			float frameThickness = 0.f, Rgba8 frameColor = Rgba8::WHITE);
	~Widget();

	void	Update();
	bool	m_BGIsOn = true;
	Rgba8	m_BGColor = Rgba8::BLACK;
	Rgba8	m_BGColor_hover = Rgba8::WHITE;

	void	UpdateClickStatus();
	bool	WasJustClicked();
	bool	WasJustReleased();
	bool	HasPressedOvertime();

	bool	m_clickedLastFrame = false;
	bool	m_clickedThisFrame = false;

	void	Render() const;

	// hierarchy
	std::vector<Widget*> m_children;
	Widget*				 m_parent;

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// sizes
	// get widget dimension functions
	Vec2	GetWidgetDimensions() const;
	AABB2	GetWidgetPhysicalScreenBounds() const;
	void	UpdateWidgetScale();

	Camera*	m_screenCamera = nullptr;
	Window* m_window = nullptr;

	bool	m_designedByHeight = false;
	bool	m_designedByWidth = false;

	int		m_designedScreenHeight_max = 1000;	// if the height is exceeding this, the widget will not grow
	int		m_designedScreenHeight_min = 500;
	int		m_maxPixelWidth = 1000;
	int		m_minPixelWidth = 500;

	AABB2	m_widgetBounds;			// normalized bounds to the windows

	float	m_widgetHeightPercentage = 0.f;
	float	m_widgetWidthPercentage = 0.f;
	float	m_aspectRatio = 0.f;
	Vec2	m_alignment = Vec2();	// the default is the left bottom of the camera

	//----------------------------------------------------------------------------------------------------------------------------------------------------
	// all the AABB2 are normalized relative to the widget
	void		AddTextLineToWidget(TextLine* text);
	TextLine*	GetTextButtonByName(std::string buttonName);
	TextLine*	GetTextButtonByCurrentButtonIndex();

	void		HideTextAndImageOfThisName(std::string name);
	void		ShowTextAndImageOfThisName(std::string name);
	bool		CheckIfTextOrImageOfThisNameIsShown(std::string name);

	void		ResetButtonIndexWhenOutOfRange();
	void		UpdateButtonIndexByMouse();
	int			m_currentButtonIndex = INVALID_BUTTON_INDEX;

	BitmapFont*						m_font;
	float							m_cellHeight = 1.f;
	std::vector<TextLine*>			m_textBoxes;
	std::vector<ImageBox>			m_imageBoxes;
	float m_scaleByResolution = 1.f;

	bool m_enabled = true;			// whether this widget should be rendered or not

	// frame
	float m_frameThicknessPercentage = 0.f;		// percent of the height
	Rgba8 m_frameColor = Rgba8::WHITE;
};

void RenderWidgetIfIsValidAndEnabled(Widget* widget);

bool DeleteWidget(Widget*& widget); // this delete widget and nullptr the argument