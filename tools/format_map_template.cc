#raw
#include "format_map.h"
#include <iostream>

#end raw


GLenum get_internal_format(GLenum format, GLenum type)
{
    switch (format) {
#for $format in $format_dic.iterkeys()
    case GL_$format:
        switch(type) {
#for $type_ in $type_dic.iterkeys()
        case GL_$type_:
            return GL_$(format_map[$format,$type_]);            
#end for
        default:
            std::cerr << "get_internal_format(): "
                      << "Could not find type enum "
                      << type << std::endl;
            return 0;
        }
#end for
    default:
        std::cerr << "get_internal_format(): "
                  << "Could not find format enum "
                  << format << std::endl;
        return 0;
    }
}

GLenum get_linear_format(GLenum format, GLenum type)
{
    switch (format) {
#for $format in $format_dic.iterkeys()
    case GL_$format:
        switch(type) {
#for $type_ in $type_dic.iterkeys()
        case GL_$type_:
            return GL_$(linear_format_map[$format,$type_]);            
#end for
        default:
            std::cerr << "get_linear_format(): "
                      << "Could not find type enum "
                      << type << std::endl;
            return 0;
        }
#end for
    default:
        std::cerr << "get_linear_format(): "
                  << "Could not find format enum "
                  << format << std::endl;
        return 0;
    }
}

GLenum get_internal_format_int(GLenum format, GLenum type)
{
    switch (format) {
#for $format in $format_dic.iterkeys()
    case GL_$format:
        switch(type) {
#for $type_ in $int_type_dic.iterkeys()
        case GL_$type_:
            return GL_$(int_format_map[$format,$type_]);            
#end for
        default:
            std::cerr << "get_internal_format_int(): "
                      << "Could not find type enum "
                      << type << std::endl;
            return 0;
        }
#end for
    default:
        std::cerr << "get_internal_format_int(): "
                  << "Could not find format enum "
                  << format << std::endl;
        return 0;
    }
}

void get_format_and_type(GLenum internal_format, 
                         GLenum *format, GLenum *type)
{
    switch(internal_format) {
#for $iformat, ($format, $type_) in $reverse_map.iteritems()
    case GL_$iformat:
        *format = GL_$format; *type = GL_$type_; return;
#end for
    default:
        std::cerr << "get_format_and_type(): "
                  << "Could not find internal_format enum " 
                  << internal_format << std::endl;
        *format = 0; *type = 0; return;
    }
}
