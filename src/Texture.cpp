#include "Texture.h"
#include "Image.h"
#include "Config.h"

namespace GL {

    Tex::UnitManager Tex::_unit_manager;

    Tex::Tex()
    {
        glGenTextures(1, &_texture_name);
        _bound_unit = 0;
        _target = 0;    
    }

    Tex::~Tex()
    {
        assert(_bound_unit == 0);

        glDeleteTextures(1, &_texture_name);
    }


    Texture::Texture(const Image& image,
                     GLenum mag_filter, GLenum min_filter,
                     GLenum wrap_method) :
        Tex(),
        _min_filter(min_filter),
        _mag_filter(mag_filter),
        _wrap_method(wrap_method),
        _dimensions(image.dimensions()),
        _format(image.format()),
        _internal_format(image.internal_format()),
        _width(image.width()), _height(image.height()), _depth(image.depth())
    {
        setup(image.const_data(), image.type(), 0);
    }

    Texture::Texture(int dimensions, int w, int h, int d,
                     GLenum format, GLenum internal_format,
                     GLenum mag_filter, GLenum min_filter,
                     GLenum wrap_method,
                     int samples) :
        Tex(),
        _min_filter(min_filter),
        _mag_filter(mag_filter),
        _wrap_method(wrap_method),
        _dimensions(dimensions),
        _format(format),
        _internal_format(internal_format),
        _width(w), _height(h), _depth(d)
    {
        setup(NULL, GL_FLOAT, samples);
    }
    void Texture::setup(const void* data, GLenum type, int samples)
    { 
        assert(_dimensions >= 1 && _dimensions <= 3);
    
        GLenum targets[] = {GL_TEXTURE_1D, GL_TEXTURE_2D, GL_TEXTURE_3D};
        GLenum wrap_enums[] = {GL_TEXTURE_WRAP_S, 
                               GL_TEXTURE_WRAP_T, 
                               GL_TEXTURE_WRAP_R};

        _target = targets[_dimensions-1];

        if (samples > 0) {
            // Multisample textures don't support mipmapping.
            assert(_min_filter != GL_NEAREST_MIPMAP_NEAREST);
            assert(_min_filter != GL_LINEAR_MIPMAP_NEAREST);
            assert(_min_filter != GL_NEAREST_MIPMAP_LINEAR);
            assert(_min_filter != GL_LINEAR_MIPMAP_LINEAR);
            assert(_dimensions == 2);

            if (_dimensions == 2) {
                _target = GL_TEXTURE_2D_MULTISAMPLE;
            }
        }
        
        bind();

        if (samples == 0) {
            if (FLEXT_EXT_texture_filter_anisotropic) {
                glTexParameterf(_target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                                config.max_anisotropy());
            }
     
            glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, _mag_filter);
            glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, _min_filter);

            for (int i = 0; i < _dimensions; ++i) {
                glTexParameteri(_target, wrap_enums[i], _wrap_method);
            }
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        switch (_dimensions) {
        case 1:
            glTexImage1D(GL_TEXTURE_1D,         // target
                         0,                     // level
                         _internal_format,      // internalFormat
                         _width,                // size
                         0,                     // border
                         _format,               // format
                         type,                  // type
                         data);                 // pixels
            break;
        case 2:
            if (samples < 1) {
                glTexImage2D(GL_TEXTURE_2D,         // target
                             0,                     // level
                             _internal_format,      // internalFormat
                             _width,_height,        // size
                             0,                     // border
                             _format,               // format
                             type,                  // type
                             data);                 // pixels
            } else {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, // target
                                        samples,                   // samples
                                        _internal_format,          // internalFormat
                                        _width,_height,            // size
                                        GL_FALSE);                 // fixedsamplelocations
            }
            break;
        case 3:
            glTexImage3D(GL_TEXTURE_3D,         // target
                         0,                     // level
                         _internal_format,      // internalFormat
                         _width,_height,_depth, // size
                         0,                     // border
                         _format,               // format
                         type,                  // type
                         data);                 // pixels
            break;
        default:
            assert(0);
            // To keep the compiler quiet..
        }    

        generate_mipmaps();

        unbind();
    }


    void Texture::read_back(Image& image) {
        if (_format != image.format()         ||
            _dimensions != image.dimensions() ||
            _width != image.width()           ||
            _height != image.height()         || 
            _depth != image.depth()) {
            cerr << "Texture: "
                 << " Attemted to read image data from incompatible texture."
                 << endl;
        }

        bind();
        glGetTexImage(_target, 0, _format, image.type(), (void*)image.data());
        unbind();
    }

    void Tex::bind()
    {
        if (_bound_unit != 0)
            return;

        _bound_unit = _unit_manager.get_unit();
        glActiveTexture(_bound_unit);
        glBindTexture(_target, _texture_name);
    }

    void Tex::unbind()
    {
        if (_bound_unit == 0)
            return;

#ifdef DEBUG_OPENGL
        glActiveTexture(_bound_unit);
        glBindTexture(_target, 0);
#endif

        _unit_manager.return_unit(_bound_unit);
        
        _bound_unit = 0;    
    }


    GLenum Tex::UnitManager::get_unit()
    {
        if (_unit_list == NULL)
            initialize();

        if (_available_units < 1) {
            cerr << "Texture::UnitManager: "
                 << "Ran out of available texture units!! :<" << endl;
            return 0;
        }

        --_available_units;

        return _unit_list[_available_units];
    }

    void Tex::UnitManager::return_unit(GLenum unit)
    {
        if (_unit_list == NULL)
            initialize();

        if (_available_units >= _max_units) {
            cerr << "Texture::UnitManager: "
                 << "More texture units returned than originally available!! o_O" << endl;
            return;
        }

        _unit_list[_available_units] = unit;

        ++_available_units;
    }

    void Tex::UnitManager::initialize()
    {
        if (_unit_list != NULL)
            return;

        glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &_max_units);
    
        _unit_list = new GLenum[_max_units];
        _available_units = _max_units;

        for (int i = 0; i < _max_units; ++i) {
            _unit_list[i] = GL_TEXTURE0 + i;
        }
    }

    Tex::UnitManager::~UnitManager() {
        if (_unit_list != NULL)
            delete[] _unit_list;
    }

    void Texture::generate_mipmaps()
    {
        assert(_bound_unit != 0);

        if (_min_filter != GL_NEAREST && _min_filter != GL_LINEAR) {
            glGenerateMipmap(_target);
        }    
    }

    TextureBuffer::TextureBuffer(GLuint size, GLenum internal_format):
        Tex(), _size(size)
    {
        _target = GL_TEXTURE_BUFFER;

        glGenBuffers(1, &_buffer);

        glBindBuffer(GL_TEXTURE_BUFFER, _buffer);
        glBufferData(GL_TEXTURE_BUFFER, size, NULL, GL_STREAM_DRAW);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);

        bind();

        glTexBuffer(GL_TEXTURE_BUFFER, internal_format, _buffer);

        unbind();
    }

    TextureBuffer::~TextureBuffer()
    {
        glDeleteBuffers(1, &_buffer);
    }

    void TextureBuffer::load(void* data)
    {
        glBindBuffer(GL_TEXTURE_BUFFER, _buffer);
        glBufferData(GL_TEXTURE_BUFFER, _size, data, GL_STREAM_DRAW);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

}
