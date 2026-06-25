#pragma once

#ifndef HORIZON_CORE_HPP
#define HORIZON_CORE_HPP

// Third-party foundation libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <gl2d/gl2d.h>

// Engine Resource Mapping Paths 
#ifndef RESOURCES_PATH
#define RESOURCES_PATH "resources/"
#endif

#define FONT_PATH    RESOURCES_PATH "fonts/"
#define ASSETS_PATH  RESOURCES_PATH "assets/"
#define MAPS_PATH    RESOURCES_PATH "maps/"
#define SFX_PATH     RESOURCES_PATH "sfx/"
#define SAVE_DIR     "saves/"

#endif // HORIZON_CORE_HPP