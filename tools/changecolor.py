import itertools
import sys

from mathutils import *
from optparse import OptionParser

import capnp
mscene = capnp.load('src/micropolis/mscene.capnp')
#import mscene_capnp as mscene

import math
from random import uniform

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


def parse_args():
    parser = OptionParser(usage='Usage %prog infile outfile')
    options, args = parser.parse_args()

    if len(args) < 2 or len(args) > 2:
        parser.print_help(1)
        exit(1)

    return args[0], args[1], options


if __name__ == '__main__':
    # parse command line arguments
    infile, outfile, options = parse_args()


    with open(infile, 'rb') as f:
        f.seek(0)
        scene = mscene.Scene.read_packed(f)

    scene = scene.as_builder()

    for o in scene.objects:
        set_vec3(o.color, Vector((1,1,1)))
        
    with open(outfile, 'w+b') as f:
        scene.write_packed(f)
