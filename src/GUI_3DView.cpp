#include "GUI.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cfloat>
#include <cmath>

#ifndef GL_CURRENT_PROGRAM
#define GL_CURRENT_PROGRAM 0x8B8D
#endif
#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE 0x809D
#endif

void CStudio_GUI::Draw_3D_Arena(ImVec2 cursorPos, ImVec2 size)
{
	constexpr float Display_Margin = 10.0f;
	constexpr float Robot_Size = 10.0f;
	constexpr float Margin_Size = 5.0f;

	ImDrawList* draw_list = ImGui::GetWindowDrawList();
	const ImVec2 rect_min(
		/*ImGui::GetWindowPos().x + */cursorPos.x - ImGui::GetScrollX(),
		/*ImGui::GetWindowPos().y + */cursorPos.y - ImGui::GetScrollY());
	const ImVec2 rect_max(rect_min.x + size.x, rect_min.y + size.y);

	draw_list->AddRectFilled(rect_min, rect_max, IM_COL32(0, 0, 32, 255));

	if (mArena_Width <= 0.0f || mArena_Height <= 0.0f) {
		return;
	}

	auto get_camera_view_size = [&]() {
		struct Vec3 {
			float x;
			float y;
			float z;
		};

		auto dot = [](Vec3 lhs, Vec3 rhs) {
			return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
			};

		constexpr float Deg_Per_Rad = 57.29577951308232f;
		const float rotation = mArena3D_View_Rotation / Deg_Per_Rad;
		const float elevation = mArena3D_View_Elevation / Deg_Per_Rad;
		const float cos_rotation = std::cos(rotation);
		const float sin_rotation = std::sin(rotation);
		const float cos_elevation = std::cos(elevation);
		const float sin_elevation = std::sin(elevation);
		const Vec3 camera_right{
			cos_rotation,
			-sin_rotation,
			0.0f,
		};
		const Vec3 camera_up{
			-sin_rotation * sin_elevation,
			-cos_rotation * sin_elevation,
			cos_elevation,
		};
		const Vec3 camera_depth{
			-(camera_right.y * camera_up.z - camera_right.z * camera_up.y),
			-(camera_right.z * camera_up.x - camera_right.x * camera_up.z),
			-(camera_right.x * camera_up.y - camera_right.y * camera_up.x),
		};
		const Vec3 scene_center{
			mArena_Width * 0.5f,
			mArena_Height * 0.5f,
			0.0f,
		};

		auto transform_to_camera = [&](Vec3 point) {
			point.x -= scene_center.x;
			point.y -= scene_center.y;
			point.z -= scene_center.z;
			return Vec3{
				dot(camera_right, point),
				dot(camera_up, point),
				dot(camera_depth, point),
			};
			};

		Vec3 view_min{ FLT_MAX, FLT_MAX, FLT_MAX };
		Vec3 view_max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const float x : { -Margin_Size, mArena_Width + Margin_Size }) {
			for (const float y : { -Margin_Size, mArena_Height + Margin_Size }) {
				for (const float z : { 0.0f, Robot_Size }) {
					const Vec3 projected = transform_to_camera(Vec3{ x, y, z });
					view_min.x = std::min(view_min.x, projected.x);
					view_min.y = std::min(view_min.y, projected.y);
					view_max.x = std::max(view_max.x, projected.x);
					view_max.y = std::max(view_max.y, projected.y);
				}
			}
		}

		const float view_padding = 24.0f;
		const float zoom = std::max(0.05f, mArena3D_View_Zoom);
		const float viewport_width = std::max(1.0f, size.x - Display_Margin * 2.0f);
		const float viewport_height = std::max(1.0f, size.y - Display_Margin * 2.0f);
		const float viewport_aspect = viewport_width / viewport_height;
		float view_width = (view_max.x - view_min.x) + view_padding;
		float view_height = (view_max.y - view_min.y) + view_padding;
		if (view_width / view_height > viewport_aspect) {
			view_height = view_width / viewport_aspect;
		}
		else {
			view_width = view_height * viewport_aspect;
		}

		return ImVec2(view_width / zoom, view_height / zoom);
		};

	ImGuiIO& io = ImGui::GetIO();
	const bool is_hovered = ImGui::IsMouseHoveringRect(rect_min, rect_max, true);
	if (is_hovered && io.MouseWheel != 0.0f) {
		const float zoom_factor = std::pow(1.12f, io.MouseWheel);
		mArena3D_View_Zoom = std::clamp(mArena3D_View_Zoom * zoom_factor, 0.2f, 8.0f);
	}

	if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		mArena3D_Is_Rotating = true;
	}
	if (is_hovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
		mArena3D_Is_Panning = true;
	}
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		mArena3D_Is_Rotating = false;
	}
	if (!ImGui::IsMouseDown(ImGuiMouseButton_Right) && !ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
		mArena3D_Is_Panning = false;
	}

	if (mArena3D_Is_Rotating) {
		mArena3D_View_Rotation = std::fmod(mArena3D_View_Rotation - io.MouseDelta.x * 0.35f, 360.0f);
		if (mArena3D_View_Rotation < 0.0f) {
			mArena3D_View_Rotation += 360.0f;
		}
		mArena3D_View_Elevation = std::clamp(mArena3D_View_Elevation + io.MouseDelta.y * 0.25f, 15.0f, 70.0f);
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}
	else if (mArena3D_Is_Panning) {
		const ImVec2 camera_view_size = get_camera_view_size();
		const float viewport_width = std::max(1.0f, size.x - Display_Margin * 2.0f);
		const float viewport_height = std::max(1.0f, size.y - Display_Margin * 2.0f);
		mArena3D_View_Pan.x -= io.MouseDelta.x * camera_view_size.x / viewport_width;
		mArena3D_View_Pan.y += io.MouseDelta.y * camera_view_size.y / viewport_height;
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
	}

	struct TOpenGL_Arena_Render_Data {
		ImVec2 rect_min;
		ImVec2 size;
		float arena_width;
		float arena_height;
		float view_rotation;
		float view_elevation;
		float view_zoom;
		ImVec2 view_pan;
		std::vector<TRobot_Position> robot_positions;
	};

	static TOpenGL_Arena_Render_Data render_data;
	render_data.rect_min = rect_min;
	render_data.size = size;
	render_data.arena_width = mArena_Width;
	render_data.arena_height = mArena_Height;
	render_data.view_rotation = mArena3D_View_Rotation;
	render_data.view_elevation = mArena3D_View_Elevation;
	render_data.view_zoom = mArena3D_View_Zoom;
	render_data.view_pan = mArena3D_View_Pan;
	render_data.robot_positions = mRobot_Positions;

	draw_list->AddCallback([](const ImDrawList*, const ImDrawCmd* command) {
		constexpr float Robot_Size = 10.0f;
		constexpr float Margin_Size = 5.0f;
		constexpr float Deg_Per_Rad = 57.29577951308232f;

		const auto* data = static_cast<const TOpenGL_Arena_Render_Data*>(command->UserCallbackData);
		if (data == nullptr || data->size.x <= 0.0f || data->size.y <= 0.0f) {
			return;
		}

		ImDrawData* draw_data = ImGui::GetDrawData();
		const ImVec2 framebuffer_scale = draw_data != nullptr ? draw_data->FramebufferScale : ImGui::GetIO().DisplayFramebufferScale;
		const ImVec2 display_pos = draw_data != nullptr ? draw_data->DisplayPos : ImVec2(0.0f, 0.0f);
		const ImVec2 display_size = draw_data != nullptr ? draw_data->DisplaySize : ImGui::GetIO().DisplaySize;

		const int viewport_x = static_cast<int>((data->rect_min.x - display_pos.x + Display_Margin) * framebuffer_scale.x);
		const int viewport_y = static_cast<int>((display_size.y - (data->rect_min.y - display_pos.y + data->size.y - Display_Margin)) * framebuffer_scale.y);
		const int viewport_width = std::max(1, static_cast<int>((data->size.x - Display_Margin * 2.0f) * framebuffer_scale.x));
		const int viewport_height = std::max(1, static_cast<int>((data->size.y - Display_Margin * 2.0f) * framebuffer_scale.y));

		GLint previous_viewport[4] = {};
		GLint previous_scissor_box[4] = {};
		GLint previous_matrix_mode = GL_MODELVIEW;
		GLfloat previous_clear_color[4] = {};
		GLfloat previous_line_width = 1.0f;
		const GLboolean previous_depth_test = glIsEnabled(GL_DEPTH_TEST);
		const GLboolean previous_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
		const GLboolean previous_blend = glIsEnabled(GL_BLEND);
		const GLboolean previous_cull_face = glIsEnabled(GL_CULL_FACE);
		const GLboolean previous_texture_2d = glIsEnabled(GL_TEXTURE_2D);
		const GLboolean previous_multisample = glIsEnabled(GL_MULTISAMPLE);
		glGetIntegerv(GL_VIEWPORT, previous_viewport);
		glGetIntegerv(GL_SCISSOR_BOX, previous_scissor_box);
		glGetIntegerv(GL_MATRIX_MODE, &previous_matrix_mode);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, previous_clear_color);
		glGetFloatv(GL_LINE_WIDTH, &previous_line_width);

		using TGlUseProgram = void (APIENTRY*)(GLuint program);
		TGlUseProgram gl_use_program = reinterpret_cast<TGlUseProgram>(glfwGetProcAddress("glUseProgram"));
		GLint previous_program = 0;
		if (gl_use_program != nullptr) {
			glGetIntegerv(GL_CURRENT_PROGRAM, &previous_program);
			gl_use_program(0);
		}

		glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
		glEnable(GL_SCISSOR_TEST);
		glScissor(viewport_x, viewport_y, viewport_width, viewport_height);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glClearColor(0.0f, 0.0f, 0.125f, 1.0f);
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_BLEND);
		glDisable(GL_CULL_FACE);
		glDisable(GL_TEXTURE_2D);

		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		struct Vec3 {
			float x;
			float y;
			float z;
		};

		auto dot = [](Vec3 lhs, Vec3 rhs) {
			return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
			};

		const float rotation = data->view_rotation / Deg_Per_Rad;
		const float elevation = data->view_elevation / Deg_Per_Rad;
		const float cos_rotation = std::cos(rotation);
		const float sin_rotation = std::sin(rotation);
		const float cos_elevation = std::cos(elevation);
		const float sin_elevation = std::sin(elevation);
		const Vec3 camera_right{
			cos_rotation,
			-sin_rotation,
			0.0f,
		};
		const Vec3 camera_up{
			-sin_rotation * sin_elevation,
			-cos_rotation * sin_elevation,
			cos_elevation,
		};
		const Vec3 camera_depth{
			-(camera_right.y * camera_up.z - camera_right.z * camera_up.y),
			-(camera_right.z * camera_up.x - camera_right.x * camera_up.z),
			-(camera_right.x * camera_up.y - camera_right.y * camera_up.x),
		};
		const Vec3 scene_center{
			data->arena_width * 0.5f,
			data->arena_height * 0.5f,
			0.0f,
		};

		auto transform_to_camera = [&](Vec3 point) {
			point.x -= scene_center.x;
			point.y -= scene_center.y;
			point.z -= scene_center.z;
			return Vec3{
				dot(camera_right, point),
				dot(camera_up, point),
				dot(camera_depth, point),
			};
			};

		Vec3 view_min{ FLT_MAX, FLT_MAX, FLT_MAX };
		Vec3 view_max{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for (const float x : { -Margin_Size, data->arena_width + Margin_Size }) {
			for (const float y : { -Margin_Size, data->arena_height + Margin_Size }) {
				for (const float z : { 0.0f, Robot_Size }) {
					const Vec3 projected = transform_to_camera(Vec3{ x, y, z });
					view_min.x = std::min(view_min.x, projected.x);
					view_min.y = std::min(view_min.y, projected.y);
					view_min.z = std::min(view_min.z, projected.z);
					view_max.x = std::max(view_max.x, projected.x);
					view_max.y = std::max(view_max.y, projected.y);
					view_max.z = std::max(view_max.z, projected.z);
				}
			}
		}

		const float view_padding = 24.0f;
		const float zoom = std::max(0.05f, data->view_zoom);
		const float viewport_aspect = static_cast<float>(viewport_width) / static_cast<float>(viewport_height);
		float view_width = (view_max.x - view_min.x) + view_padding;
		float view_height = (view_max.y - view_min.y) + view_padding;
		if (view_width / view_height > viewport_aspect) {
			view_height = view_width / viewport_aspect;
		}
		else {
			view_width = view_height * viewport_aspect;
		}
		view_width /= zoom;
		view_height /= zoom;

		const ImVec2 view_center(
			(view_min.x + view_max.x) * 0.5f + data->view_pan.x,
			(view_min.y + view_max.y) * 0.5f + data->view_pan.y);
		glOrtho(
			view_center.x - view_width * 0.5f,
			view_center.x + view_width * 0.5f,
			view_center.y - view_height * 0.5f,
			view_center.y + view_height * 0.5f,
			view_min.z - 100.0f,
			view_max.z + 100.0f);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		const GLfloat arena_camera[] = {
			camera_right.x, camera_up.x, camera_depth.x, 0.0f,
			camera_right.y, camera_up.y, camera_depth.y, 0.0f,
			camera_right.z, camera_up.z, camera_depth.z, 0.0f,
			-dot(camera_right, scene_center), -dot(camera_up, scene_center), -dot(camera_depth, scene_center), 1.0f,
		};
		glMultMatrixf(arena_camera);

		auto set_color = [](ImVec4 color, float shade) {
			glColor4f(
				std::clamp(color.x * shade, 0.0f, 1.0f),
				std::clamp(color.y * shade, 0.0f, 1.0f),
				std::clamp(color.z * shade, 0.0f, 1.0f),
				color.w);
			};

		auto draw_box = [&](float min_x, float min_y, float min_z, float max_x, float max_y, float max_z, ImVec4 color) {
			glBegin(GL_QUADS);
			set_color(color, 1.15f);
			glVertex3f(min_x, min_y, max_z);
			glVertex3f(max_x, min_y, max_z);
			glVertex3f(max_x, max_y, max_z);
			glVertex3f(min_x, max_y, max_z);

			set_color(color, 0.70f);
			glVertex3f(min_x, min_y, min_z);
			glVertex3f(max_x, min_y, min_z);
			glVertex3f(max_x, min_y, max_z);
			glVertex3f(min_x, min_y, max_z);

			set_color(color, 0.85f);
			glVertex3f(max_x, min_y, min_z);
			glVertex3f(max_x, max_y, min_z);
			glVertex3f(max_x, max_y, max_z);
			glVertex3f(max_x, min_y, max_z);

			set_color(color, 0.95f);
			glVertex3f(max_x, max_y, min_z);
			glVertex3f(min_x, max_y, min_z);
			glVertex3f(min_x, max_y, max_z);
			glVertex3f(max_x, max_y, max_z);

			set_color(color, 0.60f);
			glVertex3f(min_x, max_y, min_z);
			glVertex3f(min_x, min_y, min_z);
			glVertex3f(min_x, min_y, max_z);
			glVertex3f(min_x, max_y, max_z);
			glEnd();

			glColor4f(0.02f, 0.025f, 0.035f, 1.0f);
			glBegin(GL_LINES);
			glVertex3f(min_x, min_y, min_z);
			glVertex3f(max_x, min_y, min_z);
			glVertex3f(max_x, min_y, min_z);
			glVertex3f(max_x, max_y, min_z);
			glVertex3f(max_x, max_y, min_z);
			glVertex3f(min_x, max_y, min_z);
			glVertex3f(min_x, max_y, min_z);
			glVertex3f(min_x, min_y, min_z);

			glVertex3f(min_x, min_y, max_z);
			glVertex3f(max_x, min_y, max_z);
			glVertex3f(max_x, min_y, max_z);
			glVertex3f(max_x, max_y, max_z);
			glVertex3f(max_x, max_y, max_z);
			glVertex3f(min_x, max_y, max_z);
			glVertex3f(min_x, max_y, max_z);
			glVertex3f(min_x, min_y, max_z);

			glVertex3f(min_x, min_y, min_z);
			glVertex3f(min_x, min_y, max_z);
			glVertex3f(max_x, min_y, min_z);
			glVertex3f(max_x, min_y, max_z);
			glVertex3f(max_x, max_y, min_z);
			glVertex3f(max_x, max_y, max_z);
			glVertex3f(min_x, max_y, min_z);
			glVertex3f(min_x, max_y, max_z);
			glEnd();
			};

		glBegin(GL_QUADS);
		glColor4f(0.07f, 0.11f, 0.18f, 1.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(data->arena_width, 0.0f, 0.0f);
		glVertex3f(data->arena_width, data->arena_height, 0.0f);
		glVertex3f(0.0f, data->arena_height, 0.0f);
		glEnd();

		glLineWidth(1.0f);
		glColor4f(0.12f, 0.18f, 0.28f, 1.0f);
		glBegin(GL_LINES);
		for (float x = 25.0f; x < data->arena_width; x += 25.0f) {
			glVertex3f(x, 0.0f, 0.05f);
			glVertex3f(x, data->arena_height, 0.05f);
		}
		for (float y = 25.0f; y < data->arena_height; y += 25.0f) {
			glVertex3f(0.0f, y, 0.05f);
			glVertex3f(data->arena_width, y, 0.05f);
		}
		glEnd();

		glLineWidth(2.0f);
		glColor4f(0.78f, 0.84f, 0.92f, 1.0f);
		glBegin(GL_LINE_LOOP);
		glVertex3f(0.0f, 0.0f, 0.08f);
		glVertex3f(data->arena_width, 0.0f, 0.08f);
		glVertex3f(data->arena_width, data->arena_height, 0.08f);
		glVertex3f(0.0f, data->arena_height, 0.08f);
		glEnd();

		const ImVec4 margin_color(0.62f, 0.66f, 0.72f, 1.0f);
		draw_box(-Margin_Size, -Margin_Size, 0.0f, data->arena_width + Margin_Size, 0.0f, Margin_Size, margin_color);
		draw_box(-Margin_Size, data->arena_height, 0.0f, data->arena_width + Margin_Size, data->arena_height + Margin_Size, Margin_Size, margin_color);
		draw_box(-Margin_Size, 0.0f, 0.0f, 0.0f, data->arena_height, Margin_Size, margin_color);
		draw_box(data->arena_width, 0.0f, 0.0f, data->arena_width + Margin_Size, data->arena_height, Margin_Size, margin_color);

		for (const auto& pos : data->robot_positions) {
			const ImVec4 team_color = gTeam_Colors[pos.team_idx % gTeam_Colors.size()];
			const float half_size = Robot_Size * 0.5f;

			glPushMatrix();
			glTranslatef(pos.x, pos.y, 0.0f);
			glRotatef(pos.orientation * Deg_Per_Rad, 0.0f, 0.0f, 1.0f);
			draw_box(-half_size, -half_size, 0.0f, half_size, half_size, Robot_Size, team_color);
			glPopMatrix();
		}

		glLineWidth(previous_line_width);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
		glMatrixMode(previous_matrix_mode);

		if (gl_use_program != nullptr) {
			gl_use_program(static_cast<GLuint>(previous_program));
		}
		glClearColor(previous_clear_color[0], previous_clear_color[1], previous_clear_color[2], previous_clear_color[3]);
		glViewport(previous_viewport[0], previous_viewport[1], previous_viewport[2], previous_viewport[3]);
		glScissor(previous_scissor_box[0], previous_scissor_box[1], previous_scissor_box[2], previous_scissor_box[3]);
		previous_depth_test ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
		previous_scissor_test ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
		previous_blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
		previous_cull_face ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
		previous_texture_2d ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
		previous_multisample ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
		}, &render_data);
	draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

	draw_list->AddRect(rect_min, rect_max, IM_COL32(0, 0, 128, 180), 0.0f, 0, 2.0f);
}
