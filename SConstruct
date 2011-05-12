
env = Environment()
env['CPPPATH'] = ['#/external', '#/generated', '#src']
env['LIBS'] = ['GL', 'glfw', 'boost_regex', 'IL', 'OpenCL']
env['CCFLAGS'] = ['-ggdb']

env.Command(['#/generated/flextGL.c', '#/generated/flextGL.h'],
            ['#/src/extensions.txt'],
            'python2 tools/flextGL/flextGLgen.py src/extensions.txt -Dgenerated -Tglfw')
             
env.Command(['#/generated/Config.cpp', '#/generated/Config.h'],
            ['#/src/config.xml'],
            'python2 tools/configGen/configGen.py src/config.xml -Dgenerated -Hh -Ccpp')
             
env.Command(['#/generated/format_map.cpp', '#/generated/format_map.h'],
            Glob('tools/format*'),
            'python2 tools/formatMapGen.py -Dgenerated -Hh -Ccpp')

env.Program('micropolis', Glob('src/*cpp') + 
            ['generated/flextGL.c', 
             'generated/Config.cpp', 
             'generated/format_map.cpp'])
