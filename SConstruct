
import os

env = Environment()
env['CPPPATH'] = ['#/external', '#/generated', '#src', 
                  '%s/include' % os.environ['AMDAPPSDKROOT']]
env['LIBS'] = ['GL', 'glfw', 'boost_regex', 'IL', 'OpenCL', 'Xrandr', 'rt']
env['CCFLAGS'] = ['-O2', '-ggdb']
env['CXXFLAGS'] = ['-std=c++98']
env['CFLAGS'] = ['-std=c99']
env['LINKFLAGS'] = []

env['CPPDEFINES'] = ['linux']

python = 'python2.7'

env.Command(['#/generated/flextGL.c', '#/generated/flextGL.h'],
            ['#/src/extensions.glprofile', '#/tools/flextGL/flextGLgen.py', 
             Glob('#/tools/flextGL/templates/glfw/*template')],
            python+' tools/flextGL/flextGLgen.py src/extensions.glprofile -Dgenerated -Tglfw')
             
env.Command(['#/generated/Config.cpp', '#/generated/Config.h'],
            ['#/src/config.xml', Glob('#/tools/configGen/*')],
            python+' tools/configGen/configGen.py src/config.xml -Dgenerated -Hh -Ccpp')
             
env.Command(['#/generated/format_map.cpp', '#/generated/format_map.h'],
            Glob('tools/format*'),
            python+' tools/formatMapGen.py -Dgenerated -Hh -Ccpp')

env.Program('micropolis', Glob('src/*cpp') + 
            ['generated/flextGL.c', 
             'generated/Config.cpp', 
             'generated/format_map.cpp'])
