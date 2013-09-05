
import os

env = Environment()

env['CPPPATH'] = ['#/external', '#/generated', '#src']
env['LINK'] = 'g++'
env['CC'] = 'gcc'
env['CXX'] = 'g++'
env['LIBS'] = ['GL', 'glfw', 'boost_regex', 'IL', 'OpenCL', 'Xrandr', 'rt']
#env['CCFLAGS'] = ['-O0', '-ggdb']
env['CCFLAGS'] = ['-O3']
env['CXXFLAGS'] = ['-std=c++98']
env['CFLAGS'] = ['-std=c99']
env['LINKFLAGS'] = []

env['CPPDEFINES'] = ['linux']

python27 = 'python2.7'
python3 = 'python3'

env.Command(['#/generated/flextGL.c', '#/generated/flextGL.h'],
            ['#/src/extensions.glprofile', '#/tools/flextGL/flextGLgen.py', 
             Glob('#/tools/flextGL/templates/glfw/*template')],
            python3+' tools/flextGL/flextGLgen.py src/extensions.glprofile -Dgenerated -Tglfw3')
             
env.Command(['#/generated/Config.cpp', '#/generated/Config.h'],
            ['#/src/config.xml', Glob('#/tools/configGen/*')],
            python27+' tools/configGen/configGen.py src/config.xml -Dgenerated -Hh -Ccpp')
             
env.Command(['#/generated/format_map.cpp', '#/generated/format_map.h'],
            Glob('tools/format*'),
            python27+' tools/formatMapGen.py -Dgenerated -Hh -Ccpp')

env.Program('micropolis', Glob('src/*cpp') + 
            ['generated/flextGL.c', 
             'generated/Config.cpp', 
             'generated/format_map.cpp'])
