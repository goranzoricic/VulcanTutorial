#pragma once
#include <memory>
#include <map>
#include <string>
#include <array>
#include <vector>
#include <set>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <fstream>
#include <ios>
#include <chrono>

// force the use of Vulkan's depth range (0 to 1) instead of GLM's default OpenGL range (-1 to 1)
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "ThirdParty/stb_image.h"
#include "ThirdParty/tiny_obj_loader.h"