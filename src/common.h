
#ifndef COMMON_H
#define COMMON_H

typedef unsigned char byte;

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

#include <vector>
#include <map>

using std::vector;
using std::map;

#include <string>
#include <iostream>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include <flextGL.h>
#define GLFW_NO_GLU
#include <GL/glfw.h>

#include "utility.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>

using boost::shared_ptr;
using boost::scoped_ptr;
using boost::noncopyable;

#endif
