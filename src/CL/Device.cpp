#include "Device.h"

#include "Config.h"
#include "Exception.h"

#include <CL/cl_gl.h>
#include <fstream>

#include <signal.h>

#ifdef linux
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
#else
#error TODO
#endif


namespace
{
    void get_opencl_device(int& platform_index, int& device_index,
                           cl_platform_id &platform, cl_device_id &device);
    cl_context create_context_with_GL(cl_platform_id platform, cl_device_id device);
    cl_context create_context_without_GL(cl_platform_id platform, cl_device_id device);
    bool is_GPU_device(cl_device_id device);
    template <typename T> void print_device_param(cl_device_id device,
                                                  cl_device_info param_enum,
                                                  string param_name, 
                                                  const string& indent);
    void print_device_workgroup_size(cl_device_id device, const string& indent);
    void print_device_queue_properties(cl_device_id device, const string& indent);

    void CL_CALLBACK opencl_error_callback(const char* errinfo, const void* private_info, size_t cb, void* user_data);
                               
}



CL::Device::Device(int platform_index, int device_index)
    : _id_count(0)
    , _dump_trace(false)
{
    cl_platform_id platform;

    get_opencl_device(platform_index, device_index, platform, _device);

    if (!config.disable_buffer_sharing() && is_GPU_device(_device)) {
        try {
            _context = create_context_with_GL(platform, _device);
            _share_gl = true;
        } catch (Exception& e) {
            _context = create_context_without_GL(platform, _device);
            _share_gl = false;
        }
    } else {
        _context = create_context_without_GL(platform, _device);
        _share_gl = false;
            
    }

    if (config.verbosity_level() >= 1) {
        print_info();
    }
        
    if(config.verbosity_level() >= 1 || !share_gl()) {
        cout << endl;
        cout << "Device is" << (share_gl() ? " " : " NOT ") << "shared." << endl << endl;
    }

}

CL::Device::~Device()
{
    clReleaseContext(_context);
}                                               \


#define PRINT_CL_DEVICE_INFO(type, name)                    \
    print_device_param<type>(_device, name, #name, indent)

void CL::Device::print_info()
{
    const string indent = "    ";
    cout << endl << indent << "OpenCL Device:" << endl;
    PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_NAME);
    PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_VERSION);
    PRINT_CL_DEVICE_INFO(char[1000], CL_DRIVER_VERSION);
    if (config.verbosity_level() > 0) {
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_ADDRESS_BITS);
        PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_AVAILABLE);
        PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_COMPILER_AVAILABLE);
        PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_ENDIAN_LITTLE);
        PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_ERROR_CORRECTION_SUPPORT);
        PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE);
        PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_GLOBAL_MEM_SIZE);
        PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_HOST_UNIFIED_MEMORY);
        PRINT_CL_DEVICE_INFO(cl_bool, CL_DEVICE_IMAGE_SUPPORT);
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE2D_MAX_WIDTH);
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE2D_MAX_HEIGHT); 
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE3D_MAX_WIDTH);
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE3D_MAX_HEIGHT); 
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_IMAGE3D_MAX_DEPTH);
        PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_LOCAL_MEM_SIZE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_CLOCK_FREQUENCY);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_COMPUTE_UNITS);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_CONSTANT_ARGS);
        PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
        PRINT_CL_DEVICE_INFO(cl_ulong, CL_DEVICE_MAX_MEM_ALLOC_SIZE);
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_MAX_PARAMETER_SIZE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_READ_IMAGE_ARGS);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_SAMPLERS);
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_MAX_WORK_GROUP_SIZE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS);
        print_device_workgroup_size(_device, indent);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MAX_WRITE_IMAGE_ARGS);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MEM_BASE_ADDR_ALIGN);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF);
        PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_OPENCL_C_VERSION);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE);
        PRINT_CL_DEVICE_INFO(cl_uint, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF);
        PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_PROFILE);
        PRINT_CL_DEVICE_INFO(size_t, CL_DEVICE_PROFILING_TIMER_RESOLUTION);
        print_device_queue_properties(_device, indent);
        PRINT_CL_DEVICE_INFO(char[1000], CL_DEVICE_EXTENSIONS);
    }
}



CL::Event CL::Device::insert_event(const string& name, const string& queue_name, cl_event event, const CL::Event& dependencies)
{
    int id = _id_count;
    ++_id_count;

    EventIndex& idx = _events[id];

    idx.id = id;
    idx.name = name;
    idx.queue_name = queue_name;
    idx.event =  event;
    idx.is_user = false;
    idx.dependency_count = dependencies.get_id_count();

    for (int i = 0; i < idx.dependency_count; ++i) {
        idx.dependency_ids[i] = dependencies.get_ids()[i];
    }

    return CL::Event(id);
}


size_t CL::Device::setup_event_pad(const CL::Event& event, vector<cl_event>& event_pad, cl_event*& event_pad_ptr)
{
    const int* ids = event.get_ids();
    size_t cnt = event.get_id_count();

    if (event_pad.size() < cnt) {
        event_pad.resize(cnt);
    }

    for (size_t i = 0; i < cnt; ++i) {
        event_pad[i] = _events.at(ids[i]).event;
    }
        
    if (cnt == 0) {
        event_pad_ptr = 0;
    } else {
        event_pad_ptr = event_pad.data();
    }
        
    return cnt;
}




int CL::Device::insert_user_event(const string& name, cl_event event, const CL::Event& dependencies)
{
    int id = _id_count;
    ++_id_count;

    EventIndex& idx = _events[id];

    idx.id = id;
    idx.name = name;
    idx.queue_name = "host";
    idx.event =  event;
    idx.is_user = true;
    idx.user_begin = nanotime();
    idx.dependency_count = dependencies.get_id_count();

    for (int i = 0; i < idx.dependency_count; ++i) {
        idx.dependency_ids[i] = dependencies.get_ids()[i];
    }

    return id;    
}


void CL::Device::end_user_event(int id)
{
    _events[id].user_end = nanotime();
}




void CL::Device::dump_trace()
{
    _dump_trace = true;
}


void CL::Device::release_events()
{
    cl_int status;
    
    if (_dump_trace) {
        _dump_trace = false;
        
        std::ofstream fs(config.trace_file().c_str());

        for (auto i : _events) {
            const EventIndex& idx = i.second;
            
            cl_ulong queued, submit, start, end;

            if (idx.is_user) {
                queued = idx.user_begin;
                submit = idx.user_begin;
                start = idx.user_begin;
                end = idx.user_end;
                
            } else {
                status = clGetEventProfilingInfo(idx.event, CL_PROFILING_COMMAND_QUEUED, sizeof(queued), &queued, NULL);
                OPENCL_ASSERT(status);
            
                status = clGetEventProfilingInfo(idx.event, CL_PROFILING_COMMAND_SUBMIT, sizeof(submit), &submit, NULL);
                OPENCL_ASSERT(status);
            
                status = clGetEventProfilingInfo(idx.event, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
                OPENCL_ASSERT(status);
            
                status = clGetEventProfilingInfo(idx.event, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
                OPENCL_ASSERT(status);
            }
            
            fs << idx.name << "@" << idx.queue_name << ":"
               << queued << ":"
               << submit << ":"
               << start << ":"
               << end << ":"
               << idx.id << ":";

            for (int i = 0; i < idx.dependency_count; ++i) {
                fs << idx.dependency_ids[i];

                if (i+1 < idx.dependency_count)
                    fs << "|";
            }
            
            fs << endl;
        }

        cout << endl << "OpenCL trace dumped." << endl << endl;
            
    }

    
    for (auto i : _events) {
        const EventIndex& idx = i.second;
                
        status = clReleaseEvent(idx.event);

        OPENCL_ASSERT(status);
    }
  
    _events.clear();
    _id_count = 0;
}



size_t CL::Device::max_compute_units() const
{
    cl_uint retval;

    cl_int status = clGetDeviceInfo(_device, CL_DEVICE_MAX_COMPUTE_UNITS,
                                    sizeof(retval), &retval, nullptr);
    OPENCL_ASSERT(status);

    return retval;
}


size_t CL::Device::preferred_work_group_size_multiple() const
{
#warning TODO: find a more elegant way to do this
    
    if (is_GPU_device(_device)) {
        return 64;
    } else {
        return 4;
    }
}


namespace
{
    
    void get_opencl_device(int& platform_index, int& device_index,
                           cl_platform_id &platform, cl_device_id &device)
    {
        cl_int status;

        cl_platform_id platforms[10];
        cl_uint num_platforms;

    
        status = clGetPlatformIDs(10, platforms, &num_platforms);

        OPENCL_ASSERT(status);

        if (platform_index >= int(num_platforms)) {
            if (num_platforms == 0) {
                OPENCL_EXCEPTION("No OpenCL platforms available.");
            } else {
                cerr << "WARNING: No OpenCL platform " << platform_index 
                     << " available. Falling back to platform number 0." << endl;
                platform_index = 0;
            }
        }

        cl_device_id devices[10];
        cl_uint num_devices;

        status = clGetDeviceIDs(platforms[platform_index], CL_DEVICE_TYPE_ALL, 
                                10, devices, &num_devices);

        OPENCL_ASSERT(status);

        if (device_index >= int(num_devices)) {
            if (num_platforms == 0) {
                OPENCL_EXCEPTION("No OpenCL devices available on platform.");
            } else {
                cerr << "WARNING: No OpenCL device " << device_index 
                     << " available. Falling back to device number 0." << endl;
                device_index = 0;
            }        
        }

        device = devices[device_index];
        platform = platforms[platform_index];
    }

    cl_context create_context_with_GL(cl_platform_id platform, cl_device_id device)
    {
        cl_int status;

#ifdef linux
        cl_context_properties props[] = 
            {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 
             CL_GL_CONTEXT_KHR,   (cl_context_properties)glfwGetGLXContext(glfwGetCurrentContext()),
             CL_GLX_DISPLAY_KHR,  (cl_context_properties)glfwGetX11Display(),
             0};
#endif
        
        cl_context context = clCreateContext(props, 
                                             1, &device,
                                             opencl_error_callback, NULL, &status);
        
        OPENCL_ASSERT(status);

        return context;
    }

    cl_context create_context_without_GL(cl_platform_id platform, cl_device_id device)
    {
        cl_int status;

        cl_context_properties props[] = 
            {CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0};
        cl_context context = clCreateContext(props, 
                                             1, &device,
                                             opencl_error_callback, NULL, &status);

        OPENCL_ASSERT(status);

        return context;
    }

    bool is_GPU_device(cl_device_id device)
    {
        cl_device_type type;
        cl_int status;

        status = clGetDeviceInfo(device, CL_DEVICE_TYPE,
                                 sizeof(type), &type, NULL);

        OPENCL_ASSERT(status);
        return type == CL_DEVICE_TYPE_GPU;
    }


    
    template <typename T> void print_device_param(cl_device_id device,
                                                  cl_device_info param_enum,
                                                  string param_name, 
                                                  const string& indent)
    {
        T tmp;
        cl_int status;

        status = clGetDeviceInfo(device, param_enum, sizeof(T), &tmp, NULL);

        if (status == CL_SUCCESS) {
            cout << indent << "  " << param_name << ": " << tmp << endl;
        } else {
            cout << indent << "  " << param_name << ": " << "<unsupported>" << endl;
        }
    }
                 
    void print_device_workgroup_size(cl_device_id device, const string& indent)
    {
        cl_uint max_dim;
        size_t sizes[50];

        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, 
                        sizeof(cl_uint), &max_dim, NULL);
        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES, 
                        sizeof(size_t)*max_dim, sizes, NULL);

        cout << indent << "  CL_DEVICE_MAX_WORK_ITEM_SIZES:";

        for (size_t i = 0; i < max_dim; ++i) {
            cout << " " << sizes[i];
        }

        cout << endl;
    }
                 
    void print_device_queue_properties(cl_device_id device, const string& indent)
    {
        cl_command_queue_properties props;

        clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, 
                        sizeof(props), &props, NULL);
        cout << indent << "  CL_DEVICE_QUEUE_PROPERTIES:";

        if (props | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
            cout << " out-of-order:YES";
        } else {
            cout << " out-of-order:NO";
        }

        if (props | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
            cout << ", profiling:YES";
        } else {
            cout << ", profiling:NO";
        }

        cout << endl;
    }

    void CL_CALLBACK opencl_error_callback(const char* errinfo, const void* private_info, size_t cb, void* user_data)
    {
        cout << errinfo << endl;
        
#ifdef DEBUG_OPENCL
        raise(SIGTRAP);
#endif
    }

}

