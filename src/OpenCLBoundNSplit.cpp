#include "OpenCLBoundNSplit.h"


#include "PatchesIndex.h"
#include "Config.h"
#include "Statistics.h"


using namespace Reyes;


Reyes::OpenCLBoundNSplit::OpenCLBoundNSplit(CL::Device& device,
                                            CL::CommandQueue& queue,
                                            shared_ptr<PatchesIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)
    , _active_handle(nullptr)
    , _active_patch_buffer(nullptr)
    , _patch_ids(device, _queue, 2*config.reyes_patches_per_pass() * sizeof(int), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
    , _patch_min(device, _queue, 2*config.reyes_patches_per_pass() * sizeof(vec2), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
    , _patch_max(device, _queue, 2*config.reyes_patches_per_pass() * sizeof(vec2), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
{
    _patch_index->enable_retain_vector();
    _patch_index->enable_load_opencl_buffer(device, queue);
}


void Reyes::OpenCLBoundNSplit::init(void* patches_handle, const mat4& matrix, const Projection* projection)
{
    statistics.start_bound_n_split();
    
    _active_handle = patches_handle;
    _active_patch_buffer = _patch_index->get_opencl_buffer(patches_handle);
    
    const vector<BezierPatch>& patches = _patch_index->get_patch_vector(_active_handle);
    
    _stack.resize(patches.size());
    for (size_t i = 0; i < patches.size(); ++i) {
        _stack[i] = PatchRange{Bound(0,0,1,1), 0, i};
    }

    _projection = projection;
    
    mat4 proj;
    _projection->calc_projection_with_aspect_correction(proj);
    
    _mvp = proj * matrix;
    _mv = matrix;

    statistics.stop_bound_n_split();
}


bool Reyes::OpenCLBoundNSplit::done()
{
    return _stack.size() == 0;
}


Batch Reyes::OpenCLBoundNSplit::do_bound_n_split(CL::Event& ready)
{
    _queue.wait_for_events(ready);
    
    statistics.start_bound_n_split();
    
    const vector<BezierPatch>& patches = _patch_index->get_patch_vector(_active_handle);

    size_t patch_count = 0;
    int*  pids = (int* )_patch_ids.host_ptr();
    vec2* mins = (vec2*)_patch_min.host_ptr();
    vec2* maxs = (vec2*)_patch_max.host_ptr();
    
    BBox box;
    float vlen, hlen;

    float s = config.bound_n_split_limit();

    PatchRange r0,r1;
    
    while (!_stack.empty()) {

        PatchRange r = _stack.back();
        _stack.pop_back();


        bound_patch_range(r, patches[r.patch_id], _mv, _mvp, box, vlen, hlen);

        vec2 size;
        bool cull;
        _projection->bound(box, size, cull);

        if (cull) continue;

        if (box.min.z < 0 && size.x < s && size.y < s) {
            
            const Bound& pr = r.range;
            const size_t pid = r.patch_id;

            pids[patch_count] = pid;
            mins[patch_count] = r.range.min;
            maxs[patch_count] = r.range.max;
            
            ++patch_count;
            statistics.inc_patch_count();
            
            if (patch_count >= config.reyes_patches_per_pass()) {
                break; // buffer full, end this batch
            }

        } else if (r.depth > config.max_split_depth()) {
            // TODO: Add low-overhead warning mechanism for this
            // cout << "Warning: Split limit reached" << endl
        } else {
            if (vlen < hlen) {
                vsplit_range(r, r0, r1);
            } else {
                hsplit_range(r, r0, r1);
            }

            _stack.push_back(r0);
            _stack.push_back(r1);
        }

    }

    // Transfer local data to OpenCL buffers
    CL::Event a,b,c;
    a = _queue.enq_write_buffer(_patch_ids, pids, patch_count * sizeof(int), "write patch ids" , CL::Event());
    b = _queue.enq_write_buffer(_patch_min, mins, patch_count * sizeof(vec2), "write patch mins", CL::Event());
    c = _queue.enq_write_buffer(_patch_max, maxs, patch_count * sizeof(vec2), "write patch maxs", CL::Event());
    
    //_queue.wait_for_events(a|b|c);
    statistics.stop_bound_n_split();
    
    return {patch_count, *_active_patch_buffer, _patch_ids, _patch_min, _patch_max, a|b|c};
}
