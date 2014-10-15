import bpy
import math
from math import *
import bmesh

from collections import deque
from mathutils import *

import capnp
mscene = capnp.load('src/micropolis/mscene.capnp')



def vsum(vs):
    s = Vector([0,0,0])
    for v in vs:
        s += v
    return s



def face_center(f):
    return vsum((v.co for v in f.verts)) / len(f.verts)



def edge_center(e):
    return (e.verts[0].co + e.verts[1].co)/2



def edge_between(v1,v2):
    for e in v1.link_edges:
        if e.other_vert(v1) == v2:
            return e
    
    return None



# def other_face(e,f):
#     return [x for x in e.link_faces if x != f][0]



def rotate_next(w):
    v,e,f = w

    if f == None: return None
        
    try:
        #find edge opposite to this one
        en = [x for x in f.edges if x != e and v in x.verts][0]
        
        #find face on the other side of the new edge
        fn = [x for x in en.link_faces if x != f][0] if len(en.link_faces) >= 2 else None

        return v,en,fn
        
    except:
        return None

    

def rotate_prev(w):
    v,e,f = w
    
    if f == None: return None

    try:
        #find face on the other side of e
        fn = [x for x in e.link_faces if x != f][0] if len(e.link_faces) >= 2 else None
        
        if fn == None: return None
        
        #find edge opposite to this one
        en = [x for x in fn.edges if x != e and v in x.verts][0]

        return v,en,fn
        
    except:
        return None



def has_next(w):
    return w and rotate_next(w) != None



# def has_prev(w):
#     return w and rotate_prev(w) != None


# def rewind(w):
#     _,e,_ = w

#     ww = w
#     w = rotate_prev(w)
    
#     while w and has_prev(w) and w[0] != e:
#         ww = w
#         w = rotate_prev(w)

#     return ww


def neighborhood(f,v,c):
    w = (c, edge_between(v,c), f)

    #w = rewind(w)
    first_edge = w[1]

    E = []
    F = []
    
    i=0
    vc = -1
    
    while True:
        if w[1].other_vert(c)==v:  vc=i

        if w and w[1]: E.append(w[1])
        if w and w[2]: F.append(w[2])
                
        w = rotate_next(w)
        i+=1

        if not has_next(w) or w[1] == first_edge: break

        
    #if w and w[1]: E.append(w[1])
    #if w and w[2]: F.append(w[2])
        
    return F,E,[i-vc for i in range(len(E))]



def calc_r(f,v,c):
    w = (c, edge_between(v,c), f)

    fn = f
    _,ep,fp = rotate_prev(w)
    _,en,_ = rotate_next(w)

    cp,cn = face_center(fp),face_center(fn)
    mp,mn = edge_center(ep),edge_center(en)

    return 1/3*(mn-mp)+2/3*(cn-cp)
    
    

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

        
def make_patch(f):
    if len(f.verts) != 4: return []

    p = []
    
    for v in f.verts:
        n = len(v.link_edges)

        m = list((edge_center(e) for e in v.link_edges))
        c = list((face_center(ff) for ff in v.link_faces))

        p.append(((n-3)/(n+5))*v.co + (4/(n*(n+5))) * (vsum(m)+vsum(c)))
        #p.append(v.co)

    em = []
    ep = []
        
    for i in range(4):
        c = f.verts[i]
        v = f.verts[(i+1)%4]

        n = len(c.link_edges)
        
        sigma = pow(4+pow(cos(pi/n),2), -0.5)
        lbda = 1/16*(5+cos(2*pi/n)+cos(pi/n)*sqrt(18+2*cos(2*pi/n)))
        
        F,E,I = neighborhood(f,v,c)

        M = [edge_center(e) for e in E]
        C = [face_center(f) for f in F]

        q =  2/n * vsum([(1-sigma*cos(pi/n))*cos(2*pi*i/n)*mi for i,mi in zip(I,M)])
        q += 2/n * vsum([2*sigma*cos((2*pi*i+pi)/n)*ci for i,ci in zip(I,C)])

        ep.append( p[i] + 2/3 * lbda * q )

        v = f.verts[(i+3)%4]
        F,E,I = neighborhood(f,v,c)

        M = [edge_center(e) for e in E]
        C = [face_center(f) for f in F]
        
        q =  2/n * vsum([(1-sigma*cos(pi/n))*cos(2*pi*i/n)*mi for i,mi in zip(I,M)])
        q += 2/n * vsum([2*sigma*cos((2*pi*i+pi)/n)*ci for i,ci in zip(I,C)])

        em.append( p[i] + 2/3 * lbda * q)

        
    fm = [None]*4
    fp = [None]*4
    
    for i in range(4):
        j = (i+1)%4
        d = 3 # We always handle quads

        c0 = cos(2*pi/(len(f.verts[i].link_faces)+0))
        c1 = cos(2*pi/(len(f.verts[j].link_faces)+0))

        r0 = calc_r(f,f.verts[j],f.verts[i])
        r1 = calc_r(f,f.verts[i],f.verts[j])

        #r0,r1 = Vector(),Vector()
        
        fp[i] = 1/d*(c1*p[i] + (d-2*c0-c1)*ep[i] + 2*c0*em[j] + r0)
        fm[j] = 1/d*(c0*p[j] + (d-2*c1-c0)*em[j] + 2*c1*ep[i] + r1)

        #fm[i] = fp[i]
        
    return p + ep + em + fp + fm
            
            
            
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
    camera.fovy = rad(c.data.angle_y)

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

# def find_edges(v,f):
#     l = [j for j in f.loops if j.vert == v][0]

#     return l.link_loop_prev.edge, l.edge

# def edge_point_pos(e):
#     return vsum([v.co for v in e.verts])*1/4+vsum([face_center(g) for g in e.link_faces])*1/4

# def local_subdivide(f):
#     bm = bmesh.new()

#     c = bm.verts.new(face_center(f))
#     e = [bm.verts.new(edge_point_pos(e)) for e in f.edges]
    
#     ed = dict([(f.edges[i], e[i]) for i in range(3)])
#     v = []

#     for x in f.verts:
#         for ex in x.link_edges:
#             if ex not in ed:
#                 ed[ex] = bm.verts.new(edge_point_pos(ex))
                
#         n = len(x.link_edges)

#         co = vsum([face_center(f) for f in x.link_faces])/(n*n) + 2*vsum([ed[e].co for e in x.link_edges])/(n*n) + (n-3)*x.co/n
#         #co = vsum([face_center(f) for f in x.link_faces])/(n*n) + vsum([ed[e].co for e in x.link_edges])/(n*n) + (n-2)*x.co/n
#         v.append(bm.verts.new(co))

#     faces = []


#     for i in range(3):
#         j = (i+2)%3

#         faces.append(bm.faces.new([v[i],e[i],c,e[j]]))

#     fd = {f:c}

#     # for f in [other_face(e) for e in f.edges]:
#     #     f.append(bm.verts.new(face_center(f)))
#     #     fd[f] = f[-1]
        

#     for bv,x in zip(v,f.verts):
#         for g in x.link_faces:
#             if g == f:
#                 continue
#             if g not in fd:
#                 fd[g] = bm.verts.new(face_center(g))

#             bg = fd[g]

#             er,el = find_edges(x,g)
#             be1,be2 = ed[el],ed[er]
            
#             bm.faces.new([bv,be1,bg,be2])
    
#     return bm, faces

def is_border_face(f):
    for v in f.verts:
        if any([len(e.link_faces)!=2 for e in v.link_edges]):
            return True

    return False;

def add_mesh(meshes, m):
    mesh = meshes.add()

    mesh.name = m.name
    mesh.type = 'gregory'

    bm = bmesh.new()
    bm.from_mesh(m)

    positions = []

    for f in bm.faces:
        fs = [f]
        
        if is_border_face(f):
            continue
        if len(f.loops) != 4:
            continue # Throw away
            #_,fs = local_subdivide(f)

        for g in fs:
            P = make_patch(g)
            positions += P

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
            print('Object "%s" is of unsupported type %s' % (o.name,o.type))

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


def main():
    register()

    # test call
    bpy.ops.export_test.some_data('INVOKE_DEFAULT')

if __name__ == "__main__":
    main()
