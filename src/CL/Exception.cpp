#include "Exception.h"





CL::Exception::Exception(cl_int err_code, const string& file, int line_no):
    _file(file), _line_no(line_no) 
{
    get_errors();

    switch(err_code) {
    case CL_BUILD_PROGRAM_FAILURE:               _msg = "Program build failure";                 break;
    case CL_COMPILER_NOT_AVAILABLE:              _msg = "Compiler not available";                break;
    case CL_INVALID_ARG_INDEX:                   _msg = "Invalid kernel argument index";         break;
    case CL_INVALID_ARG_SIZE:                    _msg = "Invalid kernel argument size";          break;
    case CL_INVALID_ARG_VALUE:                   _msg = "Invalid kernel argument value";         break;
    case CL_INVALID_BINARY:                      _msg = "Invalid binary";                        break;
    case CL_INVALID_BUFFER_SIZE:                 _msg = "Invalid buffer size";                   break;
    case CL_INVALID_BUILD_OPTIONS:               _msg = "Invalid build options";                 break;
    case CL_INVALID_COMMAND_QUEUE:               _msg = "Invalid command queue";                 break;
    case CL_INVALID_CONTEXT:                     _msg = "Invalid context";                       break;
    case CL_INVALID_DEVICE:                      _msg = "Invalid device";                        break;
    case CL_INVALID_EVENT_WAIT_LIST:             _msg = "Invalid event wait list";               break;
    case CL_INVALID_EVENT:                       _msg = "Invalid event";
    case CL_INVALID_GLOBAL_OFFSET:               _msg = "Invalid global offset";                 break;
    case CL_INVALID_GLOBAL_WORK_SIZE:            _msg = "Invalid global work size";              break;
    case CL_INVALID_GL_OBJECT:                   _msg = "Invalid OpenGL object";                 break;
    case CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR: _msg = "Invalid OpenGL sharegroup reference";   break;
    case CL_INVALID_IMAGE_SIZE:                  _msg = "Invalid image size";                    break;
    case CL_INVALID_KERNEL:                      _msg = "Invalid kernel";                        break;
    case CL_INVALID_KERNEL_ARGS:                 _msg = "(Some) kernel args not specified";      break;
    case CL_INVALID_KERNEL_DEFINITION:           _msg = "Invalid kernel definition";             break;
    case CL_INVALID_KERNEL_NAME:                 _msg = "Kernel name not found in program";      break;
    case CL_INVALID_MEM_OBJECT:                  _msg = "Invalid mem object";                    break;
    case CL_INVALID_MIP_LEVEL:                   _msg = "Invalid miplevel";                      break;
    case CL_INVALID_OPERATION:                   _msg = "Invalid operation";                     break;
    case CL_INVALID_PROGRAM:                     _msg = "Invalid program";                       break;
    case CL_INVALID_PROGRAM_EXECUTABLE:          _msg = "Program not built successfully";        break;
    case CL_INVALID_PROPERTY:                    _msg = "Invalid property";                      break;
    case CL_INVALID_SAMPLER:                     _msg = "Invalid sampler object";                break;
    case CL_INVALID_VALUE:                       _msg = "Invalid value";                         break;
    case CL_INVALID_WORK_DIMENSION:              _msg = "Invalid work dimension";                break;
    case CL_INVALID_WORK_GROUP_SIZE:             _msg = "Invalid work group size";               break;
    case CL_INVALID_WORK_ITEM_SIZE:              _msg = "Invalid work item size";                break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:       _msg = "Mem-object allocation failure";         break;
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:        _msg = "Misaligned sub-buffer offset";          break;
    case CL_OUT_OF_HOST_MEMORY:                  _msg = "Out of host memory";                    break;
    case CL_OUT_OF_RESOURCES:                    _msg = "Out of resources";                      break;
    case CL_PROFILING_INFO_NOT_AVAILABLE:        _msg = "Profiling info not available";          break;
    case -1001:                                  _msg = "Vendor ICD not correctly installed(?)"; break;
    default:                                     _msg = "Unknown error: " + to_string(err_code);
    }
}

CL::Exception::Exception(const string& msg, const string& file, int line_no):
    _msg(msg), _file(file), _line_no(line_no) 
{

}

const string& CL::Exception::msg()
{
    return _msg;
}
