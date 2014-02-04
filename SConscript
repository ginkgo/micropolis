Import('env')
Import('python3')
Import('config')

env.Command(['#/%s/generated/flextGL.c' % config, '#/%s/generated/flextGL.h' % config],
            ['#/src/extensions.glprofile', '#/tools/flextGL/flextGLgen.py', '#/tools/flextGL/flext.py',
             Glob('#/tools/flextGL/templates/glfw3/*template')],
            python3+' tools/flextGL/flextGLgen.py src/extensions.glprofile -D%s/generated -Tglfw3' % config)
             
env.Command(['#/%s/generated/Config.cpp' % config, '#/%s/generated/Config.h' % config],
            ['#/src/config.xml', Glob('#/tools/configGen/*')],
            python3+' tools/configGen/configGen.py src/config.xml -D%s/generated -Hh -Ccpp' % config)
             
env.Command(['#/%s/generated/format_map.cpp' % config, '#/%s/generated/format_map.h' % config],
            Glob('tools/format*'),
            python3+' tools/formatMapGen.py -D%s/generated -Hh -Ccpp' % config)

env.Command(['#/%s/generated/mscene.capnp.c++' % config, '#/%s/generated/mscene.capnp.h' % config],
            ['#/src/mscene.capnp'],
            'capnpc src/mscene.capnp --src-prefix=src -oc++:%s/generated' % config)

env.Program('#/micropolis_%s' % config, Glob('src/*cpp') + Glob('src/*/*cpp') + 
            ['#/%s/generated/flextGL.c' % config, 
             '#/%s/generated/Config.cpp' % config, 
             '#/%s/generated/format_map.cpp' % config,
             '#/%s/generated/mscene.capnp.c++' % config])
