#include "Engine/Math/IntVec2.hpp"
#include "Engine/core/Rgba8.hpp"
#include <string>
#include <Vector>

class Image
{
	friend class Renderer;

public:
	Image(char const* imageFilePath);
	~Image() = default;
	Image(IntVec2 size, Rgba8 color);	// create an image with all pixel of an image of the size into the same specific color

	std::string const&	GetImageFilePath() const;
	IntVec2				GetDimensions() const;
	const void*			GetRawData() const;
	Rgba8				GetTexelColor(IntVec2 const& texelCoords) const;
	void				SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor);

	std::vector< Rgba8 >		m_texelRgba8Data;  // or Rgba8* m_rgbaTexels = nullptr; if you prefer new[] and delete[]

private:
	std::string					m_imageFilePath;
	IntVec2						m_dimensions = IntVec2(0, 0);
};

