import itertools
import sys

from mathutils import *
from optparse import OptionParser

import capnp
mscene = capnp.load('src/micropolis/mscene.capnp')
#import mscene_capnp as mscene

import math
from random import uniform


def lookat(eye, at, up):
    e,l,U = (Vector(v) for v in (eye,at,up))

    f = (l-e).normalized()
    u_ = U.normalized()

    s = f.cross(u_)
    u = s.cross(f)

    M = Matrix(((s.x,s.y,s.z,0), (u.x,u.y,u.z,0), (-f.x,-f.y,-f.z,0), (0,0,0,1)))
    T = Matrix(((1,0,0,-e.x), (0,1,0,-e.y), (0,0,1,-e.z), (0,0,0,1)))

    L = M * T
    return L.inverted()


def set_vec3(vec3, v):
    v = v.to_4d()
    vh = v.xyz / v.w
    
    vec3.x = vh.x
    vec3.y = vh.y
    vec3.z = vh.z
        

def set_quat(quat, q):
    quat.r = q.w
    quat.i = q.x
    quat.j = q.y
    quat.k = q.z


def set_transform(transform, matrix):
    set_vec3(transform.translation, matrix.to_translation())
    set_quat(transform.rotation, matrix.to_quaternion())


def add_camera(cameras, name, eye, at, up, near, far, fovy):
    camera = cameras.add()

    camera.name = name
    set_transform(camera.transform, lookat(eye,at,up))
    camera.near = near
    camera.far = far
    camera.fovy = fovy


def add_directional_light(lights, name, color, intensity, direction):
    light = lights.add()

    light.name = name
    set_transform(light.transform, lookat((0,0,0), -Vector(direction), (0,1,0)))
    set_vec3(light.color, Vector(color))
    light.intensity = intensity
    light.type = 'directional'

    
def add_object(objects, name, transform, meshname, color):
    obj = objects.add()

    obj.name = name
    set_transform(obj.transform, transform)
    obj.meshname = meshname
    set_vec3(obj.color, Vector(color))


def add_bezier_mesh(meshes, name, positions):
    mesh = meshes.add()

    mesh.name = name
    mesh.type = 'bezier'

    mesh.init('positions', len(positions))

    for i in range(len(positions)):
        mesh.positions[i] = positions[i]
    

def parse_bez (filename, options):
    patchids = []
    vertices = []
    with open(filename) as f:
        patchcnt = int(f.readline())
        for i in range(patchcnt):
            patchids.append([int(s) for s in f.readline().split(',')])
        vertcnt = int(f.readline())
        for i in range(vertcnt):
            vertices.append([float(s) for s in f.readline().split(',')])
    patches = []
    for pid in patchids:
        patch = [vertices[i-1] for i in pid]

        if options.flip:
            patch = [patch[i] for i in [0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15]]
        
        patches.append(patch)

    return list(itertools.chain(*itertools.chain(*patches)))


def parse_args():
    parser = OptionParser(usage='Usage %prog [options] infile outfile')
    parser.add_option('-f', '--flip', dest='flip', action='store_true', default=False)
    parser.add_option('-m', '--multi', dest='multi', type=int, default=0)
    options, args = parser.parse_args()

    if len(args) < 2 or len(args) > 2:
        parser.print_help(1)
        exit(1)

    return args[0], args[1], options


if __name__ == '__main__':
    # parse command line arguments
    infile, outfile, options = parse_args()

    # parse bez data
    patchdata = parse_bez(infile, options)

    # setup scene message
    scene = mscene.Scene.new_message()

    cameras = scene.init_resizable_list('cameras')
    meshes = scene.init_resizable_list('meshes')
    lights = scene.init_resizable_list('lights')
    objects = scene.init_resizable_list('objects')
    
    add_camera(cameras, 'Camera', (25,25,3), (0,0,0), (0,0,1), 0.01, 1000.0, 60.0)
    add_directional_light(lights, 'Light', Color((1,1,1)), 1.0, (0.1,1,0.1))
    add_bezier_mesh(meshes, 'Mesh', patchdata)

    if options.multi == 0:
        add_object(objects, 'Object', Matrix.Identity(4), 'Mesh', Color((1.0,0.1, 0.03)))
    else:
        for i in range(options.multi):
            rcolor = Color()
            rcolor.hsv = uniform(0,1), uniform(0.7,1),1

            rmatrix = Matrix.Identity(3)
            rmatrix.rotate(Euler((uniform(0,math.pi*2), uniform(0,math.pi*2), uniform(0,math.pi*2)), 'XYZ'))
            rmatrix.resize_4x4()
            rmatrix.translation = uniform(-10,10), uniform(-10,10), uniform(-10,10)
            
            add_object(objects, 'Object-%d' % i, rmatrix, 'Mesh', rcolor)
    
    cameras.finish()
    objects.finish()
    lights.finish()
    meshes.finish()

    with open(outfile, 'w') as f:
        scene.write_packed(f)
