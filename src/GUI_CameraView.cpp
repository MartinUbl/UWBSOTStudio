#include "GUI.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

void CStudio_GUI::Draw_Camera_View(ImVec2 startPosition, ImVec2 canvasSize) {
	if (canvasSize.x <= 0.0f || canvasSize.y <= 0.0f)
		return;

	ImDrawList* drawList = ImGui::GetWindowDrawList();

	const ImVec2 canvasEnd(
		startPosition.x + canvasSize.x,
		startPosition.y + canvasSize.y
	);

	// Optional background.
	drawList->AddRectFilled(
		startPosition,
		canvasEnd,
		IM_COL32(0, 0, 32, 255)
	);

	drawList->AddRect(startPosition, canvasEnd, IM_COL32(0, 0, 128, 180), 0.0f, 0, 2.0f);

	if (mCamera_Texture.texture_id == 0 ||
		mCamera_Texture.width <= 0 ||
		mCamera_Texture.height <= 0)
	{
		return;
	}

	const float imageAspect =
		static_cast<float>(mCamera_Texture.width) /
		static_cast<float>(mCamera_Texture.height);

	const float canvasAspect =
		canvasSize.x / canvasSize.y;

	ImVec2 imageSize;

	if (imageAspect > canvasAspect)
	{
		imageSize.x = canvasSize.x;
		imageSize.y = canvasSize.x / imageAspect;
	}
	else
	{
		imageSize.y = canvasSize.y;
		imageSize.x = canvasSize.y * imageAspect;
	}

	const ImVec2 imageStart(
		startPosition.x + (canvasSize.x - imageSize.x) * 0.5f,
		startPosition.y + (canvasSize.y - imageSize.y) * 0.5f
	);

	const ImVec2 imageEnd(
		imageStart.x + imageSize.x,
		imageStart.y + imageSize.y
	);

	drawList->AddImage(
		static_cast<ImTextureID>(
			static_cast<intptr_t>(mCamera_Texture.texture_id)
			),
		imageStart,
		imageEnd,
		ImVec2(0.0f, 0.0f),
		ImVec2(1.0f, 1.0f),
		IM_COL32(255, 255, 255, 255)
	);
}

bool CStudio_GUI::Update_Camera_Texture(const void* rgbaPixels, int width, int height) {
	if (rgbaPixels == nullptr || width <= 0 || height <= 0)
		return false;

	if (mCamera_Texture.texture_id == 0)
	{
		glGenTextures(1, &mCamera_Texture.texture_id);

		glBindTexture(GL_TEXTURE_2D, mCamera_Texture.texture_id);

		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_MIN_FILTER,
			GL_LINEAR
		);

		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_MAG_FILTER,
			GL_LINEAR
		);

		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_S,
			GL_CLAMP_TO_EDGE
		);

		glTexParameteri(
			GL_TEXTURE_2D,
			GL_TEXTURE_WRAP_T,
			GL_CLAMP_TO_EDGE
		);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, mCamera_Texture.texture_id);
	}

	// RGBA8 has four bytes per pixel, so alignment 4 would normally work.
	// Alignment 1 is safe for all row widths.
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (mCamera_Texture.width != width || mCamera_Texture.height != height)
	{
		mCamera_Texture.width = width;
		mCamera_Texture.height = height;

		// Allocate or reallocate storage when dimensions change.
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			width,
			height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			rgbaPixels
		);
	}
	else
	{
		// Reuse existing texture storage for normal video frames.
		glTexSubImage2D(
			GL_TEXTURE_2D,
			0,
			0,
			0,
			width,
			height,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			rgbaPixels
		);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	return true;
}
