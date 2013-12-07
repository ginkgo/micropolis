/******************************************************************************\
 * This file is part of Micropolis.                                           *
 *                                                                            *
 * Micropolis is free software: you can redistribute it and/or modify         *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * Micropolis is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with Micropolis.  If not, see <http://www.gnu.org/licenses/>.        *
\******************************************************************************/



#ifndef COMMON_H
#define COMMON_H

#define NOMINMAX
#define _USE_MATH_DEFINES

typedef unsigned char byte;

#include <glm/glm.hpp>
#include <glm/ext.hpp>

using glm::vec2;
using glm::vec3;
using glm::vec4;

using glm::ivec2;
using glm::ivec3;
using glm::ivec4;

using glm::uvec2;
using glm::uvec3;
using glm::uvec4;

using glm::mat2;
using glm::mat3;
using glm::mat4;
using glm::mat4x3;

using glm::quat;

#include <vector>
#include <map>
#include <unordered_map>

using std::vector;
using std::map;
using std::unordered_map;

#include <string>
#include <iostream>

using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include <flextGL.h>
#include <GLFW/glfw3.h>

#include "utility.h"

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include <boost/format.hpp>
#include <boost/range/irange.hpp>

using boost::shared_ptr;
using boost::scoped_ptr;
using boost::noncopyable;
using boost::format;
using boost::irange;

template<typename T>
using shared_vector = vector<shared_ptr<T> >;

#endif
