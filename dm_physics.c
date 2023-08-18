#include "dm.h"

#include "debug_render_pass.h"

#include <limits.h>
#include <float.h>
#include <assert.h>

#ifdef DM_PHYSICS_SMALLER_DT
#define DM_PHYSICS_FIXED_DT          0.00416f // 1 / 240
#define DM_PHYSICS_FIXED_DT_INV      240
#else
#define DM_PHYSICS_FIXED_DT          0.00833f // 1 / 120
#define DM_PHYSICS_FIXED_DT_INV      120
#endif

#define DM_PHYSICS_MAX_GJK_ITER      64
#define DM_PHYSICS_EPA_MAX_FACES     64
#define DM_PHYSICS_EPA_MAX_ITER      64

#define DM_PHYSICS_EPA_TOLERANCE     0.00001f
#define DM_PHYSICS_MAX_CONTACTS      8
#define DM_PHYSICS_DIVISION_EPSILON  1e-8f
#define DM_PHYSICS_TEST_EPSILON      1e-4f
#define DM_PHYSICS_CLIP_MAX_PTS      15
#define DM_PHYSICS_CONSTRAINT_ITER   10
#define DM_PHYSICS_PERSISTENT_THRESH 0.001f
#define DM_PHYSICS_W_LIM             50.0f

// collision handling
typedef struct dm_plane
{
    float normal[3];
    float distance;
} dm_plane;

typedef struct dm_simplex
{
    float    points[4][3];
    uint32_t size;
} dm_simplex;

typedef struct dm_contact_constraint
{
    float  jacobian[4][3];
    float  delta_v[4][3];
    float  b;
    double lambda, warm_lambda, impulse_sum, impulse_min, impulse_max;
} dm_contact_constraint;

typedef struct dm_contact_point
{
    dm_contact_constraint normal;
    dm_contact_constraint friction_a, friction_b;
    
    float global_pos[2][3];
    float local_pos[2][3];
    float penetration;
} dm_contact_point;

typedef struct dm_contact_data_t
{
    float *vel_x, *vel_y, *vel_z;
    float *w_x, *w_y, *w_z;
    
    float mass, inv_mass;
    float i_body_inv_00, i_body_inv_11, i_body_inv_22;
    float v_damp, w_damp;
} dm_contact_data;

typedef struct dm_contact_manifold
{
    float     normal[3];
    float     tangent_a[3]; 
    float     tangent_b[3];
    float     orientation_a[4];
    float     orientation_b[4];
    
    dm_contact_data contact_data[2];
    
    dm_contact_point points[DM_PHYSICS_CLIP_MAX_PTS];
    uint32_t         point_count;
} dm_contact_manifold;

typedef struct dm_collision_pair_t
{
    dm_ecs_system_entity_container entity_a, entity_b;
} dm_collision_pair;

typedef enum dm_physics_flag_t
{
    DM_PHYSICS_FLAG_PAUSED,
    DM_PHYSICS_FLAG_UNKNOWN
} dm_physics_flag;

typedef struct dm_physics_manager_t
{
    double          accum_time, simulation_time;
    dm_physics_flag flag;
    
    uint32_t num_possible_collisions, collision_capacity;
    uint32_t num_manifolds, manifold_capacity;
    
    dm_collision_pair*   possible_collisions;
    dm_contact_manifold* manifolds;
} dm_physics_manager;

typedef struct dm_physics_broadphase_sort_data_t
{
    uint32_t index;
    size_t   block_size;
    float*   min;
} dm_physics_broadphase_sort_data;

#define DM_PHYSICS_DEFAULT_COLLISION_CAPACITY 16
#define DM_PHYSICS_DEFAULT_MANIFOLD_CAPACITY  16
#define DM_PHYSICS_LOAD_FACTOR                0.75f
#define DM_PHYSICS_RESIZE_FACTOR              2

/**********
BROADPHASE
************/
#ifdef DM_PLATFORM_LINUX
int dm_physics_broadphase_sort(const void* a, const void* b, void* c)
#else
int dm_physics_broadphase_sort(void* c, const void* a, const void* b)
#endif
{
    dm_physics_broadphase_sort_data* sort_data = c;
    dm_ecs_system_entity_container container_a = *(dm_ecs_system_entity_container*)a;
    dm_ecs_system_entity_container container_b = *(dm_ecs_system_entity_container*)b;
    
    uint32_t c_index = sort_data->index;
    
    uint32_t a_c_index = container_a.component_indices[c_index];
    uint32_t b_c_index = container_b.component_indices[c_index];
    
    uint32_t a_b_index = container_a.block_indices[c_index];
    uint32_t b_b_index = container_b.block_indices[c_index];
    
    size_t a_offset = a_b_index * sort_data->block_size;
    size_t b_offset = b_b_index * sort_data->block_size;
    
    float a_min = ((float*)((char*)sort_data->min + a_offset))[a_c_index];
    float b_min = ((float*)((char*)sort_data->min + b_offset))[b_c_index];
    
    return (a_min > b_min) - (a_min < b_min);
}

void dm_physics_update_sphere_aabb(uint32_t t_index, uint32_t c_index,  dm_component_transform_block* t_block, dm_component_collision_block* c_block)
{
    c_block->aabb_global_min_x[c_index] = c_block->aabb_local_min_x[c_index] + t_block->pos_x[t_index];
    c_block->aabb_global_min_y[c_index] = c_block->aabb_local_min_y[c_index] + t_block->pos_y[t_index];
    c_block->aabb_global_min_z[c_index] = c_block->aabb_local_min_z[c_index] + t_block->pos_z[t_index];
    
    c_block->aabb_global_max_x[c_index] = c_block->aabb_local_max_x[c_index] + t_block->pos_x[t_index];
    c_block->aabb_global_max_y[c_index] = c_block->aabb_local_max_y[c_index] + t_block->pos_y[t_index];
    c_block->aabb_global_max_z[c_index] = c_block->aabb_local_max_z[c_index] + t_block->pos_z[t_index];
}

void dm_physics_update_box_aabb(const uint32_t t_index, const uint32_t c_index,  dm_component_transform_block* t_block, dm_component_collision_block* c_block)
{
    float a,b      = 0.0f;
    float quat[N4] = { 0 };
    float rot[M3]  = { 0 };
    
    float world_min[3], world_max[3];
    
    quat[0] = t_block->rot_i[t_index];
    quat[1] = t_block->rot_j[t_index];
    quat[2] = t_block->rot_k[t_index];
    quat[3] = t_block->rot_r[t_index];
    
    dm_mat3_rotate_from_quat(quat, rot);
    
    const float min[] = {
        c_block->aabb_local_min_x[c_index],
        c_block->aabb_local_min_y[c_index],
        c_block->aabb_local_min_z[c_index],
    };
    
    const float max[] = {
        c_block->aabb_local_max_x[c_index],
        c_block->aabb_local_max_y[c_index],
        c_block->aabb_local_max_z[c_index],
    };
    
    // start
    world_min[0] = world_max[0] = t_block->pos_x[t_index];
    world_min[1] = world_max[1] = t_block->pos_y[t_index];
    world_min[2] = world_max[2] = t_block->pos_z[t_index];
    
    for(uint32_t i=0; i<3; i++)
    {
        for(uint32_t j=0; j<3; j++)
        {
            a = rot[j * 3 + i] * min[j];
            b = rot[j * 3 + i] * max[j];
            
            world_min[i] += DM_MIN(a,b);
            world_max[i] += DM_MAX(a,b);
        }
    }
    
    c_block->aabb_global_min_x[c_index] = world_min[0];
    c_block->aabb_global_min_y[c_index] = world_min[1];
    c_block->aabb_global_min_z[c_index] = world_min[2];
    
    c_block->aabb_global_max_x[c_index] = world_max[0];
    c_block->aabb_global_max_y[c_index] = world_max[1];
    c_block->aabb_global_max_z[c_index] = world_max[2];
}

void dm_physics_update_world_aabb(uint32_t t_index, uint32_t c_index,  dm_component_transform_block* t_block, dm_component_collision_block* c_block)
{
    switch(c_block->shape[c_index])
    {
        case DM_COLLISION_SHAPE_SPHERE: dm_physics_update_sphere_aabb(t_index, c_index, t_block, c_block);
        break;
        
        case DM_COLLISION_SHAPE_BOX: dm_physics_update_box_aabb(t_index, c_index, t_block, c_block);
        break;
        
        default:
        DM_LOG_ERROR("Unknown collider! Shouldn't be here...");
        break;
    }
}

int dm_physics_broadphase_get_variance_axis(dm_ecs_system_manager* system, dm_context* context)
{
    float center_sum[3]    = { 0 };
    float center_sq_sum[3] = { 0 };
    
    int axis = 0;
    
    dm_component_collision_block* c_block;
    dm_component_transform_block* t_block;
    
    const uint32_t t_id = context->ecs_manager.default_components.transform;
    const uint32_t c_id = context->ecs_manager.default_components.collision;
    
    uint32_t  t_block_index, t_index;
    uint32_t  c_block_index, c_index;
    
    float center[N3];
    
    for(uint32_t e=0; e<system->entity_count; e++)
    {
        t_index       = system->entity_containers[e].component_indices[t_id];
        t_block_index = system->entity_containers[e].block_indices[t_id];
        
        c_index       = system->entity_containers[e].component_indices[c_id];
        c_block_index = system->entity_containers[e].block_indices[c_id];
        
        t_block = (dm_component_transform_block*)context->ecs_manager.components[t_id].data + t_block_index;
        c_block = (dm_component_collision_block*)context->ecs_manager.components[c_id].data + c_block_index;
        
        // reset collision flags
        c_block->flag[c_index] = DM_COLLISION_FLAG_NO;
        
        // update global aabb
        dm_physics_update_world_aabb(t_index, c_index, t_block, c_block);
        
        // add to center and center sq vectors
        center[0] = 0.5f * (c_block->aabb_global_max_x[c_index] - c_block->aabb_global_min_x[c_index]);
        center[1] = 0.5f * (c_block->aabb_global_max_y[c_index] - c_block->aabb_global_min_y[c_index]);
        center[2] = 0.5f * (c_block->aabb_global_max_z[c_index] - c_block->aabb_global_min_z[c_index]);
        
        float dum[3];
        dm_vec3_add_vec3(center_sum, center, center_sum);
        dm_vec3_mul_vec3(center_sum, center_sum, dum);
        dm_vec3_add_vec3(center_sq_sum, dum, center_sq_sum);
    }
    
    const float scale = 1.0f / system->entity_count;
    dm_vec3_scale(center_sum, scale, center_sum);
    dm_vec3_scale(center_sq_sum, scale, center_sq_sum);
    
    float var[3];
    dm_vec3_mul_vec3(center_sum, center_sum, center_sum);
    var[0] = center_sq_sum[0] - center_sum[0];
    var[1] = center_sq_sum[1] - center_sum[1];
    var[2] = center_sq_sum[2] - center_sum[2];
    
    float max_var = -FLT_MAX;
    for(uint32_t i=0; i<N3; i++)
    {
        if(var[0] <= max_var) continue;
        
        max_var = var[i];
        axis = i;
    }
    
    return axis;
}

// uses simple sort and sweep based on objects' aabbs
bool dm_physics_broadphase(dm_ecs_system_manager* system, dm_physics_manager* manager, dm_context* context)
{
    //manager->collision_capacity = DM_PHYSICS_DEFAULT_COLLISION_CAPACITY;
    manager->num_possible_collisions = 0;
    
    const int axis = dm_physics_broadphase_get_variance_axis(system, context);
    
    dm_physics_broadphase_sort_data data = { 0 };
    data.index = context->ecs_manager.default_components.collision;
    data.block_size = sizeof(dm_component_collision_block);
    
    dm_component_collision_block* master_block = context->ecs_manager.components[data.index].data;
    dm_component_collision_block* a_block;
    dm_component_collision_block* b_block;
    
    // sort
    switch(axis)
    {
        case 0:
        data.min = master_block->aabb_global_min_x;
        break;
        
        case 1:
        data.min = master_block->aabb_global_min_y;
        break;
        
        case 2:
        data.min = master_block->aabb_global_min_z;
        break;
    }
    
#ifdef DM_PLATFORM_WIN32
    qsort_s(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
#elif defined(DM_PLATFORM_LINUX)
    qsort_r(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
#elif defined(DM_PLATFORM_APPLE)
    qsort_r(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), &data, dm_physics_broadphase_sort);
#endif
    
    // sweep
    float max_i, min_j;
    dm_ecs_system_entity_container entity_a, entity_b;
    uint32_t a_c_index, b_c_index;
    
    float load;
    bool x_check, y_check, z_check;
    
    float a_pos[3], a_dim[3];
    float b_pos[3], b_dim[3];
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        entity_a = system->entity_containers[i];
        
        a_c_index = entity_a.component_indices[data.index];
        
        a_block = master_block + entity_a.block_indices[data.index];
        
        a_dim[0] = 0.5f * dm_fabs(a_block->aabb_global_max_x[a_c_index] - a_block->aabb_global_min_x[a_c_index]);
        a_dim[1] = 0.5f * dm_fabs(a_block->aabb_global_max_y[a_c_index] - a_block->aabb_global_min_y[a_c_index]);
        a_dim[2] = 0.5f * dm_fabs(a_block->aabb_global_max_z[a_c_index] - a_block->aabb_global_min_z[a_c_index]);
        
        a_pos[0] = a_block->aabb_global_max_x[a_c_index] - a_dim[0];
        a_pos[1] = a_block->aabb_global_max_y[a_c_index] - a_dim[1];
        a_pos[2] = a_block->aabb_global_max_z[a_c_index] - a_dim[2];
        
        switch(axis)
        {
            case 0: max_i = a_block->aabb_global_max_x[a_c_index];
            break;
            
            case 1: max_i = a_block->aabb_global_max_y[a_c_index];
            break;
            
            case 2: max_i = a_block->aabb_global_max_z[a_c_index];
            break;
            
            default:
            DM_LOG_FATAL("Shouldn't be here in broadphase");
            return false;
        }
        
        for(uint32_t j=i+1; j<system->entity_count; j++)
        {
            entity_b = system->entity_containers[j];
            
            b_c_index = entity_b.component_indices[data.index];
            
            b_block = master_block + entity_b.block_indices[data.index];
            
            b_dim[0] = 0.5f * dm_fabs(b_block->aabb_global_max_x[b_c_index] - b_block->aabb_global_min_x[b_c_index]);
            b_dim[1] = 0.5f * dm_fabs(b_block->aabb_global_max_y[b_c_index] - b_block->aabb_global_min_y[b_c_index]);
            b_dim[2] = 0.5f * dm_fabs(b_block->aabb_global_max_z[b_c_index] - b_block->aabb_global_min_z[b_c_index]);
            
            b_pos[0] = b_block->aabb_global_max_x[b_c_index] - b_dim[0];
            b_pos[1] = b_block->aabb_global_max_y[b_c_index] - b_dim[1];
            b_pos[2] = b_block->aabb_global_max_z[b_c_index] - b_dim[2];
            
            switch(axis)
            {
                case 0: min_j = b_block->aabb_global_min_x[b_c_index];
                break;
                
                case 1: min_j = b_block->aabb_global_min_y[b_c_index];
                break;
                
                case 2: min_j = b_block->aabb_global_min_z[b_c_index];
                break;
                
                default:
                DM_LOG_FATAL("Shouldn't be here in broadphase");
                return false;
            }
            
            if(min_j > max_i) break;
            
            x_check = dm_fabs(a_pos[0] - b_pos[0]) <= (a_dim[0] + b_dim[0]);
            y_check = dm_fabs(a_pos[1] - b_pos[1]) <= (a_dim[1] + b_dim[1]);
            z_check = dm_fabs(a_pos[2] - b_pos[2]) <= (a_dim[2] + b_dim[2]);
            
            if(!x_check || !y_check || !z_check) continue;
            
            a_block->flag[a_c_index] = DM_COLLISION_FLAG_POSSIBLE;
            b_block->flag[b_c_index] = DM_COLLISION_FLAG_POSSIBLE;
            
            manager->possible_collisions[manager->num_possible_collisions].entity_a = entity_a;
            manager->possible_collisions[manager->num_possible_collisions].entity_b = entity_b;
            manager->num_possible_collisions++;
            
            load = (float)manager->num_possible_collisions / (float)manager->collision_capacity;
            if(load < DM_PHYSICS_LOAD_FACTOR) continue;
            
            manager->collision_capacity *= DM_PHYSICS_RESIZE_FACTOR;
            manager->possible_collisions = dm_realloc(manager->possible_collisions, sizeof(dm_collision_pair) * manager->collision_capacity);
        }
    }
    
    return true;
}

/***********
NARROWPHASE
*************/
// 3d support funcs
void dm_physics_support_func_sphere(const float pos[3], const float cen[3], const float internals[6], const float dir[3], float support[3])
{
    const float radius = internals[0];
    
    support[0] = (dir[0] * radius) + (pos[0] + cen[0]); 
    support[1] = (dir[1] * radius) + (pos[1] + cen[1]); 
    support[2] = (dir[2] * radius) + (pos[2] + cen[2]);
}

void dm_physics_support_func_box(const float pos[3], const float rot[4], const float cen[3], const float internals[6], const float dir[3], float support[3])
{
    float inv_rot[4] = { 0 };
    float d_rot[3]   = { 0 };
    float sup[3]     = { 0 };
    
    // put direction into local space
    dm_quat_inverse(rot, inv_rot);
    dm_vec3_rotate(dir, inv_rot, d_rot);
    
    sup[0] = (d_rot[0] > 0) ? internals[3] : internals[0];
    sup[1] = (d_rot[1] > 0) ? internals[4] : internals[1];
    sup[2] = (d_rot[2] > 0) ? internals[5] : internals[2];
    
    // rotate back into world space
    dm_vec3_rotate(sup, rot, sup);
    dm_vec3_add_vec3(sup, cen, sup);
    dm_vec3_add_vec3(sup, pos, support);
}

// gjk
void dm_physics_support_entity(const float pos[3], const float rot[4], const float cen[3], const float internals[6], const dm_collision_shape shape, float direction[3], float support[3])
{
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE: 
        dm_physics_support_func_sphere(pos, cen, internals, direction, support);
        break;
        
        case DM_COLLISION_SHAPE_BOX: 
        dm_physics_support_func_box(pos, rot, cen, internals, direction, support);
        break;
        
        default:
        DM_LOG_ERROR("Collision shape not supported, or unknown shape! Probably shouldn't be here...");
        break;
    }
}

void dm_physics_support(const float pos[2][3], const float rot[2][4], const float cen[2][3], const float internals[2][6], const dm_collision_shape shapes[2], float direction[3], float support[3], float supports[2][3])
{
    float dir_neg[3];
    float support_a[3];
    float support_b[3];
    
    dm_vec3_negate(direction, dir_neg);
    
    dm_physics_support_entity(pos[0], rot[0], cen[0], internals[0], shapes[0], direction, support_a);
    dm_physics_support_entity(pos[1], rot[1], cen[1], internals[1], shapes[1], dir_neg,   support_b);
    
    DM_VEC3_COPY(supports[0], support_a);
    DM_VEC3_COPY(supports[1], support_b);
    
    dm_vec3_sub_vec3(support_a, support_b, support);
}

void dm_simplex_push_front(float point[3], dm_simplex* simplex)
{
    DM_VEC3_COPY(simplex->points[3], simplex->points[2]);
    DM_VEC3_COPY(simplex->points[2], simplex->points[1]);
    DM_VEC3_COPY(simplex->points[1], simplex->points[0]);
    DM_VEC3_COPY(simplex->points[0], point);
    
    simplex->size++;
    simplex->size = DM_CLAMP(simplex->size, 0, 4);
}

/*
find vector ab. find vector a to origin. 
if ao is not in the same direction as ab, we need to restart
*/
bool dm_simplex_line(float direction[3], dm_simplex* simplex)
{
    float a[3], b[3];
    float ab[3]; float ao[3];
    
    DM_VEC3_COPY(a, simplex->points[0]);
    DM_VEC3_COPY(b, simplex->points[1]);
    
    dm_vec3_sub_vec3(b, a, ab);
    dm_vec3_negate(a, ao);
    
    if(dm_vec3_same_direction(ab, ao)) 
    {
        dm_vec3_cross_cross(ab, ao, ab, direction);
    }
    else 
    {
        simplex->size = 1;
        DM_VEC3_COPY(direction, ao);
    }
    
    return false;
}

/*
C         .
|\   1   .
| \     .
|  \   .
| 2 \ .  
------
|    A  5
| 3 / .
|  /   .
| /     .
 |/   4   .
B         .
TODO: I think some of these checks are not needed
*/
bool dm_simplex_triangle(float direction[3], dm_simplex* simplex)
{
    float a[3], b[3], c[3];
    float ab[3], ac[3], ao[3], abc[3];
    float dum[3];
    
    DM_VEC3_COPY(a, simplex->points[0]);
    DM_VEC3_COPY(b, simplex->points[1]);
    DM_VEC3_COPY(c, simplex->points[2]);
    
    dm_vec3_sub_vec3(b, a, ab);
    dm_vec3_sub_vec3(c, a, ac);
    dm_vec3_negate(a, ao);
    
    dm_vec3_cross(ab, ac, abc);
    
    // check beyond AC plane (region 1 and 5)
    dm_vec3_cross(abc, ac, dum);
    if(dm_vec3_same_direction(dum, ao))
    {
        // are we actually in region 1?
        if(dm_vec3_same_direction(ac, ao))
        {
            DM_VEC3_COPY(simplex->points[1], c);
            simplex->size = 2;
            
            dm_vec3_cross_cross(ac, ao, ac, direction);
        }
        else 
        {
            // are we in region 4?
            if(dm_vec3_same_direction(ab, ao))
            {
                simplex->size = 2;
                
                dm_vec3_cross_cross(ab, ao, ab, direction);
            }
            // must be in region 5
            else
            {
                simplex->size = 1;
                
                DM_VEC3_COPY(direction, ao);
            }
        }
    }
    else
    {
        // check beyond AB plane (region 4 and 5)
        dm_vec3_cross(ab, abc, dum);
        if(dm_vec3_same_direction(dum, ao))
        {
            // are we in region 4?
            if(dm_vec3_same_direction(ab, ao))
            {
                simplex->size = 2;
                
                dm_vec3_cross_cross(ab, ao, ab, direction);
            }
            // must be in region 5
            else
            {
                simplex->size = 1;
                
                DM_VEC3_COPY(direction, ao);
            }
        }
        // origin must be in triangle
        else
        {
            // are we above plane? (region 2)
            if(dm_vec3_same_direction(abc, ao)) 
            {
                dm_memcpy(direction, abc, sizeof(abc));
            }
            // below plane (region 3)
            else
            {
                DM_VEC3_COPY(simplex->points[1], c);
                DM_VEC3_COPY(simplex->points[2], b);
                simplex->size = 3;
                
                dm_vec3_negate(abc, direction);
            }
        }
    }
    
    return false;
}

// series of triangle checks
bool dm_simplex_tetrahedron(float direction[3], dm_simplex* simplex)
{
    float a[3], b[3], c[3], d[3];
    float ab[3], ac[3], ad[3], ao[3];
    float abc[3], acd[3], adb[3];
    
    DM_VEC3_COPY(a, simplex->points[0]);
    DM_VEC3_COPY(b, simplex->points[1]);
    DM_VEC3_COPY(c, simplex->points[2]);
    DM_VEC3_COPY(d, simplex->points[3]);
    
    dm_vec3_sub_vec3(b, a, ab);
    dm_vec3_sub_vec3(c, a, ac);
    dm_vec3_sub_vec3(d, a, ad);
    dm_vec3_negate(a, ao);
    
    dm_vec3_cross(ab, ac, abc);
    dm_vec3_cross(ac, ad, acd);
    dm_vec3_cross(ad, ab, adb);
    
    if(dm_vec3_same_direction(abc, ao))
    {
        DM_VEC3_COPY(direction, abc);
        
        return false;
    }
    else if(dm_vec3_same_direction(acd, ao))
    {
        DM_VEC3_COPY(simplex->points[0], a);
        DM_VEC3_COPY(simplex->points[1], c);
        DM_VEC3_COPY(simplex->points[2], d);
        simplex->size = 3;
        
        DM_VEC3_COPY(direction, acd);
        
        return false;
    }
    else if(dm_vec3_same_direction(adb, ao))
    {
        DM_VEC3_COPY(simplex->points[0], a);
        DM_VEC3_COPY(simplex->points[1], d);
        DM_VEC3_COPY(simplex->points[2], b);
        simplex->size = 3;
        
        DM_VEC3_COPY(direction, adb);
        
        return false;
    }
    
    return true;
}

bool dm_next_simplex(float direction[3], dm_simplex* simplex)
{
    switch(simplex->size)
    {
        case 2: 
        return dm_simplex_line(direction, simplex);
        case 3: 
        return dm_simplex_triangle(direction, simplex);
        case 4: 
        return dm_simplex_tetrahedron(direction, simplex);
        default: 
        return false;
    }
}

bool dm_physics_gjk(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const dm_collision_shape shapes[2], float supports[2][3], dm_simplex* simplex)
{
    // initial guess should not matter, but algorithm WILL FAIL if initial guess is
    // perfectly aligned with global unit axes
    
    float direction[3] = { 0 };
    float sep[3];
    dm_vec3_sub_vec3(pos[0], pos[1], sep);
    if(sep[1]==0 && sep[2]==0) direction[1] = 1;
    else direction[0] = 1;
    
    // start simplex
    float support[3];
    
    dm_physics_support(pos, rots, cens, internals, shapes, direction, support, supports);
    dm_simplex_push_front(support, simplex);
    dm_vec3_negate(support, direction);
    
    for(uint32_t iter=0; iter<DM_PHYSICS_MAX_GJK_ITER; iter++)
    {
        if(simplex->size > 4) DM_LOG_ERROR("GJK simplex size greater than 4...?");
        
        dm_physics_support(pos, rots, cens, internals, shapes, direction, support, supports);
        if(dm_vec3_dot(support, direction) < 0 ) return false;
        
        dm_simplex_push_front(support, simplex);
        
        if(dm_next_simplex(direction, simplex)) return true;
    }
    
    DM_LOG_ERROR("GJK failed to converge after %u iterations", DM_PHYSICS_MAX_GJK_ITER);
    return false;
}

void dm_triangle_normal(float triangle[3][3], float normal[3])
{
    float ab[3], ac[3];
    
    dm_vec3_sub_vec3(triangle[1], triangle[0], ab);
    dm_vec3_sub_vec3(triangle[2], triangle[0], ac);
    dm_vec3_cross(ab, ac, normal);
    dm_vec3_norm(normal, normal);
}

/**********************************************************
// EPA
//https://github.com/kevinmoran/GJK/blob/master/GJK.h
//https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
*******************************************************************/
void dm_physics_epa(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const dm_collision_shape shapes[2], float penetration[3], float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3], float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3], uint32_t* polytope_count, dm_simplex* simplex)
{
    float faces[DM_PHYSICS_EPA_MAX_FACES][3][3] = { 0 };
    float normals[DM_PHYSICS_EPA_MAX_FACES][3]  = { 0 };
    
    float a[3], b[3], c[3], d[3];
    float dum1[3], dum2[3];
    
    DM_VEC3_COPY(a, simplex->points[0]);
    DM_VEC3_COPY(b, simplex->points[1]);
    DM_VEC3_COPY(c, simplex->points[2]);
    DM_VEC3_COPY(d, simplex->points[3]);
    
    // face a,b,c
    DM_VEC3_COPY(faces[0][0], a);
    DM_VEC3_COPY(faces[0][1], b);
    DM_VEC3_COPY(faces[0][2], c);
    dm_triangle_normal(faces[0], normals[0]);
    
    // face a,c,d
    DM_VEC3_COPY(faces[1][0], a);
    DM_VEC3_COPY(faces[1][1], c);
    DM_VEC3_COPY(faces[1][2], d);
    dm_triangle_normal(faces[1], normals[1]);
    
    // face a,d,b
    DM_VEC3_COPY(faces[2][0], a);
    DM_VEC3_COPY(faces[2][1], d);
    DM_VEC3_COPY(faces[2][2], b);
    dm_triangle_normal(faces[2], normals[2]);
    
    // face b,d,c
    DM_VEC3_COPY(faces[3][0], b);
    DM_VEC3_COPY(faces[3][1], d);
    DM_VEC3_COPY(faces[3][2], c);
    dm_triangle_normal(faces[3], normals[3]);
    
    /////////////////////////
    uint32_t num_faces    = 4;
    uint32_t num_loose    = 0;
    uint32_t closest_face = 0;
    
    float    min_distance, distance;
    
    float    support[3]                                  = { 0 };
    float    supports[2][3]                              = { 0 };
    float    loose_edges[DM_PHYSICS_EPA_MAX_FACES][2][3] = { 0 };
    float    current_edge[2][3]                          = { 0 };
    
    bool     remove = false;
    //////////////////////////
    for(uint32_t iter=0; iter<DM_PHYSICS_EPA_MAX_FACES; iter++)
    {
        assert(num_faces);
        
        // get closest face to origin
        min_distance = FLT_MAX;
        closest_face = 0;
        for(uint32_t i=0; i<num_faces; i++)
        {
            distance = dm_vec3_dot(faces[i][0], normals[i]);
            if(distance >= min_distance) continue;
            
            min_distance = distance;
            closest_face = i;
        }
        
        dm_physics_support(pos, rots, cens, internals, shapes, normals[closest_face], support, supports);
        
        // have we converged
        const float dot_d = dm_vec3_dot(support, normals[closest_face]);
        if(dot_d - min_distance < DM_PHYSICS_EPA_TOLERANCE)
        {
            float depth = dm_vec3_dot(support, normals[closest_face]);
            dm_vec3_scale(normals[closest_face], depth, penetration);
            
            dm_memcpy(polytope,         faces,   DM_VEC3_SIZE * num_faces * 3);
            dm_memcpy(polytope_normals, normals, DM_VEC3_SIZE * num_faces);
            *polytope_count = num_faces;
            
            return;
        }
        
        // revamp polytope
        num_loose = 0;
        for(uint32_t i=0; i<num_faces; i++)
        {
            dm_vec3_sub_vec3(support, faces[i][0], dum1);
            if(!dm_vec3_same_direction(normals[i], dum1)) continue;
            
            for(uint32_t j=0; j<3; j++)
            {
                DM_VEC3_COPY(current_edge[0], faces[i][j]);
                DM_VEC3_COPY(current_edge[1], faces[i][(j+1)%3]);
                bool found = false;
                
                for(uint32_t k=0; k<num_loose; k++)
                {
                    bool cond1 = dm_vec3_equals_vec3(loose_edges[k][1], current_edge[0]);
                    bool cond2 = dm_vec3_equals_vec3(loose_edges[k][0], current_edge[1]);
                    remove = cond1 && cond2;
                    if(!remove) continue;
                    
                    DM_VEC3_COPY(loose_edges[k][0], loose_edges[num_loose-1][0]);
                    DM_VEC3_COPY(loose_edges[k][1], loose_edges[num_loose-1][1]);
                    num_loose--;
                    found = true;
                    k = num_loose;
                }
                
                if(found) continue;
                if(num_loose >= DM_PHYSICS_EPA_MAX_FACES) break;
                
                DM_VEC3_COPY(loose_edges[num_loose][0], current_edge[0]);
                DM_VEC3_COPY(loose_edges[num_loose][1], current_edge[1]);
                num_loose++;
            }
            
            // triangle is visible, remove it
            DM_VEC3_COPY(faces[i][0], faces[num_faces-1][0]);
            DM_VEC3_COPY(faces[i][1], faces[num_faces-1][1]);
            DM_VEC3_COPY(faces[i][2], faces[num_faces-1][2]);
            DM_VEC3_COPY(normals[i],  normals[num_faces-1]);
            num_faces--;
            i--;
        }
        
        for(uint32_t i=0; i<num_loose; i++)
        {
            DM_VEC3_COPY(faces[num_faces][0], loose_edges[i][0]);
            DM_VEC3_COPY(faces[num_faces][1], loose_edges[i][1]);
            DM_VEC3_COPY(faces[num_faces][2], support);
            
            dm_vec3_sub_vec3(faces[num_faces][0], faces[num_faces][1], dum1);
            dm_vec3_sub_vec3(faces[num_faces][0], faces[num_faces][2], dum2);
            dm_vec3_cross(dum1, dum2, normals[num_faces]);
            dm_vec3_norm(normals[num_faces], normals[num_faces]);
            
            static const float bias = 0.00001f;
            if(dm_vec3_dot(faces[num_faces][0], normals[num_faces])+bias < 0)
            {
                float temp[3];
                DM_VEC3_COPY(temp, faces[num_faces][0]);
                DM_VEC3_COPY(faces[num_faces][0], faces[num_faces][1]);
                DM_VEC3_COPY(faces[num_faces][1], temp);
                dm_vec3_negate(normals[num_faces], normals[num_faces]);
            }
            
            num_faces++;
        }
    }
    
    DM_LOG_ERROR("EPA failed to converge after %u iterations", DM_PHYSICS_EPA_MAX_FACES);
    float depth = dm_vec3_dot(faces[closest_face][0], normals[closest_face]);
    dm_vec3_scale(normals[closest_face], depth, penetration);
    
    dm_memcpy(polytope,         faces,   sizeof(float) * 3 * 3 * num_faces);
    dm_memcpy(polytope_normals, normals, sizeof(float) * 3 * num_faces);
    *polytope_count = num_faces;
}

void dm_physics_draw_polytope(float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3], float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3], uint32_t num_faces, float normal[3], dm_context* context)
{
    float offset[3]  = { 0,0,0 };
    float o_color[4] = { 1,0,1,1 };
    
    float w = 0.1f;
    float h = 0.1f;
    
    debug_render_bilboard(offset, w,h, o_color, context);
    float dummy[3], dummy2[3], dummy3[3];
    
    dm_vec3_add_vec3(offset, normal, dummy);
    debug_render_line(offset, dummy, o_color, context);
    
    static uint32_t draw_index = 0;
    
    if(dm_input_key_just_pressed(DM_KEY_P, context)) draw_index++;
    else if(dm_input_key_just_pressed(DM_KEY_O, context)) draw_index--;
    
    draw_index = DM_CLAMP(draw_index, 0, num_faces);
    
    for(uint32_t j=0; j<num_faces; j++)
    {
        float f = (float)j / (float)num_faces;
        
        float c[4] = {
            1 - f,
            0,
            f,
            1
        };
        
        // add little offset so we can distinguish each face
        float normal_scale[3];
        dm_vec3_scale(polytope_normals[j], 0.1f, normal_scale);
        
        dm_vec3_add_vec3(polytope[j][0], offset, dummy);
        dm_vec3_add_vec3(dummy, normal_scale, dummy);
        dm_vec3_add_vec3(polytope[j][1], offset, dummy2);
        dm_vec3_add_vec3(dummy2, normal_scale, dummy2);
        dm_vec3_add_vec3(polytope[j][2], offset, dummy3);
        dm_vec3_add_vec3(dummy3, normal_scale, dummy3);
        
        debug_render_bilboard(dummy,  w,h, c, context);
        debug_render_bilboard(dummy2, w,h, c, context);
        debug_render_bilboard(dummy3, w,h, c, context);
        
        debug_render_line(dummy,  dummy2, c, context);
        debug_render_line(dummy,  dummy3, c, context);
        debug_render_line(dummy2, dummy3, c, context);
    }
}

/*
Collision resolution
*/
void dm_support_face_box_planes(dm_plane planes[5], float points[10][3], float face_normal[3])
{
    uint32_t ids[] = { 1,2,3,0 };
    float neg_normal[3];
    dm_vec3_negate(face_normal, neg_normal);
    
    float ref_pt[3], normal[3];
    float dum1[3], dum2[3];
    float distance;
    
    for(uint32_t i=0; i<4; i++)
    {
        DM_VEC3_COPY(ref_pt, points[i]);
        
        dm_vec3_sub_vec3(points[ids[i]], ref_pt, dum1);
        dm_vec3_cross(dum1, neg_normal, dum2);
        dm_vec3_norm(dum2, normal);
        distance = -dm_vec3_dot(normal, ref_pt);
        
        DM_VEC3_COPY(planes[i+1].normal, normal);
        planes[i+1].distance=distance;
    }
}

void dm_support_face_box(const float pos[3], const float rot[4], const float cen[3], const float internals[6], float direction[3], float points[10][3], uint32_t* num_pts, dm_plane planes[5], uint32_t* num_planes, float normal[3])
{
    const float box_min[3] = {
        internals[0],
        internals[1],
        internals[2],
    };
    
    const float box_max[] = {
        internals[3],
        internals[4],
        internals[5],
    };
    
    float inv_rot[4];
    DM_VEC4_COPY(inv_rot, rot);
    dm_quat_inverse(inv_rot, inv_rot);
    dm_vec3_rotate(direction, inv_rot, direction);
    
    float axes[][3] = {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };
    
    float best_proximity = -FLT_MAX;
    float best_sgn = 0.0f;
    int   best_axis = -1;
    
    for(uint32_t i=0; i<3; i++)
    {
        float proximity = dm_vec3_dot(direction, axes[i]);
        float s = DM_SIGNF(proximity);
        proximity *= s;
        if(proximity <= best_proximity) continue;
        
        best_proximity = proximity;
        best_sgn = s;
        best_axis = i;
    }
    
    switch(best_axis)
    {
        case 0:
        {
            if(best_sgn > 0)
            {
                points[0][0] = box_max[0]; points[0][1] = box_min[1]; points[0][2] = box_min[2];
                points[1][0] = box_max[0]; points[1][1] = box_min[1]; points[1][2] = box_max[2];
                points[2][0] = box_max[0]; points[2][1] = box_max[1]; points[2][2] = box_max[2];
                points[3][0] = box_max[0]; points[3][1] = box_max[1]; points[3][2] = box_min[2];
            }
            else
            {
                points[0][0] = box_min[0]; points[0][1] = box_min[1]; points[0][2] = box_max[2];
                points[1][0] = box_min[0]; points[1][1] = box_min[1]; points[1][2] = box_min[2];
                points[2][0] = box_min[0]; points[2][1] = box_max[1]; points[2][2] = box_min[2];
                points[3][0] = box_min[0]; points[3][1] = box_max[1]; points[3][2] = box_max[2];
            }
        } break;
        
        case 1:
        {
            if(best_sgn > 0)
            {
                points[0][0] = box_min[0]; points[0][1] = box_max[1]; points[0][2] = box_min[2];
                points[1][0] = box_max[0]; points[1][1] = box_max[1]; points[1][2] = box_min[2];
                points[2][0] = box_max[0]; points[2][1] = box_max[1]; points[2][2] = box_max[2];
                points[3][0] = box_min[0]; points[3][1] = box_max[1]; points[3][2] = box_max[2];
            }
            else
            {
                points[0][0] = box_max[0]; points[0][1] = box_min[1]; points[0][2] = box_min[2];
                points[1][0] = box_min[0]; points[1][1] = box_min[1]; points[1][2] = box_min[2];
                points[2][0] = box_min[0]; points[2][1] = box_min[1]; points[2][2] = box_max[2];
                points[3][0] = box_max[0]; points[3][1] = box_min[1]; points[3][2] = box_max[2];
            }
        } break;
        
        case 2:
        {
            if(best_sgn > 0)
            {
                points[0][0] = box_max[0]; points[0][1] = box_min[1]; points[0][2] = box_max[2];
                points[1][0] = box_min[0]; points[1][1] = box_min[1]; points[1][2] = box_max[2];
                points[2][0] = box_min[0]; points[2][1] = box_max[1]; points[2][2] = box_max[2];
                points[3][0] = box_max[0]; points[3][1] = box_max[1]; points[3][2] = box_max[2];
            }
            else
            {
                points[0][0] = box_min[0]; points[0][1] = box_min[1]; points[0][2] = box_min[2];
                points[1][0] = box_max[0]; points[1][1] = box_min[1]; points[1][2] = box_min[2];
                points[2][0] = box_max[0]; points[2][1] = box_max[1]; points[2][2] = box_min[2];
                points[3][0] = box_min[0]; points[3][1] = box_max[1]; points[3][2] = box_min[2];
            }
        } break;
    }
    
    // get points into world frame
    for(uint32_t i=0; i<4; i++)
    {
        dm_vec3_rotate(points[i], rot, points[i]);
        dm_vec3_add_vec3(points[i], pos, points[i]);
    }
    
    dm_vec3_scale(axes[best_axis], best_sgn, normal);
    dm_vec3_rotate(normal, rot, normal);
    dm_vec3_norm(normal, normal);
    
    DM_VEC3_COPY(planes[0].normal, normal);
    planes[0].distance = -dm_vec3_dot(planes[0].normal, points[0]);
    
    dm_support_face_box_planes(planes, points, normal);
    
    *num_pts    = 4;
    *num_planes = 5;
}

bool dm_physics_point_in_plane(float point[3], dm_plane plane)
{
    float t = plane.distance + dm_vec3_dot(point, plane.normal);
    
    return t < -DM_PHYSICS_TEST_EPSILON;
}

void dm_physics_plane_edge_intersect(dm_plane plane, float start[3], float end[3], float out[3])
{
    float ab[3];
    dm_vec3_sub_vec3(end, start, ab);
    float ab_d = dm_vec3_dot(plane.normal, ab);
    
    if(dm_fabs(ab_d) <= DM_PHYSICS_TEST_EPSILON)
    {
        DM_VEC3_COPY(out, start);
    }
    else
    {
        float p[3], w[3];
        dm_vec3_scale(plane.normal, -plane.distance, p);
        dm_vec3_sub_vec3(start, p, w);
        
        float fac = -dm_vec3_dot(plane.normal, w) / ab_d;
        dm_vec3_scale(ab, fac, ab);
        dm_vec3_add_vec3(start, ab, ab);
        
        DM_VEC3_COPY(out, ab);
    }
}

void dm_physics_sutherland_hodgman(float input_face[10][3], uint32_t num_input, dm_plane clip_planes[5], uint32_t num_planes, float output_face[10][3], uint32_t* num_output)
{
    float    input[10][3] = { 0 };
    uint32_t input_count = 0;
    
    float    output[10][3] = { 0 };
    dm_memcpy(output, input_face, DM_VEC3_SIZE * num_input);
    uint32_t output_count = num_input;
    
    float start[3], end[3];
    
    for(uint32_t i=0; i<num_planes; i++)
    {
        if(output_count==0) break;
        
        // swap blocks
        dm_memcpy(input, output, sizeof(output));
        input_count = output_count;
        dm_memzero(output, sizeof(output));
        output_count = 0;
        
        const dm_plane plane = clip_planes[i];
        DM_VEC3_COPY(start, input[input_count-1]);
        
        for(uint32_t j=0; j<input_count; j++)
        {
            DM_VEC3_COPY(end, input[j]);
            
            bool start_in_plane = dm_physics_point_in_plane(start, plane);
            bool end_in_plane   = dm_physics_point_in_plane(end, plane);
            
            if(start_in_plane && end_in_plane)
            {
                DM_VEC3_COPY(output[output_count++], end);
            }
            else if(start_in_plane && !end_in_plane)
            {
                float new_p[3];
                dm_physics_plane_edge_intersect(plane, start, end, new_p);
                DM_VEC3_COPY(output[output_count++], new_p);
            }
            else if(!start_in_plane && end_in_plane)
            {
                float new_p[3];
                dm_physics_plane_edge_intersect(plane, end, start, new_p);
                DM_VEC3_COPY(output[output_count++], new_p);
                DM_VEC3_COPY(output[output_count++], end);
            }
            
            DM_VEC3_COPY(start, end);
        }
    }
    
    dm_memcpy(output_face, output, sizeof(output));
    *num_output = output_count;
}

/*************
CONTACT POINT
***************/
void dm_physics_init_constraint(float vec[3], float r_a[3], float r_b[3], float b, float impulse_min, float impulse_max, dm_contact_constraint* constraint)
{
    *constraint = (dm_contact_constraint){ 0 };
    
    dm_vec3_negate(vec, constraint->jacobian[0]);
    dm_vec3_cross(r_a, vec, constraint->jacobian[1]);
    dm_vec3_negate(constraint->jacobian[1], constraint->jacobian[1]);
    DM_VEC3_COPY(constraint->jacobian[2], vec);
    dm_vec3_cross(r_b, vec, constraint->jacobian[3]);
    constraint->b = b;
    constraint->impulse_min = impulse_min;
    constraint->impulse_max = impulse_max;
}

void dm_physics_add_contact_point(const float on_a[3], const float on_b[3], const float normal[3], const float depth, const float pos[2][3], const float rot[2][4], const float vel[2][3], const float w[2][3], dm_contact_manifold* manifold, dm_context* context)
{
    DM_QUAT_COPY(manifold->orientation_a, rot[0]);
    DM_QUAT_COPY(manifold->orientation_b, rot[1]);
    
    float dum1[3];
    
    dm_memcpy(manifold->normal, normal, DM_VEC3_SIZE);
    
    // get r vectors
    float r_a[3], r_b[3];
    
    dm_vec3_sub_vec3(on_a, pos[0], r_a);
    dm_vec3_sub_vec3(on_b, pos[1], r_b);
    
    // tangent vectors
    float v_a[3], v_b[3], rel_v[3];
    
    dm_vec3_cross(w[0], r_a, dum1);
    dm_vec3_add_vec3(vel[0], dum1, v_a);
    
    dm_vec3_cross(w[1], r_b, dum1);
    dm_vec3_add_vec3(vel[1], dum1, v_b);
    
    dm_vec3_sub_vec3(v_b, v_a, rel_v);
    float rel_vn = dm_vec3_dot(rel_v, manifold->normal);
    dm_vec3_scale(manifold->normal, rel_vn, dum1);
    dm_vec3_sub_vec3(rel_v, dum1, manifold->tangent_a);
    
    if(dm_vec3_dot(manifold->tangent_a, manifold->tangent_a) < 0.001f)
    {
        float x[3] = { 1,0,0 };
        dm_vec3_cross(manifold->normal, x, manifold->tangent_a);
        if(dm_vec3_dot(manifold->tangent_a, manifold->tangent_a) < 0.001f)
        {
            float z[3] = { 0,0,1 };
            dm_vec3_cross(manifold->normal, z, manifold->tangent_a);
            if(dm_vec3_dot(manifold->tangent_a, manifold->tangent_a) < 0.001f)
            {
                float y[3] = { 0,1,0 };
                dm_vec3_cross(manifold->normal, y, manifold->tangent_a);
            }
        }
    }
    
    dm_vec3_norm(manifold->tangent_a, manifold->tangent_a);
    dm_vec3_cross(manifold->normal, manifold->tangent_a, manifold->tangent_b);
    
    float b = 0.0f;
    
    // baumgarte offset
    {
        float neg_norm[3];
        dm_vec3_negate(manifold->normal, neg_norm);
        
#define DM_PHYSICS_BAUMGARTE_COEF 0.3f
#define DM_PHYSICS_BAUMGARTE_SLOP 0.001f
        float ba[3];
        dm_vec3_sub_vec3(on_b, on_a, ba);
        float d = dm_vec3_dot(ba, neg_norm);
        
        b -= (DM_PHYSICS_BAUMGARTE_COEF * DM_PHYSICS_FIXED_DT_INV) * d;
    }
    
    // restitution
    {
#define DM_PHYSICS_REST_COEF 0.5f
#define DM_PHYSICS_REST_SLOP 0.5f

        b += (DM_PHYSICS_REST_COEF * rel_vn); 
    }
    
    // point position data
    float inv_rot[4];
    dm_contact_point p = { 0 };
    DM_VEC3_COPY(p.global_pos[0], on_a);
    DM_VEC3_COPY(p.global_pos[1], on_b);
    dm_quat_inverse(manifold->orientation_a, inv_rot);
    dm_vec3_rotate(r_a, inv_rot, p.local_pos[0]);
    dm_quat_inverse(manifold->orientation_b, inv_rot);
    dm_vec3_rotate(r_b, inv_rot, p.local_pos[1]);
    p.penetration = depth;
    
    // normal constraint
    dm_physics_init_constraint(manifold->normal,    r_a, r_b, b, 0, FLT_MAX, &p.normal);
    
    // friction a constraint
    dm_physics_init_constraint(manifold->tangent_a, r_a, r_b, 0, -FLT_MAX, FLT_MAX, &p.friction_a);
    
    // friction b constraint
    dm_physics_init_constraint(manifold->tangent_b, r_a, r_b, 0, -FLT_MAX, FLT_MAX, &p.friction_b);
    
    if(manifold->point_count>20)
    {
        DM_LOG_ERROR("REALLY BAD");
        return;
    }
    
    manifold->points[manifold->point_count++] = p;
}

// support face funcs
void dm_support_face_entity(const float pos[3], const float rot[4], const float cen[3], const float internals[6], const dm_collision_shape shape, float direction[3], float points[10][3], uint32_t* num_pts, dm_plane planes[5], uint32_t* num_planes, float normal[3])
{
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE:
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        dm_support_face_box(pos, rot, cen, internals, direction, points, num_pts, planes, num_planes, normal);
        break;
        
        default:
        DM_LOG_ERROR("Unknown or unsupported collider type");
        return;
    }
}

bool dm_physics_collide_sphere_other(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex)
{
    switch(shapes[1])
    {
        case DM_COLLISION_SHAPE_SPHERE:
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
    
    return true;
}

void dm_physics_collide_poly_sphere(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_context* context)
{
#if 0 
    float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3]      = { 0 };
    float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3] = { 0 };
    uint32_t num_faces;
    float penetration[3] = { 0 };
    
    dm_physics_epa(pos, rots, cens, internals, shapes, penetration, polytope, polytope_normals, &num_faces, simplex);
    
    float    points_box[10][3];
    uint32_t num_pts;
    float    normal_box[3];
    dm_plane planes[5];
    uint32_t num_planes;
#endif
}

void dm_physics_collide_poly_poly(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold, dm_context* context)
{
    float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3]      = { 0 };
    float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3] = { 0 };
    uint32_t num_faces;
    float penetration[3] = { 0 };
    
    dm_physics_epa(pos, rots, cens, internals, shapes, penetration, polytope, polytope_normals, &num_faces, simplex);
    
    //dm_physics_draw_polytope(polytope, polytope_normals, num_faces, penetration, context);
    
    if(dm_vec3_mag(penetration)==0) return;
    
    float    points_a[10][3];
    uint32_t num_pts_a;
    float    normal_a[3];
    dm_plane planes_a[5];
    uint32_t num_planes_a;
    
    float    points_b[10][3];
    uint32_t num_pts_b;
    float    normal_b[3];
    dm_plane planes_b[5];
    uint32_t num_planes_b;
    
    float norm_pen[3];
    float neg_pen[3];
    
    dm_vec3_norm(penetration, norm_pen);
    dm_vec3_negate(norm_pen, neg_pen);
    float pen_depth = dm_vec3_mag(penetration);
    
    dm_support_face_entity(pos[0], rots[0], cens[0], internals[0], shapes[0], norm_pen, points_a, &num_pts_a, planes_a, &num_planes_a, normal_a);
    dm_support_face_entity(pos[1], rots[1], cens[1], internals[1], shapes[1], neg_pen, points_b, &num_pts_b, planes_b, &num_planes_b, normal_b);
    
#if 0
    float dum[3];
    for(uint32_t i=0; i<num_pts_a; i++)
    {
        debug_render_bilboard(points_a[i], 0.15f,0.15f, (float[]){ 1,0,0,1 }, context);
    }
    for(uint32_t i=0; i<num_pts_b; i++)
    {
        debug_render_bilboard(points_b[i], 0.15f,0.15f, (float[]){ 1,1,0,1 }, context);
    }
#endif
    
    bool flipped = dm_fabs(dm_vec3_dot(norm_pen, normal_a)) > dm_fabs(dm_vec3_dot(norm_pen, normal_b));
    
    dm_plane ref_plane;
    float    clipped_face[10][3];
    uint32_t num_clipped = 0;
    
    if(flipped) 
    {
        dm_memcpy(ref_plane.normal, normal_b, sizeof(normal_b));
        ref_plane.distance = dm_vec3_dot(normal_b, points_b[0]);
        
        dm_physics_sutherland_hodgman(points_a, num_pts_a, planes_b, num_planes_b, clipped_face, &num_clipped);
    }
    else
    {
        dm_memcpy(ref_plane.normal, normal_a, sizeof(normal_a));
        ref_plane.distance = dm_vec3_dot(normal_a, points_a[0]);
        
        dm_physics_sutherland_hodgman(points_b, num_pts_b, planes_a, num_planes_a, clipped_face, &num_clipped);
    }
    
    for(uint32_t i=0; i<num_clipped; i++)
    {
        float point[3];
        dm_memcpy(point, clipped_face[i], sizeof(point));
        
        float on_a[3], on_b[3], dum[3];
        
        float contact_pen = -dm_fabs(dm_vec3_dot(point, ref_plane.normal) + ref_plane.distance);
        contact_pen = DM_MIN(contact_pen, pen_depth);
        
        dm_vec3_scale(ref_plane.normal, contact_pen, dum);
        if(flipped)
        {
            dm_vec3_sub_vec3(point, dum, on_a);
            dm_memcpy(on_b, point, sizeof(point));
        }
        else
        {
            dm_vec3_add_vec3(point, dum, on_b);
            dm_memcpy(on_a, point, sizeof(point));
        }
        
        contact_pen = -(dm_fabs(contact_pen) + pen_depth) * 0.5f;
        
        dm_physics_add_contact_point(on_a, on_b, norm_pen, contact_pen, pos, rots, vels, ws, manifold, context);
    }
}

bool dm_physics_collide_poly_other(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold, dm_context* context)
{
    switch(shapes[1])
    {
        case DM_COLLISION_SHAPE_SPHERE:
        dm_physics_collide_poly_sphere(pos, rots, cens, internals, vels, ws, shapes, simplex, context);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        dm_physics_collide_poly_poly(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold, context);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
    
    return true;
}

bool dm_physics_collide_entities(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold, dm_context* context)
{
    switch(shapes[0])
    {
        case DM_COLLISION_SHAPE_SPHERE:
        return dm_physics_collide_sphere_other(pos, rots, cens, internals, vels, ws, shapes, simplex);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        return dm_physics_collide_poly_other(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold, context);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
}

// narrowphase
bool dm_physics_narrowphase(dm_ecs_system_manager* system, dm_physics_manager* manager, dm_context* context)
{
    const dm_ecs_id t_id = context->ecs_manager.default_components.transform;
    const dm_ecs_id c_id = context->ecs_manager.default_components.collision;
    const dm_ecs_id p_id = context->ecs_manager.default_components.physics;
    
    dm_component_transform_block* t_block = context->ecs_manager.components[t_id].data;
    dm_component_collision_block* c_block = context->ecs_manager.components[c_id].data;
    dm_component_physics_block*   p_block = context->ecs_manager.components[p_id].data;
    
    float              pos[2][3], rots[2][4], cens[2][3], internals[2][6], vels[2][3], ws[2][3];
    dm_collision_shape shapes[2];
    
    uint32_t a_t_c_index, a_c_c_index, a_p_c_index;
    uint32_t b_t_c_index, b_c_c_index, b_p_c_index;
    
    dm_component_transform_block* a_t_block, *b_t_block;
    dm_component_collision_block* a_c_block, *b_c_block;
    dm_component_physics_block*   a_p_block, *b_p_block;
    
    dm_collision_pair collision_pair;
    dm_ecs_system_entity_container entity_a, entity_b;
    
    dm_contact_manifold* manifold;
    
    dm_simplex simplex;
    
    float load;
    
    for(uint32_t i=0; i<manager->num_possible_collisions; i++)
    {
        collision_pair = manager->possible_collisions[i];
        
        entity_a = collision_pair.entity_a;
        entity_b = collision_pair.entity_b;
        
        // get all data
        
        // entity a
        a_t_c_index = entity_a.component_indices[t_id];
        a_c_c_index = entity_a.component_indices[c_id];
        a_p_c_index = entity_a.component_indices[p_id];
        
        a_t_block = t_block + entity_a.block_indices[t_id];;
        a_c_block = c_block + entity_a.block_indices[c_id];;
        a_p_block = p_block + entity_a.block_indices[p_id];;
        
        pos[0][0]       = a_t_block->pos_x[a_t_c_index];
        pos[0][1]       = a_t_block->pos_y[a_t_c_index];
        pos[0][2]       = a_t_block->pos_z[a_t_c_index];
        rots[0][0]      = a_t_block->rot_i[a_t_c_index];
        rots[0][1]      = a_t_block->rot_j[a_t_c_index];
        rots[0][2]      = a_t_block->rot_k[a_t_c_index];
        rots[0][3]      = a_t_block->rot_r[a_t_c_index];
        cens[0][0]      = a_c_block->center_x[a_c_c_index];
        cens[0][1]      = a_c_block->center_y[a_c_c_index];
        cens[0][2]      = a_c_block->center_z[a_c_c_index];
        internals[0][0] = a_c_block->internal_0[a_c_c_index];
        internals[0][1] = a_c_block->internal_1[a_c_c_index];
        internals[0][2] = a_c_block->internal_2[a_c_c_index];
        internals[0][3] = a_c_block->internal_3[a_c_c_index];
        internals[0][4] = a_c_block->internal_4[a_c_c_index];
        internals[0][5] = a_c_block->internal_5[a_c_c_index];
        shapes[0]       = a_c_block->shape[a_c_c_index];
        vels[0][0]      = a_p_block->vel_x[a_p_c_index];
        vels[0][1]      = a_p_block->vel_y[a_p_c_index];
        vels[0][2]      = a_p_block->vel_z[a_p_c_index];
        ws[0][0]        = a_p_block->w_x[a_p_c_index];
        ws[0][1]        = a_p_block->w_y[a_p_c_index];
        ws[0][2]        = a_p_block->w_z[a_p_c_index];
        
        // entity b
        b_t_c_index = entity_b.component_indices[t_id];
        b_c_c_index = entity_b.component_indices[c_id];
        b_p_c_index = entity_b.component_indices[p_id];
        
        b_t_block = t_block + entity_b.block_indices[t_id];;
        b_c_block = c_block + entity_b.block_indices[c_id];;
        b_p_block = p_block + entity_b.block_indices[p_id];;
        
        pos[1][0]       = b_t_block->pos_x[b_t_c_index];
        pos[1][1]       = b_t_block->pos_y[b_t_c_index];
        pos[1][2]       = b_t_block->pos_z[b_t_c_index];
        rots[1][0]      = b_t_block->rot_i[b_t_c_index];
        rots[1][1]      = b_t_block->rot_j[b_t_c_index];
        rots[1][2]      = b_t_block->rot_k[b_t_c_index];
        rots[1][3]      = b_t_block->rot_r[b_t_c_index];
        cens[1][0]      = b_c_block->center_x[b_c_c_index];
        cens[1][1]      = b_c_block->center_y[b_c_c_index];
        cens[1][2]      = b_c_block->center_z[b_c_c_index];
        internals[1][0] = b_c_block->internal_0[b_c_c_index];
        internals[1][1] = b_c_block->internal_1[b_c_c_index];
        internals[1][2] = b_c_block->internal_2[b_c_c_index];
        internals[1][3] = b_c_block->internal_3[b_c_c_index];
        internals[1][4] = b_c_block->internal_4[b_c_c_index];
        internals[1][5] = b_c_block->internal_5[b_c_c_index];
        shapes[1]       = b_c_block->shape[b_c_c_index];
        vels[1][0]      = b_p_block->vel_x[b_p_c_index];
        vels[1][1]      = b_p_block->vel_y[b_p_c_index];
        vels[1][2]      = b_p_block->vel_z[b_p_c_index];
        ws[1][0]        = b_p_block->w_x[b_p_c_index];
        ws[1][1]        = b_p_block->w_y[b_p_c_index];
        ws[1][2]        = b_p_block->w_z[b_p_c_index];
        
        //////
        simplex = (dm_simplex){ 0 };
        
        float supports[2][3];
        if(!dm_physics_gjk(pos, rots, cens, internals, shapes, supports, &simplex)) continue;
        
        assert(simplex.size==4);
        
#if 0 
        debug_render_bilboard((float[]){0,0,0}, 0.1f,0.1f, (float[]){ 1,0,1,0.75 }, context);
        
        float color[4] = { 1,1,1,0.75f };
        debug_render_bilboard(simplex.points[0], 0.1f,0.1f, color, context);
        debug_render_bilboard(simplex.points[1], 0.1f,0.1f, color, context);
        debug_render_bilboard(simplex.points[2], 0.1f,0.1f, color, context);
        debug_render_bilboard(simplex.points[3], 0.1f,0.1f, color, context);
        
        debug_render_line(simplex.points[0], simplex.points[1], color, context);
        debug_render_line(simplex.points[0], simplex.points[2], color, context);
        debug_render_line(simplex.points[0], simplex.points[3], color, context);
        
        debug_render_line(simplex.points[1], simplex.points[2], color, context);
        debug_render_line(simplex.points[1], simplex.points[3], color, context);
        
        debug_render_line(simplex.points[2], simplex.points[3], color, context);
#endif
        
        manifold = &manager->manifolds[manager->num_manifolds++];
        *manifold = (dm_contact_manifold){ 0 };
        
        a_c_block->flag[a_c_c_index] = DM_COLLISION_FLAG_YES;
        
        manifold->contact_data[0].vel_x         = &a_p_block->vel_x[a_p_c_index];
        manifold->contact_data[0].vel_y         = &a_p_block->vel_y[a_p_c_index];
        manifold->contact_data[0].vel_z         = &a_p_block->vel_z[a_p_c_index];
        manifold->contact_data[0].w_x           = &a_p_block->w_x[a_p_c_index];
        manifold->contact_data[0].w_y           = &a_p_block->w_y[a_p_c_index];
        manifold->contact_data[0].w_z           = &a_p_block->w_z[a_p_c_index];
        manifold->contact_data[0].mass          = a_p_block->mass[a_p_c_index];
        manifold->contact_data[0].inv_mass      = a_p_block->inv_mass[a_p_c_index];
        manifold->contact_data[0].v_damp        = a_p_block->damping_v[a_p_c_index];
        manifold->contact_data[0].w_damp        = a_p_block->damping_w[a_p_c_index];
        manifold->contact_data[0].i_body_inv_00 = a_p_block->i_body_inv_00[a_p_c_index];
        manifold->contact_data[0].i_body_inv_11 = a_p_block->i_body_inv_11[a_p_c_index];
        manifold->contact_data[0].i_body_inv_22 = a_p_block->i_body_inv_22[a_p_c_index];
        
        b_c_block->flag[b_c_c_index] = DM_COLLISION_FLAG_YES;
        
        manifold->contact_data[1].vel_x         = &b_p_block->vel_x[b_p_c_index];
        manifold->contact_data[1].vel_y         = &b_p_block->vel_y[b_p_c_index];
        manifold->contact_data[1].vel_z         = &b_p_block->vel_z[b_p_c_index];
        manifold->contact_data[1].w_x           = &b_p_block->w_x[b_p_c_index];
        manifold->contact_data[1].w_y           = &b_p_block->w_y[b_p_c_index];
        manifold->contact_data[1].w_z           = &b_p_block->w_z[b_p_c_index];
        manifold->contact_data[1].mass          = b_p_block->mass[b_p_c_index];
        manifold->contact_data[1].inv_mass      = b_p_block->inv_mass[b_p_c_index];
        manifold->contact_data[1].v_damp        = b_p_block->damping_v[b_p_c_index];
        manifold->contact_data[1].w_damp        = b_p_block->damping_w[b_p_c_index];
        manifold->contact_data[1].i_body_inv_00 = b_p_block->i_body_inv_00[b_p_c_index];
        manifold->contact_data[1].i_body_inv_11 = b_p_block->i_body_inv_11[b_p_c_index];
        manifold->contact_data[1].i_body_inv_22 = b_p_block->i_body_inv_22[b_p_c_index];
        
        if(!dm_physics_collide_entities(pos, rots, cens, internals, vels, ws, shapes, &simplex, manifold, context)) return false;
        
        // resize manifolds
        load = (float)manager->num_manifolds / (float)manager->manifold_capacity;
        if(load < DM_PHYSICS_LOAD_FACTOR) continue;
        
        manager->manifold_capacity *= DM_PHYSICS_RESIZE_FACTOR;
        manager->manifolds = dm_realloc(manager->manifolds, sizeof(dm_contact_manifold) * manager->manifold_capacity);
    }
    
    return true;
}

/********************
COLLISION RESOLUTION
**********************/
void dm_physics_constraint_lambda(dm_contact_constraint* constraint, dm_contact_manifold* manifold)
{
    float effective_mass = 0;
    
    float i_body_inv_a[M3] = { 0 };
    float i_body_inv_b[M3] = { 0 };
    
    float dum1[N3], dum2[N3];
    
    float vel_a[3] = {
        *manifold->contact_data[0].vel_x,
        *manifold->contact_data[0].vel_y,
        *manifold->contact_data[0].vel_z,
    };
    
    float w_a[3] = {
        *manifold->contact_data[0].w_x,
        *manifold->contact_data[0].w_y,
        *manifold->contact_data[0].w_z,
    };
    
    float vel_b[3] = {
        *manifold->contact_data[1].vel_x,
        *manifold->contact_data[1].vel_y,
        *manifold->contact_data[1].vel_z,
    };
    
    float w_b[3] = {
        *manifold->contact_data[1].w_x,
        *manifold->contact_data[1].w_y,
        *manifold->contact_data[1].w_z,
    };
    
    i_body_inv_a[0] = manifold->contact_data[0].i_body_inv_00;
    i_body_inv_a[4] = manifold->contact_data[0].i_body_inv_11;
    i_body_inv_a[8] = manifold->contact_data[0].i_body_inv_22;
    
    i_body_inv_b[0] = manifold->contact_data[1].i_body_inv_00;
    i_body_inv_b[4] = manifold->contact_data[1].i_body_inv_11;
    i_body_inv_b[8] = manifold->contact_data[1].i_body_inv_22;
    
    dm_mat3_mul_vec3(i_body_inv_a, constraint->jacobian[1], dum1);
    dm_mat3_mul_vec3(i_body_inv_b, constraint->jacobian[3], dum2);
    
    // effective mass
    effective_mass += manifold->contact_data[0].inv_mass;
    effective_mass += dm_vec3_dot(constraint->jacobian[1], dum1);
    effective_mass += manifold->contact_data[1].inv_mass;
    effective_mass += dm_vec3_dot(constraint->jacobian[3], dum2);
    effective_mass  = 1.0f / effective_mass;
    
    // compute lambda
    constraint->lambda  = dm_vec3_dot(constraint->jacobian[0], vel_a);
    constraint->lambda += dm_vec3_dot(constraint->jacobian[1], w_a);
    constraint->lambda += dm_vec3_dot(constraint->jacobian[2], vel_b);
    constraint->lambda += dm_vec3_dot(constraint->jacobian[3], w_b);
    constraint->lambda += constraint->b;
    constraint->lambda *= -effective_mass;
}

void dm_physics_constraint_apply(dm_contact_constraint* constraint, dm_contact_manifold* manifold)
{
    float old_sum = constraint->impulse_sum;
    constraint->impulse_sum += constraint->lambda;
    constraint->impulse_sum = DM_CLAMP(constraint->impulse_sum, constraint->impulse_min, constraint->impulse_max);
    constraint->lambda = constraint->impulse_sum - old_sum;
    
    float delta_v[4][3]    = { 0 };
    float i_body_inv_a[M3] = { 0 };
    float i_body_inv_b[M3] = { 0 };
    float dum1[N3], dum2[N3];
    
    i_body_inv_a[0] = manifold->contact_data[0].i_body_inv_00;
    i_body_inv_a[4] = manifold->contact_data[0].i_body_inv_11;
    i_body_inv_a[8] = manifold->contact_data[0].i_body_inv_22;
    
    i_body_inv_b[0] = manifold->contact_data[1].i_body_inv_00;
    i_body_inv_b[4] = manifold->contact_data[1].i_body_inv_11;
    i_body_inv_b[8] = manifold->contact_data[1].i_body_inv_22;
    
    dm_mat3_mul_vec3(i_body_inv_a, constraint->jacobian[1], dum1);
    dm_mat3_mul_vec3(i_body_inv_b, constraint->jacobian[3], dum2);
    
    dm_vec3_scale(constraint->jacobian[0], manifold->contact_data[0].inv_mass * constraint->lambda, delta_v[0]);
    dm_vec3_scale(dum1, constraint->lambda, delta_v[1]);
    dm_vec3_scale(constraint->jacobian[2], manifold->contact_data[1].inv_mass * constraint->lambda, delta_v[2]);
    dm_vec3_scale(dum2, constraint->lambda, delta_v[3]);
    
    float v_damping_a = 1.0f / (1.0f + manifold->contact_data[0].v_damp * DM_PHYSICS_FIXED_DT);
    float w_damping_a = 1.0f / (1.0f + manifold->contact_data[0].w_damp * DM_PHYSICS_FIXED_DT);
    float v_damping_b = 1.0f / (1.0f + manifold->contact_data[1].v_damp * DM_PHYSICS_FIXED_DT);
    float w_damping_b = 1.0f / (1.0f + manifold->contact_data[1].w_damp * DM_PHYSICS_FIXED_DT);
    
    *manifold->contact_data[0].vel_x += delta_v[0][0] * v_damping_a;
    *manifold->contact_data[0].vel_y += delta_v[0][1] * v_damping_a;
    *manifold->contact_data[0].vel_z += delta_v[0][2] * v_damping_a;
    *manifold->contact_data[0].w_x   += delta_v[1][0] * w_damping_a;
    *manifold->contact_data[0].w_y   += delta_v[1][1] * w_damping_a;
    *manifold->contact_data[0].w_z   += delta_v[1][2] * w_damping_a;
    
    *manifold->contact_data[1].vel_x += delta_v[2][0] * v_damping_b;
    *manifold->contact_data[1].vel_y += delta_v[2][1] * v_damping_b;
    *manifold->contact_data[1].vel_z += delta_v[2][2] * v_damping_b;
    *manifold->contact_data[1].w_x   += delta_v[3][0] * w_damping_b;
    *manifold->contact_data[1].w_y   += delta_v[3][1] * w_damping_b;
    *manifold->contact_data[1].w_z   += delta_v[3][2] * w_damping_b;
}

void dm_physics_apply_constraints(dm_contact_manifold* manifold)
{
    for(uint32_t p=0; p<manifold->point_count; p++)
    {
        dm_contact_point* point = &manifold->points[p];
        
        // calculate constraint lambdas
        dm_physics_constraint_lambda(&point->normal,     manifold);
        dm_physics_constraint_lambda(&point->friction_a, manifold);
        dm_physics_constraint_lambda(&point->friction_b, manifold);
        
        // apply lambdas
        dm_physics_constraint_apply(&point->normal, manifold);
        
        const float mu = 0.5f;
        const double friction_lim = DM_MATH_SQRT2 * mu * point->normal.impulse_sum;
        
        point->friction_a.impulse_min = -friction_lim;
        point->friction_a.impulse_max =  friction_lim;
        point->friction_b.impulse_min = -friction_lim;
        point->friction_b.impulse_max =  friction_lim;
        
        dm_physics_constraint_apply(&point->friction_a, manifold);
        dm_physics_constraint_apply(&point->friction_b, manifold);
    }
}

void dm_physics_solve_constraints(dm_physics_manager* manager)
{
    for(uint32_t iter=0; iter<DM_PHYSICS_CONSTRAINT_ITER; iter++)
    {
        for(uint32_t m=0; m<manager->num_manifolds; m++)
        {
            dm_physics_apply_constraints(&manager->manifolds[m]);
        }
    }
}

/******
UPDATE
********/
// this is a straight run through of the entities
// this is done as explicitly as possible, with as few steps per line as possible
// so that one can more easily convert this to a SIMD version.
// thus we aren't using the built-in vector/matrix functions
// but hardcoding it in
void dm_physics_update_entities(dm_ecs_system_manager* system, dm_context* context)
{
    const uint32_t t_id = context->ecs_manager.default_components.transform;
    const uint32_t p_id = context->ecs_manager.default_components.physics;
    
    dm_component_transform_block* master_t_block = context->ecs_manager.components[t_id].data;
    dm_component_physics_block*   master_p_block = context->ecs_manager.components[p_id].data;
    
    uint32_t t_index, p_index;
    
    dm_component_transform_block* t_block;
    dm_component_physics_block*   p_block;
    
    float dt_mass;
    
    float w_x, w_y, w_z;
    float rot_i, rot_j, rot_k, rot_r;
    
    float new_rot_i, new_rot_j, new_rot_k, new_rot_r;
    float new_rot_mag;
    
    float orientation_00, orientation_01, orientation_02;
    float orientation_10, orientation_11, orientation_12;
    float orientation_20, orientation_21, orientation_22;
    
    float body_inv_00, body_inv_01, body_inv_02;
    float body_inv_10, body_inv_11, body_inv_12;
    float body_inv_20, body_inv_21, body_inv_22;
    
    float i_inv_00, i_inv_01, i_inv_02;
    float i_inv_10, i_inv_11, i_inv_12;
    float i_inv_20, i_inv_21, i_inv_22;
    
    const static float half_dt = 0.5f * DM_PHYSICS_FIXED_DT;
    
    dm_ecs_system_entity_container entity;
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        entity = system->entity_containers[i];
        
        t_block = master_t_block + entity.block_indices[t_id];
        p_block = master_p_block + entity.block_indices[p_id];
        
        t_index = entity.component_indices[t_id];
        p_index = entity.component_indices[p_id];
        
        // integrate position
        t_block->pos_x[t_index] += p_block->vel_x[p_index] * DM_PHYSICS_FIXED_DT;
        t_block->pos_y[t_index] += p_block->vel_y[p_index] * DM_PHYSICS_FIXED_DT;
        t_block->pos_z[t_index] += p_block->vel_z[p_index] * DM_PHYSICS_FIXED_DT;
        
        // integrate velocity
        dt_mass = p_block->mass[p_index] * DM_PHYSICS_FIXED_DT;
        
        p_block->vel_x[p_index] += p_block->force_x[p_index] * dt_mass;
        p_block->vel_y[p_index] += p_block->force_y[p_index] * dt_mass;
        p_block->vel_z[p_index] += p_block->force_z[p_index] * dt_mass;
        
        // integrate angular momentum
        p_block->l_x[p_index] += p_block->torque_x[p_index] * DM_PHYSICS_FIXED_DT;
        p_block->l_y[p_index] += p_block->torque_y[p_index] * DM_PHYSICS_FIXED_DT;
        p_block->l_z[p_index] += p_block->torque_z[p_index] * DM_PHYSICS_FIXED_DT;
        
        // integrate angular velocity
        p_block->w_x[p_index] += p_block->i_inv_00[p_index] * p_block->l_x[p_index];
        p_block->w_x[p_index] += p_block->i_inv_01[p_index] * p_block->l_y[p_index];
        p_block->w_x[p_index] += p_block->i_inv_02[p_index] * p_block->l_z[p_index];
        
        p_block->w_y[p_index] += p_block->i_inv_10[p_index] * p_block->l_x[p_index];
        p_block->w_y[p_index] += p_block->i_inv_11[p_index] * p_block->l_y[p_index];
        p_block->w_y[p_index] += p_block->i_inv_12[p_index] * p_block->l_z[p_index];
        
        p_block->w_z[p_index] += p_block->i_inv_20[p_index] * p_block->l_x[p_index];
        p_block->w_z[p_index] += p_block->i_inv_21[p_index] * p_block->l_y[p_index];
        p_block->w_z[p_index] += p_block->i_inv_22[p_index] * p_block->l_z[p_index];
        
        // integrate rotation
        rot_i = t_block->rot_i[t_index];
        rot_j = t_block->rot_j[t_index];
        rot_k = t_block->rot_k[t_index];
        rot_r = t_block->rot_r[t_index];
        
        w_x = p_block->w_x[p_index];
        w_y = p_block->w_y[p_index];
        w_z = p_block->w_z[p_index];
        
        new_rot_i  = w_x * rot_r;
        new_rot_i += w_y * rot_k;
        new_rot_i -= w_z * rot_j;
        new_rot_i *= half_dt;
        new_rot_i += rot_i;
        
        new_rot_j  = -w_x * rot_k;
        new_rot_j +=  w_y * rot_r;
        new_rot_j +=  w_z * rot_i;
        new_rot_j *= half_dt;
        new_rot_j += rot_j;
        
        new_rot_k  = w_x * rot_j;
        new_rot_k -= w_y * rot_i;
        new_rot_k += w_z * rot_r;
        new_rot_k *= half_dt;
        new_rot_k += rot_k;
        
        new_rot_r  = -w_x * rot_i;
        new_rot_r -=  w_y * rot_j;
        new_rot_r -=  w_z * rot_k;
        new_rot_r *= half_dt;
        new_rot_r += rot_r;
        
        new_rot_mag  = new_rot_i * new_rot_i;
        new_rot_mag += new_rot_j * new_rot_j;
        new_rot_mag += new_rot_k * new_rot_k;
        new_rot_mag += new_rot_r * new_rot_r;
        new_rot_mag  = dm_sqrtf(new_rot_mag);
        
        new_rot_i /= new_rot_mag;
        new_rot_j /= new_rot_mag;
        new_rot_k /= new_rot_mag;
        new_rot_r /= new_rot_mag;
        
        t_block->rot_i[t_index] = new_rot_i;
        t_block->rot_j[t_index] = new_rot_j;
        t_block->rot_k[t_index] = new_rot_k;
        t_block->rot_r[t_index] = new_rot_r;
        
        // update i_inv
        orientation_00  = new_rot_j * new_rot_j;
        orientation_00 += new_rot_k * new_rot_k;
        orientation_00 *= 2;
        orientation_00  = 1 - orientation_00;
        
        orientation_01  = new_rot_i * new_rot_j;
        orientation_01 += new_rot_k * new_rot_r;
        orientation_01 *= 2;
        
        orientation_02  = new_rot_i * new_rot_k;
        orientation_02 -= new_rot_j * new_rot_r;
        orientation_02 *= 2;
        
        orientation_10  = new_rot_i * new_rot_j;
        orientation_10 -= new_rot_k * new_rot_r;
        orientation_10 *= 2;
        
        orientation_11  = new_rot_i * new_rot_i;
        orientation_11 += new_rot_k * new_rot_k;
        orientation_11 *= 2;
        orientation_11  = 1 - orientation_11;
        
        orientation_12  = new_rot_j * new_rot_k;
        orientation_12 += new_rot_i * new_rot_r;
        orientation_12 *= 2;
        
        orientation_20  = new_rot_i * new_rot_k;
        orientation_20 += new_rot_j * new_rot_r;
        orientation_20 *= 2;
        
        orientation_21  = new_rot_j * new_rot_k;
        orientation_21 -= new_rot_i * new_rot_r;
        orientation_21 *= 2;
        
        orientation_22  = new_rot_i * new_rot_i;
        orientation_22 += new_rot_j * new_rot_j;
        orientation_22 *= 2;
        orientation_22  = 1 - orientation_22;
        
        // orientation is transposed here
        body_inv_00 = orientation_00 * p_block->i_inv_00[p_index];
        body_inv_01 = orientation_10 * p_block->i_inv_11[p_index];
        body_inv_02 = orientation_20 * p_block->i_inv_22[p_index];
        
        body_inv_10 = orientation_01 * p_block->i_inv_00[p_index];
        body_inv_11 = orientation_11 * p_block->i_inv_11[p_index];
        body_inv_12 = orientation_21 * p_block->i_inv_22[p_index];
        
        body_inv_20 = orientation_02 * p_block->i_inv_00[p_index];
        body_inv_21 = orientation_12 * p_block->i_inv_11[p_index];
        body_inv_22 = orientation_22 * p_block->i_inv_22[p_index];
        
        // final i_inv matrix
        i_inv_00  = orientation_00 * body_inv_00;
        i_inv_00 += orientation_01 * body_inv_10;
        i_inv_00 += orientation_02 * body_inv_20;
        
        i_inv_01  = orientation_00 * body_inv_01;
        i_inv_01 += orientation_01 * body_inv_11;
        i_inv_01 += orientation_02 * body_inv_21;
        
        i_inv_02  = orientation_00 * body_inv_02;
        i_inv_02 += orientation_01 * body_inv_12;
        i_inv_02 += orientation_02 * body_inv_22;
        
        i_inv_10  = orientation_10 * body_inv_00;
        i_inv_10 += orientation_11 * body_inv_10;
        i_inv_10 += orientation_12 * body_inv_20;
        
        i_inv_11  = orientation_10 * body_inv_01;
        i_inv_11 += orientation_11 * body_inv_11;
        i_inv_11 += orientation_12 * body_inv_21;
        
        i_inv_12  = orientation_10 * body_inv_02;
        i_inv_12 += orientation_11 * body_inv_12;
        i_inv_12 += orientation_12 * body_inv_22;
        
        i_inv_20  = orientation_20 * body_inv_00;
        i_inv_20 += orientation_21 * body_inv_10;
        i_inv_20 += orientation_22 * body_inv_20;
        
        i_inv_21  = orientation_20 * body_inv_01;
        i_inv_21 += orientation_21 * body_inv_11;
        i_inv_21 += orientation_22 * body_inv_21;
        
        i_inv_22  = orientation_20 * body_inv_02;
        i_inv_22 += orientation_21 * body_inv_12;
        i_inv_22 += orientation_22 * body_inv_22;
        
        p_block->i_inv_00[p_index] = i_inv_00;
        p_block->i_inv_01[p_index] = i_inv_01;
        p_block->i_inv_02[p_index] = i_inv_02;
        
        p_block->i_inv_10[p_index] = i_inv_10;
        p_block->i_inv_11[p_index] = i_inv_11;
        p_block->i_inv_12[p_index] = i_inv_12;
        
        p_block->i_inv_20[p_index] = i_inv_20;
        p_block->i_inv_21[p_index] = i_inv_21;
        p_block->i_inv_22[p_index] = i_inv_22;
    }
}

void dm_physics_reset_forces(dm_ecs_system_manager* system, dm_context* context)
{
    const uint32_t p_id = context->ecs_manager.default_components.physics;
    dm_component_physics_block* master_p_block = context->ecs_manager.components[p_id].data;
    
    uint32_t p_index;
    
    dm_component_physics_block* p_block;
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        dm_ecs_system_entity_container entity = system->entity_containers[i];
        
        p_block = master_p_block + entity.block_indices[p_id];
        p_index = entity.component_indices[p_id];
        
        p_block->force_x[p_index] = 0;
        p_block->force_y[p_index] = 0;
        p_block->force_z[p_index] = 0;
        
        p_block->torque_x[p_index] = 0;
        p_block->torque_y[p_index] = 0;
        p_block->torque_z[p_index] = 0;
    }
}

/************
SYSTEM FUNCS
**************/
bool dm_physics_system_run(dm_ecs_system_timing timing, dm_ecs_id system_id, void* context_v)
{
    dm_context* context           = context_v;
    dm_ecs_system_manager* system = &context->ecs_manager.systems[timing][system_id];
    dm_physics_manager* manager   = system->system_data;
    
    double total_time     = 0;
    double broad_time     = 0;
    double narrow_time    = 0;
    double collision_time = 0;
    double update_time    = 0;
    
    dm_timer t = { 0 };
    
    manager->accum_time += context->delta;
    
    uint32_t iters = 0;
    
    while(manager->accum_time >= DM_PHYSICS_FIXED_DT)
    {
        iters++;
        
        // broadphase
        dm_timer_start(&t, context);
        if(!dm_physics_broadphase(system, manager, context)) return false;
        broad_time += dm_timer_elapsed_ms(&t, context);
        total_time += broad_time;
        
        // narrowphase
        dm_timer_start(&t, context);
        if(!dm_physics_narrowphase(system, manager, context)) return false;
        narrow_time += dm_timer_elapsed_ms(&t, context);
        total_time += narrow_time;
        
        // collision resolution
        dm_timer_start(&t, context);
        dm_physics_solve_constraints(manager);
        collision_time += dm_timer_elapsed_ms(&t, context);
        total_time += collision_time;
        
        // update
        dm_timer_start(&t, context);
        dm_physics_update_entities(system, context);
        update_time += dm_timer_elapsed_ms(&t, context);
        total_time += update_time;
        
        // cleanup
        manager->num_manifolds = 0;
        
        manager->accum_time -= DM_PHYSICS_FIXED_DT;
    }
    dm_physics_reset_forces(system, context);
    
    DM_LOG_WARN("Physics broadphase average:           %lf ms", broad_time / (float)iters);
    DM_LOG_WARN("Physics narrowphase average:          %lf ms", narrow_time / (float)iters);
    DM_LOG_WARN("Physics collision resolution average: %lf ms", collision_time / (float)iters);
    DM_LOG_WARN("Updating entities average:            %lf ms", update_time / (float)iters);
    
    DM_LOG_WARN("Physics took:                         %lf ms, %u iterations", total_time, iters);
    
    return true;
}

void dm_physics_system_shutdown(dm_ecs_system_timing timing, dm_ecs_id system_id, void* c)
{
    dm_context* context = c;
    dm_ecs_system_manager* physics_system = &context->ecs_manager.systems[timing][system_id];
    dm_physics_manager* manager = physics_system->system_data;

    dm_free(manager->possible_collisions);
    dm_free(manager->manifolds);
    dm_free(physics_system->system_data);
}

bool dm_physics_system_init(dm_ecs_id* physics_id, dm_ecs_id* collision_id, dm_context* context)
{
    *physics_id = dm_ecs_register_component(sizeof(dm_component_physics_block), context);
    if(*physics_id==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register physics component"); return true; }
    
    *collision_id = dm_ecs_register_component(sizeof(dm_component_collision_block), context);
    if(*collision_id==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register collision component"); return true; }
    
    dm_ecs_id comps[] = {
        context->ecs_manager.default_components.transform,
        context->ecs_manager.default_components.physics,
        context->ecs_manager.default_components.collision
    };
    
    dm_ecs_system_timing timing = DM_ECS_SYSTEM_TIMING_UPDATE_BEGIN;
    context->ecs_manager.default_systems.physics = dm_ecs_register_system(comps, DM_ARRAY_LEN(comps), timing, dm_physics_system_run, dm_physics_system_shutdown, context);
    
    dm_ecs_system_manager* physics_system = &context->ecs_manager.systems[timing][context->ecs_manager.default_systems.physics];
    physics_system->system_data = dm_alloc(sizeof(dm_physics_manager));
    
    dm_physics_manager* manager = physics_system->system_data;
    
    manager->collision_capacity  = DM_PHYSICS_DEFAULT_COLLISION_CAPACITY;
    manager->manifold_capacity   = DM_PHYSICS_DEFAULT_MANIFOLD_CAPACITY;
    manager->possible_collisions = dm_alloc(sizeof(dm_collision_pair) * manager->collision_capacity);
    manager->manifolds           = dm_alloc(sizeof(dm_contact_manifold) * manager->manifold_capacity);
    
    return true;
}
