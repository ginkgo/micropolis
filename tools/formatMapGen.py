
import sys
from optparse import OptionParser
import os.path

import copy

from Cheetah.Template import Template

script_dir = os.path.dirname(__file__) + os.sep

type_dic = {'BYTE' : '8',
            'UNSIGNED_BYTE' : '8',
            'SHORT' : '16',
            'UNSIGNED_SHORT' : '16',
            'HALF_FLOAT' : '16F',
            'FLOAT' : '32F'}

int_type_dic = {'BYTE' : '8I',
                'UNSIGNED_BYTE' : '8UI',
                'SHORT' : '16I',
                'UNSIGNED_SHORT' : '16UI',
                'INT' : '32I',
                'UNSIGNED_INT' : '32UI'}

format_dic = {'RED' : 'R',
              'RG' : 'RG',
              'RGB' : 'RGB',
              'RGBA' : 'RGBA',
              'BGR' : 'RGB',
              'BGRA' : 'RGBA'}

format_map = {}
int_format_map = {}

for format, iformat in format_dic.iteritems():
    for type_, itype in type_dic.iteritems():
        format_map[(format, type_)] = iformat + itype
    for type_, itype in int_type_dic.iteritems():
        int_format_map[(format, type_)] = iformat + itype

format_map[ ('DEPTH_COMPONENT', 'SHORT') ] = 'DEPTH_COMPONENT16'
format_map[ ('DEPTH_COMPONENT', 'INT') ] = 'DEPTH_COMPONENT32'
format_map[ ('DEPTH_COMPONENT', 'UNSIGNED_SHORT') ] = 'DEPTH_COMPONENT16'
format_map[ ('DEPTH_COMPONENT', 'UNSIGNED_INT') ] = 'DEPTH_COMPONENT32'
format_map[ ('DEPTH_COMPONENT', 'FLOAT') ] = 'DEPTH_COMPONENT32F'

linear_format_map = copy.copy(format_map)

# Replace RGB8 and RGBA8 formats with sRGB version
format_map[ ('RGB'  , 'BYTE'          ) ] = 'SRGB8'
format_map[ ('RGB'  , 'UNSIGNED_BYTE' ) ] = 'SRGB8'
format_map[ ('BGR'  , 'BYTE'          ) ] = 'SRGB8'
format_map[ ('BGR'  , 'UNSIGNED_BYTE' ) ] = 'SRGB8'
format_map[ ('RGBA' , 'BYTE'          ) ] = 'SRGB8_ALPHA8'
format_map[ ('RGBA' , 'UNSIGNED_BYTE' ) ] = 'SRGB8_ALPHA8'
format_map[ ('BGRA' , 'BYTE'          ) ] = 'SRGB8_ALPHA8'
format_map[ ('BGRA' , 'UNSIGNED_BYTE' ) ] = 'SRGB8_ALPHA8'

reverse_map = {}

for (format, type_), iformat in linear_format_map.iteritems():
    reverse_map[iformat] = (format, type_)
for (format, type_), iformat in format_map.iteritems():
    reverse_map[iformat] = (format, type_)
for (format, type_), iformat in int_format_map.iteritems():
    reverse_map[iformat] = (format, type_)

header_content = \
    """ 
#ifndef FORMAT_MAP_H
#define FORMAT_MAP_H

#include <flextGL.h>

GLenum get_internal_format(GLenum format, GLenum type);
GLenum get_linear_format(GLenum format, GLenum type);
GLenum get_internal_format_int(GLenum format, GLenum type);
void get_format_and_type(GLenum internal_format, 
                         GLenum *format, GLenum *type);

#endif
"""


parser = OptionParser("usage: %prog [options]")
parser.add_option("-H", "--header_ext", dest="header_ext",
                  help="file extension of generated header file", metavar="EXT",
                  default="h")
parser.add_option("-C", "--source_ext", dest="source_ext",
                  help="file extension of generated source file", metavar="EXT",
                  default="cpp")
parser.add_option("-D", "--dir", dest="output_dir",
                  help="output directory", metavar="DIR",
                  default=".")

(options, args) = parser.parse_args()


namespace = {'type_dic' : type_dic,
             'int_type_dic' : int_type_dic,
             'format_dic' : format_dic,
             'format_map' : format_map,
             'linear_format_map' : linear_format_map,
             'int_format_map' : int_format_map,
             'reverse_map' : reverse_map}

source_template = Template(open(script_dir+'format_map_template.cc', 'r').read(),
                           namespace)


header_file = open('%s/format_map.%s' % (options.output_dir,
                                 options.header_ext),
                   'w')

source_file = open('%s/format_map.%s' % (options.output_dir,
                                 options.source_ext),
                   'w')


header_file.write(header_content)
source_file.write(str(source_template))
