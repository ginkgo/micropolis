
env = Environment()
env['CPPPATH'] = ['#/external', '#/generated']
env['LIBS'] = ['GL', 'glfw']
env['CCFLAGS'] = ['-ggdb']

env.Command(['#/generated/flextGL.c', '#/generated/flextGL.h'],
            ['#/src/extensions.txt'],
            'python2 tools/flextGL/flextGLgen.py src/extensions.txt -Dgenerated -Tglfw')
             

env.Program('micropolis', Glob('src/*cpp') + ['generated/flextGL.c'])
