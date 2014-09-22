import bpy
import math

from mathutils import *

import capnp
mscene = capnp.load('src/micropolis/mscene.capnp')

def set_vec3(vec3, v):
    v = v.to_4d()
    vh = v.xyz / v.w
    
    vec3.x = vh.x
    vec3.y = vh.y
    vec3.z = vh.z
    
def set_color(vec3, c):
    vec3.x = c.r
    vec3.y = c.g
    vec3.z = c.b
        

def set_quat(quat, q):
    quat.r = q.w
    quat.i = q.x
    quat.j = q.y
    quat.k = q.z


def set_transform(transform, matrix):
    set_vec3(transform.translation, matrix.to_translation())
    set_quat(transform.rotation, matrix.to_quaternion())

def rad(alpha):
    return alpha / math.pi * 180.0    

def add_camera(cameras, c):
    camera = cameras.add()
        
    camera.name = c.name
    set_transform(camera.transform, c.matrix_world)
    camera.near = c.data.clip_start
    camera.far = c.data.clip_end
    camera.fovy = rad(c.data.angle)

def add_light(lights, l):
    light = lights.add()

    light.name = l.name
    set_transform(light.transform,  l.matrix_world)
    set_color(light.color, l.data.color)
    light.intensity = l.data.energy
    light.type = {'SUN':'directional', 'POINT':'point', 'HEMI':'hemi'}[l.data.type]

def add_object(objects, o):
    obj = objects.add()

    obj.name = o.name
    set_transform(obj.transform, o.matrix_world)
    obj.meshname = o.data.name

    if len(o.data.materials)>0:
        set_color(obj.color, o.data.materials[0].diffuse_color)
    else:
        set_color(obj.color, Color([1,1,1]))

def add_mesh(meshes, m):
    mesh = meshes.add()

    mesh.name = m.name
    mesh.type = 'gregory'

    quads = [p for p in m.polygons if len(p.vertices) == 4]
    
    positions = []
    for p in quads:
        v = []
        s = len(positions)
        for i in range(4):
            v.append(m.vertices[p.vertices[i]].co)
            positions.append(v[i])

        for i in range(4):
            positions.append(2/3 * v[i] + 1/3 * v[(i+1)%4])
        for i in range(4):
            positions.append(2/3 * v[i] + 1/3 * v[(i+3)%4])

        for i in range(4):
            positions.append(2/3 * positions[s+4+i] + 1/3 * positions[s+8+((i+3)%4)])
        for i in range(4):
            positions.append(2/3 * positions[s+4+i] + 1/3 * positions[s+8+((i+3)%4)])

    mesh.init('positions', len(positions)*3)

    for i,pos in enumerate(positions):
        mesh.positions[i*3+0] = pos.x
        mesh.positions[i*3+1] = pos.y
        mesh.positions[i*3+2] = pos.z
    
    

def write_mscene(context, filepath):
    print("running write_mscene...")

    scene = mscene.Scene.new_message()
    cameras = scene.init_resizable_list('cameras')
    meshes = scene.init_resizable_list('meshes')
    lights = scene.init_resizable_list('lights')
    objects = scene.init_resizable_list('objects')

    needed_meshes = set()
    for o in bpy.data.objects:
        if o.type == 'CAMERA':
            add_camera(cameras, o)
        elif o.type == 'LAMP':
            add_light(lights, o)
        elif o.type == 'MESH':
            add_object(objects, o)
            needed_meshes.add(o.data.name)
        else:
            print('Object "%s" of unsupported type %s' % (o.name,o.type))

    for m in bpy.data.meshes:
        if m.name not in needed_meshes:
            continue
        add_mesh(meshes, m)
        
    
    cameras.finish()
    objects.finish()
    lights.finish()
    meshes.finish()

    with open(filepath, 'w') as f:
        scene.write_packed(f)
    
    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportSomeData(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_test.some_data"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export Some Data"

    # ExportHelper mixin class uses this
    filename_ext = ".mscene"

    filter_glob = StringProperty(
            default="*.mscene",
            options={'HIDDEN'},
            )

    # List of operator properties, the attributes will be assigned
    # to the class instance from the operator settings before calling.
    # use_setting = BoolProperty(
    #         name="Example Boolean",
    #         description="Example Tooltip",
    #         default=True,
    #         )

    # type = EnumProperty(
    #         name="Example Enum",
    #         description="Choose between two items",
    #         items=(('OPT_A', "First Option", "Description one"),
    #                ('OPT_B', "Second Option", "Description two")),
    #         default='OPT_A',
    #         )

    # type2 = EnumProperty(
    #         name="Example Enum",
    #         description="Choose between two items",
    #         items=(('OPT_A', "First Option", "Description one"),
    #                ('OPT_B', "Second Option", "Description two")),
    #         default='OPT_A',
    #         )

    def execute(self, context):
        return write_mscene(context, self.filepath)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportSomeData.bl_idname, text="mscene export")


def register():
    bpy.utils.register_class(ExportSomeData)
    bpy.types.INFO_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportSomeData)
    bpy.types.INFO_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_test.some_data('INVOKE_DEFAULT')

