#ifndef COMMON_H
#define COMMON_H

typedef unsigned char byte;

#define GLM_SWIZZLE_XYZW

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;

using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::mat4x3;

namespace glm {
    using namespace glm::gtc::matrix_transform;
    using namespace glm::gtx::transform;
}

#include <vector>

using std::vector;

#include <string>
#include <iostream>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include <flextGL.h>
#include <GL/glfw.h>

#include "utility.h"

#include "boost/noncopyable.hpp"
#include "boost/unordered_map.hpp"

#endif
