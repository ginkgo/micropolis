import pickle

import bpy
from mathutils import *

def create_camera_path(filename, camname='Camera'):

    cam = bpy.data.objects[camname] # Get first camera
    cam.scale = Vector((1,1,1))

    mat = cam.matrix_world

    ####
    
    with open(filename, 'rb') as infile:
        X,Y,T,O = pickle.load(infile)

    mesh = bpy.data.meshes.new(name='CameraPath')
    mesh.from_pydata(O, [(i,i+1) for i in range(len(O)-1)], [])    
    mesh.update()

    obj = bpy.data.objects.new('CameraPath', mesh)
    obj.matrix_world = cam.matrix_world
    
    bpy.context.scene.objects.link(obj)
    obj.select = True

def create_mem_curve(filename, camname='Camera', scale=1.0):
    
    cam = bpy.data.objects[camname] # Get first camera
    cam.scale = Vector((1,1,1))

    mat = cam.matrix_world

    ####

    with open(filename, 'rb') as infile:
        X,Y,T,O = pickle.load(infile)

    verts = []

    mY = max(Y)
    sY = [y/mY * scale for y in Y]
    
    for o,m in zip(O,sY):
        verts.append(o)
        x,y,z = o
        verts.append((x,y+m,z))

    faces = []
    for i in range(len(O)-1):
        j = i*2
        faces.append((j+0,j+2,j+3,j+1))
        
        
    mesh = bpy.data.meshes.new(name='CameraPath')
    mesh.from_pydata(verts, [], faces)    
    mesh.update()

    obj = bpy.data.objects.new('CameraPath', mesh)
    obj.matrix_world = cam.matrix_world
    
    bpy.context.scene.objects.link(obj)
    obj.select = True
    

    
