
used_toolchain = 'GCC'
python3 = 'python3'


def setup_env(toolchain, optimization_flags, defines, config):
    
    env = Environment()

    warning_flags = ['-Wall', '-Wextra',
                     '-Wno-ignored-qualifiers',
                     '-Wno-unused-but-set-variable',
                     '-Wno-unused-parameter',
                     '-Wno-unused-variable',
                     '-Wno-unknown-pragmas']

    env['CPPPATH'] = ['#/external', '#/%s/generated' % config, '#src']

    if toolchain=='GCC':
        env['LINK'] = 'g++'
        env['CC'] = 'gcc'
        env['CXX'] = 'g++'
    elif used_toolchain=='Clang':
        env['LINK'] = 'clang++'
        env['CC'] = 'clang'
        env['CXX'] = 'clang++'
    else:
        print('The toolchain \'%s\' is not supported.' % toolchain)
        Exit(1)
    
    env['LIBS'] = ['GL', 'glfw', 'boost_regex', 'IL', 'OpenCL', 'Xrandr', 'rt', 'capnp', 'kj']
    env['CCFLAGS'] = optimization_flags + warning_flags 
    env['CXXFLAGS'] = ['-std=c++11']
    env['CFLAGS'] = ['-std=c99']
    env['LINKFLAGS'] = []

    env['CPPDEFINES'] = defines

    return env

release_env = setup_env(used_toolchain, optimization_flags=['-O3', '-msse4'], defines=['linux', 'NDEBUG'], config='release')
debug_env = setup_env(used_toolchain, optimization_flags=['-O0', '-ggdb'], defines=['linux', 'DEBUG_OPENCL'], config='debug')


SConscript('SConscript', variant_dir='release', duplicate=0, exports={'env':release_env, 'python3':python3, 'config':'release'})
SConscript('SConscript', variant_dir='debug',   duplicate=0, exports={'env':debug_env,   'python3':python3, 'config':'debug'})


