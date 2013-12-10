#include "OpenCLBoundNSplit.h"


#include "PatchIndex.h"
#include "Config.h"
#include "Statistics.h"


using namespace Reyes;


Reyes::OpenCLBoundNSplit::OpenCLBoundNSplit(CL::Device& device,
                                            CL::CommandQueue& queue,
                                            shared_ptr<PatchIndex>& patch_index)
    : _queue(queue)
    , _patch_index(patch_index)
    , _active_handle(nullptr)
    , _active_patch_buffer(nullptr)
    , _bound_n_split_event(device, "CPU bound & split")
    , _next_batch_record(0)
{
    _patch_index->enable_retain_vector();
    _patch_index->enable_load_opencl_buffer(device, queue);

    _bound_n_split_program.set_constant("CULL_RIBBON", config.cull_ribbon());
    _bound_n_split_program.set_constant("SCREEN_SIZE", config.window_size());
    
    _bound_n_split_program.compile(device, "bound_n_split.cl");
    shared_ptr<CL::Kernel> _bound_kernel;
    _bound_kernel.reset(_bound_n_split_program.get_kernel("bound"));

    for (int i : irange(0, config.patch_buffer_count())) {
        _batch_records.emplace_back(config.reyes_patches_per_pass(), device, queue);
    }
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
    if (_stack.size() > 0) return false;

    
    return true;
}

void Reyes::OpenCLBoundNSplit::finish()
{
    for (BatchRecord& record : _batch_records) {
        record.finish(_queue);
    }
    _next_batch_record = 0;
}


Batch Reyes::OpenCLBoundNSplit::do_bound_n_split(CL::Event& ready)
{
    size_t ring_size = config.patch_buffer_count(); // Size of batch buffer ring

    if (_next_batch_record > 0)
        _batch_records[(_next_batch_record-1)%ring_size].accept(ready);
    BatchRecord& record = _batch_records[_next_batch_record % ring_size];
    _next_batch_record++;

    record.finish(_queue);
    
    _bound_n_split_event.begin();
    statistics.start_bound_n_split();
    
    const vector<BezierPatch>& patches = _patch_index->get_patch_vector(_active_handle);
    
    size_t patch_count = 0;
    int*  pids = (int* )record.patch_ids.host_ptr();
    vec2* mins = (vec2*)record.patch_min.host_ptr();
    vec2* maxs = (vec2*)record.patch_max.host_ptr();
    
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

    record.transfer(_queue, patch_count);
    
    statistics.stop_bound_n_split();
    _bound_n_split_event.end();
    
    return {patch_count, *_active_patch_buffer, record.patch_ids, record.patch_min, record.patch_max, record.transferred};
}


Reyes::OpenCLBoundNSplit::BatchRecord::BatchRecord(size_t batch_size, CL::Device& device, CL::CommandQueue& queue)
    : status(INACTIVE)
    , patch_ids(device, queue, batch_size * sizeof(int), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
    , patch_min(device, queue, batch_size * sizeof(vec2), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
    , patch_max(device, queue, batch_size * sizeof(vec2), CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY)
    , transferred()
    , rasterizer_done()
{

}


Reyes::OpenCLBoundNSplit::BatchRecord::BatchRecord(BatchRecord&& other)
{
    status = std::move(other.status);
    patch_ids = std::move(other.patch_ids);
    patch_min = std::move(other.patch_max);
    patch_max = std::move(other.patch_min);
    transferred = other.transferred;
    rasterizer_done = other.rasterizer_done;
}


void Reyes::OpenCLBoundNSplit::BatchRecord::transfer(CL::CommandQueue& queue, size_t patch_count)
{
    CL::Event a,b,c;

    if (patch_count > 0) {
        a = queue.enq_write_buffer(patch_ids, patch_ids.host_ptr(), patch_count * sizeof(int), "write patch ids" , CL::Event());
        b = queue.enq_write_buffer(patch_min, patch_min.host_ptr(), patch_count * sizeof(vec2), "write patch mins", CL::Event());
        c = queue.enq_write_buffer(patch_max, patch_max.host_ptr(), patch_count * sizeof(vec2), "write patch maxs", CL::Event());
        
        queue.flush();
        
        status = SET_UP;
    }
    transferred = a|b|c;
}


void Reyes::OpenCLBoundNSplit::BatchRecord::accept(CL::Event& event)
{
    status = ACCEPTED;
    rasterizer_done = event;
}


void Reyes::OpenCLBoundNSplit::BatchRecord::finish(CL::CommandQueue& queue)
{
    if (status == INACTIVE) {
        return;
    } else if (status == ACCEPTED) {
        queue.wait_for_events(rasterizer_done);
    } else if (status == SET_UP) {
        queue.wait_for_events(transferred);
    }

    rasterizer_done = CL::Event();
    transferred = CL::Event();
    status = INACTIVE;    
}
