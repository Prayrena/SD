#include "Engine/core/Image.hpp"
#define STB_IMAGE_IMPLEMENTATION // Exactly one .CPP (this Image.cpp) should #define this before #including stb_image.h
#include "ThirdParty/stb/stb_image.h"
#include "Engine/core/StringUtils.hpp"
#include "Engine/core/ErrorWarningAssert.hpp"

Image::Image(char const* imageFilePath)
	:m_imageFilePath(imageFilePath)
{
	int width;
	int height;
	int numChannel;
	stbi_set_flip_vertically_on_load(1);// We prefer uvTexCoords has origin (0,0) at BOTTOM LEFT
	unsigned char* texelsDataPtr = stbi_load(imageFilePath, &width, &height, &numChannel, 0);
	if (!texelsDataPtr)
	{
		stbi_image_free(texelsDataPtr);
		ERROR_RECOVERABLE(Stringf("failed to load %s", imageFilePath));
	}
	m_dimensions = IntVec2(width, height);

	int numTexel = width * height;
	m_texelRgba8Data.resize(numTexel);
	// push each pixel rgb info into the vector array from bottom left to top right
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int texelIndex = (y * width) + x;
			// get the r pointer for each texel
			unsigned char* texelDataPtr = &texelsDataPtr[texelIndex * numChannel];
			if (numChannel == 3)
			{
				Rgba8 color;
				color.r = texelDataPtr[0];
				color.g = texelDataPtr[1];
				color.b = texelDataPtr[2];
				color.a = 255; // if the image don't have alpha info, we will let all texel has 255 in alpha to keep it visible
				m_texelRgba8Data[texelIndex] = color;
			}
			else if (numChannel == 4)
			{
				Rgba8 color;
				color.r = texelDataPtr[0];
				color.g = texelDataPtr[1];
				color.b = texelDataPtr[2];
				color.a = texelDataPtr[3];
				m_texelRgba8Data[texelIndex] = color;
			}
		}
	}

	// Free the raw image texel data now that we've sent a copy of it down to the GPU to be stored in video memory
	stbi_image_free(texelsDataPtr);
}

Image::Image(IntVec2 size, Rgba8 color)
{
	m_dimensions = size;
	int pixelNum = size.x * size.y;
	for (int i = 0; i < pixelNum; ++i)
	{
		m_texelRgba8Data.push_back(color);
	}
}

std::string const& Image::GetImageFilePath() const
{
	return m_imageFilePath;
}

IntVec2 Image::GetDimensions() const
{
	return m_dimensions;
}

// get texel rgba8 info for the image
const void* Image::GetRawData() const
{
	// int width;
	// int height;
	// int numChannel;
	// unsigned char* texelsDataPtr = stbi_load(m_imageFilePath.c_str(), &width, &height, &numChannel, 0);
	// return texelsDataPtr;
	return m_texelRgba8Data.data();
}

Rgba8 Image::GetTexelColor(IntVec2 const& texelCoords) const
{
	int texelIndex = (texelCoords.y * m_dimensions.x) + texelCoords.x;
	Rgba8 const* texelDataPtr = &m_texelRgba8Data[texelIndex];
	return *texelDataPtr;
}

void Image::SetTexelColor(IntVec2 const& texelCoords, Rgba8 const& newColor)
{
	int texelIndex = (texelCoords.y * m_dimensions.x) + texelCoords.x;
	m_texelRgba8Data[texelIndex] = newColor;
}
