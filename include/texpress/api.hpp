#pragma once

#include <texpress/core/application.hpp>
#include <texpress/core/engine.hpp>
#include <texpress/core/system.hpp>
#include <texpress/core/wallclock.hpp>
#include <texpress/graphics/render_passes/clear_pass.hpp>
#include <texpress/graphics/render_passes/swap_pass.hpp>
#include <texpress/graphics/render_passes/prepare_pass.hpp>
#include <texpress/graphics/render_passes/gui_pass.hpp>
#include <texpress/graphics/render_pass.hpp>
#include <texpress/graphics/renderer.hpp>
#include <texpress/io/file_io.hpp>
#include <texpress/io/image_io.hpp>
#include <texpress/io/regular_grid_io.hpp>
#include <texpress/types/image.hpp>
#include <texpress/types/regular_grid.hpp>

#include <glbinding/gl/gl.h>
#include <glm/glm.hpp>
#include <globjects/base/AbstractStringSource.h>
#include <globjects/base/StaticStringSource.h>
#include <globjects/globjects.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_glfw.h>