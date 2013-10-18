# micropolis scene format
@0xe2b1a4424e280f7b;

struct Scene {
    meshes  @0 :List(Mesh);
    lights  @1 :List(LightSource);
    objects @2 :List(Object);
    cameras @3 :List(Camera);
}


struct Camera {
    name      @0 :Text;
    transform @1 :Transform;
    near      @2 :Float32;
    far       @3 :Float32;
    fovy      @4 :Float32;
}


struct LightSource {
    name      @0 :Text;
    transform @1 :Transform;
    color     @2 :Vec3;
    intensity @3 :Float32;
    type      @4 :Type;
    
    enum Type {
        point       @0;
        directional @1;
        hemi        @2;
    }
}


struct Object {
    name      @0 :Text;
    transform @1 :Transform;
    meshname  @2 :Text;
    color     @3 :Vec3;
}


struct Mesh {
    name      @0 :Text;
    type      @1 :Type;
    
    positions @2 :List(Float32);

    enum Type {
        bezier @0;
    }
}
    

struct Transform {
    translation @0 :Vec3;
    rotation    @1 :Quaternion;
}


struct Vec3 {
    x @0 :Float32;
    y @1 :Float32;
    z @2 :Float32;
}


struct Quaternion {
    r @0 :Float32;
    i @1 :Float32;
    j @2 :Float32;
    k @3 :Float32;
}