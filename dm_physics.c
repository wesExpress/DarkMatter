#include "dm.h"

#include <limits.h>
#include <float.h>

#ifdef DM_PHYSICS_SMALLER_DT
#define DM_PHYSICS_FIXED_DT          0.00416f // 1 / 240
#else
#define DM_PHYSICS_FIXED_DT          0.00833f // 1 / 120
#endif

#ifdef DM_DEBUG
#define DM_PHYSICS_MAX_GJK_ITER      64
#define DM_PHYSICS_EPA_MAX_FACES     64
#else
#define DM_PHYSICS_MAX_GJK_ITER      128
#define DM_PHYSICS_EPA_MAX_FACES     128
#endif

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

typedef struct dm_contact_manifold
{
    dm_entity entities[2];
    float     normal[3];
    float     tangent_a[3]; 
    float     tangent_b[3];
    float     orientation_a[4];
    float     orientation_b[4];
    
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
#ifdef DM_PLATFORM_WIN32
int dm_physics_broadphase_sort(void* c, const void* a, const void* b)
#else
int dm_physics_broadphase_sort(const void* a, const void* b, void* c)
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

int dm_physics_broadphase_get_variance_axis(dm_ecs_system_manager* system, dm_context* context)
{
    float center_sum[3] = { 0 };
    float center_sq_sum[3] = { 0 };
    int axis = 0;
    
    dm_component_collision_block* c_block;
    dm_component_transform_block* t_block;
    
    uint32_t  t_id = context->ecs_manager.default_components.transform;
    uint32_t  c_id = context->ecs_manager.default_components.collision;
    
    uint32_t  t_block_index, t_index;
    uint32_t  c_block_index, c_index;
    
    float a,b = 0.0f;
    float quat[N4];
    float rot[M3];
    float center[N3];
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        t_index       = system->entity_containers[i].component_indices[t_id];
        t_block_index = system->entity_containers[i].block_indices[t_id];
        
        c_index       = system->entity_containers[i].component_indices[c_id];
        c_block_index = system->entity_containers[i].block_indices[c_id];
        
        t_block = (dm_component_transform_block*)context->ecs_manager.components[t_id].data + t_block_index;
        c_block = (dm_component_collision_block*)context->ecs_manager.components[c_id].data + c_block_index;
        
        quat[0] = t_block->rot_i[t_index];
        quat[1] = t_block->rot_j[t_index];
        quat[2] = t_block->rot_k[t_index];
        quat[3] = t_block->rot_r[t_index];
        
        // reset collision flags
        c_block->flag[c_index] = DM_COLLISION_FLAG_NO;
        
        // update global aabb
        switch(c_block->shape[c_index])
        {
            case DM_COLLISION_SHAPE_SPHERE:
            {
                c_block->aabb_global_min_x[c_index] = c_block->aabb_local_min_x[c_index] + t_block->pos_x[t_index];
                c_block->aabb_global_min_y[c_index] = c_block->aabb_local_min_y[c_index] + t_block->pos_y[t_index];
                c_block->aabb_global_min_z[c_index] = c_block->aabb_local_min_z[c_index] + t_block->pos_z[t_index];
                
                c_block->aabb_global_max_x[c_index] = c_block->aabb_local_max_x[c_index] + t_block->pos_x[t_index];
                c_block->aabb_global_max_y[c_index] = c_block->aabb_local_max_y[c_index] + t_block->pos_y[t_index];
                c_block->aabb_global_max_z[c_index] = c_block->aabb_local_max_z[c_index] + t_block->pos_z[t_index];
            } break;
            
            case DM_COLLISION_SHAPE_BOX:
            {
                c_block->aabb_global_min_x[c_index] = c_block->aabb_global_max_x[c_index] = t_block->pos_x[t_index];
                c_block->aabb_global_min_y[c_index] = c_block->aabb_global_max_y[c_index] = t_block->pos_y[t_index];
                c_block->aabb_global_min_z[c_index] = c_block->aabb_global_max_z[c_index] = t_block->pos_z[t_index];
                
                dm_mat3_rotate_from_quat(quat, rot);
                
                // updating x
                a = rot[0] * c_block->aabb_local_min_x[c_index];
                b = rot[0] * c_block->aabb_local_max_x[c_index];
                c_block->aabb_global_min_x[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_x[c_index] += DM_MAX(a,b);
                
                a = rot[1] * c_block->aabb_local_min_x[c_index];
                b = rot[1] * c_block->aabb_local_max_x[c_index];
                c_block->aabb_global_min_x[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_x[c_index] += DM_MAX(a,b);
                
                a = rot[2] * c_block->aabb_local_min_x[c_index];
                b = rot[2] * c_block->aabb_local_max_x[c_index];
                c_block->aabb_global_min_x[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_x[c_index] += DM_MAX(a,b);
                
                // updating y
                a = rot[3] * c_block->aabb_local_min_y[c_index];
                b = rot[3] * c_block->aabb_local_max_y[c_index];
                c_block->aabb_global_min_y[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_y[c_index] += DM_MAX(a,b);
                
                a = rot[4] * c_block->aabb_local_min_y[c_index];
                b = rot[4] * c_block->aabb_local_max_y[c_index];
                c_block->aabb_global_min_y[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_y[c_index] += DM_MAX(a,b);
                
                a = rot[5] * c_block->aabb_local_min_y[c_index];
                b = rot[5] * c_block->aabb_local_max_y[c_index];
                c_block->aabb_global_min_y[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_y[c_index] += DM_MAX(a,b);
                
                // updating z
                a = rot[6] * c_block->aabb_local_min_z[c_index];
                b = rot[6] * c_block->aabb_local_max_z[c_index];
                c_block->aabb_global_min_z[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_z[c_index] += DM_MAX(a,b);
                
                a = rot[7] * c_block->aabb_local_min_z[c_index];
                b = rot[7] * c_block->aabb_local_max_z[c_index];
                c_block->aabb_global_min_z[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_z[c_index] += DM_MAX(a,b);
                
                a = rot[8] * c_block->aabb_local_min_z[c_index];
                b = rot[8] * c_block->aabb_local_max_z[c_index];
                c_block->aabb_global_min_z[c_index] += DM_MIN(a,b);
                c_block->aabb_global_max_z[c_index] += DM_MAX(a,b);
            } break;
            
            default:
            DM_LOG_ERROR("Unknown collision shape");
            return axis;
        }
        
        // add to center and center sq vectors
        center[0] = c_block->aabb_global_max_x[c_index] - c_block->aabb_global_min_x[c_index];
        center[1] = c_block->aabb_global_max_y[c_index] - c_block->aabb_global_min_y[c_index];
        center[2] = c_block->aabb_global_max_z[c_index] - c_block->aabb_global_min_z[c_index];
        
        dm_vec3_add_vec3(center_sum, center, center_sum);
        dm_vec3_mul_vec3(center, center, center);
        dm_vec3_add_vec3(center_sq_sum, center, center_sq_sum);
    }
    
    float scale = 1.0f / system->entity_count;
    dm_vec3_scale(center_sum, scale, center_sum);
    dm_vec3_mul_vec3(center_sum, center_sum, center_sum);
    dm_vec3_scale(center_sq_sum, scale, center_sq_sum);
    
    float var[3];
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
    
    dm_component_collision_block* block = context->ecs_manager.components[data.index].data;
    
    // sort
    switch(axis)
    {
        case 0:
        data.min = block->aabb_global_min_x;
        break;
        
        case 1:
        data.min = block->aabb_global_min_y;
         break;
        
        case 2:
        data.min = block->aabb_global_min_z;
        break;
    }
    
#ifdef DM_PLATFORM_WIN32
    qsort_s(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
#else
    qsort_r(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
#endif

    // sweep
    float max_i, min_j;
    dm_ecs_system_entity_container entity_a, entity_b;
    uint32_t a_c_index, a_b_index, b_c_index, b_b_index;
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        entity_a = system->entity_containers[i];
        
        a_c_index = entity_a.component_indices[data.index];
        a_b_index = entity_a.block_indices[data.index];
        
        switch(axis)
        {
            case 0: max_i = (block + a_b_index)->aabb_global_max_x[a_c_index];
            break;
            
            case 1: max_i = (block + a_b_index)->aabb_global_max_y[a_c_index];
            break;
            
            case 2: max_i = (block + a_b_index)->aabb_global_max_z[a_c_index];
            break;

            default:
            DM_LOG_FATAL("Shouldn't be here in broadphase");
            return false;
        }
        
        for(uint32_t j=i+1; j<system->entity_count; j++)
        {
            entity_b = system->entity_containers[j];
            
            b_c_index = entity_b.component_indices[data.index];
            b_b_index = entity_b.block_indices[data.index];
            
            switch(axis)
            {
                case 0: min_j = (block + b_b_index)->aabb_global_min_x[b_c_index];
                break;
                
                case 1: min_j = (block + b_b_index)->aabb_global_min_y[b_c_index];
                break;
                
                case 2: min_j = (block + b_b_index)->aabb_global_min_z[b_c_index];
                break;
                
                default:
                DM_LOG_FATAL("Shouldn't be here in broadphase");
                return false;
            }
            
            if(min_j > max_i) break;
            
            manager->possible_collisions[manager->num_possible_collisions].entity_a = entity_a;
            manager->possible_collisions[manager->num_possible_collisions].entity_b = entity_b;
            manager->num_possible_collisions++;
            
            (block + a_b_index)->flag[a_c_index] = DM_COLLISION_FLAG_POSSIBLE;
            (block + b_b_index)->flag[b_c_index] = DM_COLLISION_FLAG_POSSIBLE;
            
            float load = (float)manager->num_possible_collisions / (float)manager->collision_capacity;
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
void dm_support_func_sphere(float pos[3], float cen[3], float internals[6], float d[3], float support[3])
{
    float radius = internals[0];
    
    support[0] = (d[0] * radius) + (pos[0] + cen[0]); 
    support[1] = (d[1] * radius) + (pos[1] + cen[1]); 
    support[2] = (d[2] * radius) + (pos[2] + cen[2]);
}

void dm_support_func_box(float pos[3], float rot[4], float cen[3], float internals[6], float d[3], float support[3])
{
    float inv_rot[4];
    float p[3];
    
    dm_quat_inverse(rot, inv_rot);
    dm_vec3_rotate(d, inv_rot, d);
    
    support[0] = (d[0] > 0) ? internals[3] : internals[0];
    support[1] = (d[1] > 0) ? internals[4] : internals[1];
    support[2] = (d[2] > 0) ? internals[5] : internals[2];
    
    dm_vec3_rotate(support, rot, support);
    dm_vec3_add_vec3(pos, cen, p);
    dm_vec3_add_vec3(support, p, support);
    dm_vec3_rotate(d, rot, d);
}

// 2d support funcs

// gjk
void dm_physics_gjk_support(dm_ecs_system_entity_container entity, float direction[3], float support[3], dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block)
{
    dm_component_transform_block* e_t_block = t_block + entity.block_indices[t_id];
    dm_component_collision_block* e_c_block = c_block + entity.block_indices[c_id];
    
    float pos[] = {
        e_t_block->pos_x[entity.component_indices[t_id]],
        e_t_block->pos_y[entity.component_indices[t_id]],
        e_t_block->pos_z[entity.component_indices[t_id]],
    };
    
    float rot[] = {
        e_t_block->rot_i[entity.component_indices[t_id]],
        e_t_block->rot_j[entity.component_indices[t_id]],
        e_t_block->rot_k[entity.component_indices[t_id]],
        e_t_block->rot_r[entity.component_indices[t_id]],
    };
    
    float cen[] = {
        e_c_block->center_x[entity.component_indices[c_id]],
        e_c_block->center_y[entity.component_indices[c_id]],
        e_c_block->center_z[entity.component_indices[c_id]],
    };
    
    float internals[] = {
        e_c_block->internal_0[entity.component_indices[c_id]],
        e_c_block->internal_1[entity.component_indices[c_id]],
        e_c_block->internal_2[entity.component_indices[c_id]],
        e_c_block->internal_3[entity.component_indices[c_id]],
        e_c_block->internal_4[entity.component_indices[c_id]],
        e_c_block->internal_5[entity.component_indices[c_id]],
    };
    
    dm_collision_shape shape = e_c_block->shape[entity.component_indices[c_id]];
    
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE: 
        dm_support_func_sphere(pos, cen, internals, direction, support);
        break;
        
        case DM_COLLISION_SHAPE_BOX: 
        dm_support_func_box(pos, rot, cen, internals, direction, support);
        break;
        
        default:
        DM_LOG_ERROR("Collision shape not supported, or unknown shape! Probably shouldn't be here...");
        break;
    }
}

void dm_support(dm_ecs_system_entity_container entity_a, dm_ecs_system_entity_container entity_b, float direction[3], float out[3], dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block)
{
    float dir_neg[3];
    float support_a[3];
    float support_b[3];
    
    dm_vec3_negate(direction, dir_neg);
    
    dm_physics_gjk_support(entity_a, direction, support_a, t_id, c_id, t_block, c_block);
    dm_physics_gjk_support(entity_b, dir_neg, support_b, t_id, c_id, t_block, c_block);
    
    dm_vec3_sub_vec3(support_a, support_b, out);
}

void dm_simplex_push_front(float point[3], dm_simplex* simplex)
{
    dm_memcpy(simplex->points[3], simplex->points[2], sizeof(float) * 3);
    dm_memcpy(simplex->points[2], simplex->points[1], sizeof(float) * 3);
    dm_memcpy(simplex->points[1], simplex->points[0], sizeof(float) * 3);
    dm_memcpy(simplex->points[0], point, sizeof(float) * 3);
    
    simplex->size = DM_MIN(simplex->size+1, 4);
}

/*
find vector ab. find vector a to origin. 
if ao is not in the same direction as ab, we need to restart
*/
bool dm_simplex_line(float direction[3], dm_simplex* simplex)
{
    float ab[3];
    float ao[3];
    
    dm_vec3_sub_vec3(simplex->points[1], simplex->points[0], ab);
    dm_vec3_scale(simplex->points[0], -1, ao);
    
    if(dm_vec3_same_direction(ab, ao)) 
    {
        float abo[3];
        dm_vec3_cross(ab, ao, abo);
        dm_vec3_cross(abo, ab, direction);
    }
    else 
    {
        simplex->size = 1;
        direction[0] = ao[0]; direction[1] = ao[1]; direction[2] = ao[2];
    }
    
    return false;
}

/*
C         .
|\   1   .
| \     .
|  \   .
| 2 \ .  
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
    float ab[3]; float ac[3]; float ao[3]; float abc[3]; float d[3];
    
    dm_vec3_sub_vec3(simplex->points[1], simplex->points[0], ab);
    dm_vec3_sub_vec3(simplex->points[2], simplex->points[0], ac);
    dm_vec3_negate(simplex->points[0], ao);
    dm_vec3_cross(ab, ac, abc);
    
    // check beyond AC plane (region 1 and 5)
    dm_vec3_cross(abc, ac, d);
    if(dm_vec3_same_direction(d, ao))
    {
        // are we actually in region 1?
        if(dm_vec3_same_direction(ac, ao))
        {
            simplex->size = 2;
            simplex->points[1][0] = simplex->points[2][0];
            simplex->points[1][1] = simplex->points[2][1];
            simplex->points[1][2] = simplex->points[2][2];
            
            dm_vec3_cross(ac, ao, d);
            dm_vec3_cross(d, ac, direction);
        }
        else 
        {
            // are we in region 4?
            if(dm_vec3_same_direction(ab, ao))
            {
                simplex->size = 2;
                
                dm_vec3_cross(ab, ao, d);
                dm_vec3_cross(d, ab, direction);
                
                return dm_simplex_line(direction, simplex); 
            }
            
            // must be in region 5
            simplex->size = 1;
            direction[0] = ao[0]; direction[1] = ao[1]; direction[2] = ao[2];
        }
    }
    else
    {
        // check beyond AB plane (region 4 and 5)
        dm_vec3_cross(ab, abc, d);
        if(dm_vec3_same_direction(d, ao))
        {
            // are we in region 4?
            if(dm_vec3_same_direction(ab, ao))
            {
                simplex->size = 2;
                
                dm_vec3_cross(ab, ao, d);
                dm_vec3_cross(d, ab, direction);
                
                return dm_simplex_line(direction, simplex); 
            }
            
            // must be in region 5
            simplex->size = 1;
            direction[0] = ao[0]; direction[1] = ao[1]; direction[2] = ao[2];
        }
        // origin must be in triangle
        else
        {
            // are we above plane? (region 2)
            if(dm_vec3_same_direction(abc, ao)) 
            {
                direction[0] = abc[0]; direction[1] = abc[1]; direction[2] = abc[2];
            }
            else
            {
                // below plane (region 3)
                simplex->size = 3;
                d[0] = simplex->points[1][0];
                d[1] = simplex->points[1][1];
                d[2] = simplex->points[1][2];
                
                simplex->points[1][0] = simplex->points[2][1]; simplex->points[1][1] = simplex->points[2][1]; simplex->points[1][2] = simplex->points[2][1];
                simplex->points[2][0] = d[0]; simplex->points[2][1] = d[1]; simplex->points[2][2] = d[2];
                
                dm_vec3_negate(abc, direction);
            }
        }
    }
    return false;
}

// series of triangle checks
bool dm_simplex_tetrahedron(float direction[3], dm_simplex* simplex)
{
    float ab[3]; float ac[3]; float ad[3]; float ao[3];
    float abc[3]; float acd[3]; float adb[3];
    
    dm_vec3_sub_vec3(simplex->points[1], simplex->points[0], ab);
    dm_vec3_sub_vec3(simplex->points[2], simplex->points[0], ac);
    dm_vec3_sub_vec3(simplex->points[3], simplex->points[0], ad);
    dm_vec3_negate(simplex->points[0], ao);
    
    dm_vec3_cross(ab, ac, abc);
    dm_vec3_cross(ac, ad, acd);
    dm_vec3_cross(ad, ab, adb);
    
    if(dm_vec3_same_direction(abc, ao))
    {
        simplex->size = 3;
        direction[0] = abc[0]; direction[1] = abc[1]; direction[2] = abc[2];
        
        return dm_simplex_triangle(direction, simplex);
    }
    if (dm_vec3_same_direction(acd, ao))
    {
        simplex->size = 3;
        
        simplex->points[1][0] = simplex->points[2][1]; 
        simplex->points[1][1] = simplex->points[2][1]; 
        simplex->points[1][2] = simplex->points[2][1];
        
        simplex->points[2][0] = simplex->points[3][1]; 
        simplex->points[2][1] = simplex->points[3][1]; 
        simplex->points[2][2] = simplex->points[3][1];
        
        direction[0] = acd[0]; direction[1] = acd[1]; direction[2] = acd[2];
        
        return dm_simplex_triangle(direction, simplex);
    }
    if (dm_vec3_same_direction(adb, ao))
    {
        simplex->size = 3;
        
        simplex->points[1][0] = simplex->points[3][1]; 
        simplex->points[1][1] = simplex->points[3][1]; 
        simplex->points[1][2] = simplex->points[3][1];
        
        simplex->points[2][0] = simplex->points[2][1]; 
        simplex->points[2][1] = simplex->points[2][1]; 
        simplex->points[2][2] = simplex->points[2][1];
        
        direction[0] = adb[0]; direction[1] = adb[1]; direction[2] = adb[2];
        
        return dm_simplex_triangle(direction, simplex);
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

bool dm_physics_gjk(dm_ecs_system_entity_container entity_a, dm_ecs_system_entity_container entity_b, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, dm_simplex* simplex)
{
    // initial guess should not matter, but algorithm WILL FAIL if initial guess is
    // perfectly aligned with global unit axes
    
    float direction[3] = { 0,1,0 };
    
    // start simplex
    float support[3];
    
    dm_support(entity_a, entity_b, direction, support, t_id, c_id, t_block, c_block);
    dm_vec3_negate(support, direction);
    dm_simplex_push_front(support, simplex);
    
    for(uint32_t iter=0; iter<DM_PHYSICS_MAX_GJK_ITER; iter++)
    {
        if(simplex->size > 4) DM_LOG_ERROR("GJK simplex size greater than 4...?");
        
        dm_support(entity_a, entity_b, direction, support, t_id, c_id, t_block, c_block);
        if(dm_vec3_dot(support, direction) < 0 ) return false;
        
        dm_simplex_push_front(support, simplex);
        
        // if we have a collision, find penetration depth
        if(dm_next_simplex(direction, simplex)) return true;
    }
    
    DM_LOG_ERROR("GJK failed to converge after %u iterations", DM_PHYSICS_MAX_GJK_ITER);
    return false;
}

// EPA
//https://github.com/kevinmoran/GJK/blob/master/GJK.h
//https://www.youtube.com/watch?v=0XQ2FSz3EK8&ab_channel=Winterdev
bool dm_physics_epa(dm_ecs_system_entity_container entity_a, dm_ecs_system_entity_container entity_b, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, float penetration[3], dm_simplex* simplex)
{
    float faces[DM_PHYSICS_EPA_MAX_FACES][3][3];
    float normals[DM_PHYSICS_EPA_MAX_FACES][3];
    
    float a[3], b[3], c[3], d[3];
    float dum1[3], dum2[3];
    
    dm_memcpy(a, simplex->points[0], sizeof(a));
    dm_memcpy(b, simplex->points[1], sizeof(a));
    dm_memcpy(c, simplex->points[2], sizeof(a));
    dm_memcpy(d, simplex->points[3], sizeof(a));
    
    // face a,b,c
    dm_memcpy(faces[0][0], a, sizeof(a));
    dm_memcpy(faces[0][1], b, sizeof(b));
    dm_memcpy(faces[0][2], c, sizeof(c));
    dm_vec3_sub_vec3(b, a, dum1);
    dm_vec3_sub_vec3(c, a, dum2);
    dm_vec3_cross(dum1, dum2, normals[0]);
    dm_vec3_norm(normals[0], normals[0]);
    
    // face a,c,d
    dm_memcpy(faces[1][0], a, sizeof(a));
    dm_memcpy(faces[1][1], c, sizeof(c));
    dm_memcpy(faces[1][2], d, sizeof(d));
    dm_vec3_sub_vec3(c, a, dum1);
    dm_vec3_sub_vec3(d, a, dum2);
    dm_vec3_cross(dum1, dum2, normals[1]);
    dm_vec3_norm(normals[1], normals[1]);
    
    // face a,d,b
    dm_memcpy(faces[2][0], a, sizeof(a));
    dm_memcpy(faces[2][1], d, sizeof(d));
    dm_memcpy(faces[2][2], b, sizeof(b));
    dm_vec3_sub_vec3(d, a, dum1);
    dm_vec3_sub_vec3(b, a, dum2);
    dm_vec3_cross(dum1, dum2, normals[2]);
    dm_vec3_norm(normals[2], normals[2]);
    
    // face b,d,c
    dm_memcpy(faces[3][0], b, sizeof(b));
    dm_memcpy(faces[3][1], d, sizeof(b));
    dm_memcpy(faces[3][2], c, sizeof(c));
    dm_vec3_sub_vec3(d, b, dum1);
    dm_vec3_sub_vec3(c, b, dum2);
    dm_vec3_cross(dum1, dum2, normals[3]);
    dm_vec3_norm(normals[3], normals[3]);
    
    uint32_t num_faces = 4;
    uint32_t closest_face;
    float    min_distance;
    float    support[3];
    float    direction[3];
    float    loose_edges[DM_PHYSICS_EPA_MAX_FACES][2][3];
    uint32_t num_loose = 0;
    float    current_edge[2][3];
    bool     found;
    
    for(uint32_t iter=0; iter<DM_PHYSICS_EPA_MAX_FACES; iter++)
    {
        min_distance = dm_vec3_dot(faces[0][0], normals[0]);
        closest_face = 0;
        for(uint32_t i=1; i<num_faces; i++)
        {
            float distance = dm_vec3_dot(faces[i][0], normals[i]);
            if(distance >= min_distance) continue;
            
            min_distance = distance;
            closest_face = i;
        }
        
        dm_memcpy(direction, normals[closest_face], sizeof(direction));
        
        dm_support(entity_a, entity_b, direction, support, t_id, c_id, t_block, c_block);
        
        // have we converged
        if(dm_vec3_dot(support, direction) < DM_PHYSICS_EPA_TOLERANCE)
        {
            float depth = dm_vec3_dot(support, direction);
            if(!depth) return false;
            
            dm_vec3_scale(direction, depth, penetration);
            return true;
        }
        
        // revamp polytope
        for(uint32_t i=0; i<num_faces; i++)
        {
            dm_vec3_sub_vec3(support, faces[i][0], dum1);
            if(dm_vec3_dot(normals[i], dum1) <= 0) continue;
            
            for(uint32_t j=0; j<3; j++)
            {
                dm_memcpy(current_edge[0], faces[i][j],       sizeof(current_edge[0]));
                dm_memcpy(current_edge[1], faces[i][(j+1)%3], sizeof(current_edge[1]));
                found = false;
                
                for(uint32_t k=0; k<num_loose; k++)
                {
                    bool cond1 = dm_vec3_equals_vec3(loose_edges[k][1], current_edge[0]);
                    bool cond2 = dm_vec3_equals_vec3(loose_edges[k][0], current_edge[1]);
                    
                    if(!cond1 || !cond2) continue;
                    
                    dm_memcpy(loose_edges[k][0], loose_edges[num_loose-1][0], sizeof(loose_edges[k][0]));
                    dm_memcpy(loose_edges[k][1], loose_edges[num_loose-1][1], sizeof(loose_edges[k][0]));;
                    
                    num_loose--;
                    found = true;
                    k = num_loose;
                }
                
                if(found) continue;
                if(num_loose>=DM_PHYSICS_EPA_MAX_FACES) break;
                
                dm_memcpy(loose_edges[num_loose][0], current_edge[0], sizeof(loose_edges[num_loose][0]));
                dm_memcpy(loose_edges[num_loose][1], current_edge[1], sizeof(loose_edges[num_loose][1]));
                num_loose++;
            }
            
            dm_memcpy(faces[i][0], faces[num_faces-1][0], sizeof(faces[i][0]));
            dm_memcpy(faces[i][1], faces[num_faces-1][1], sizeof(faces[i][1]));
            dm_memcpy(faces[i][2], faces[num_faces-1][2], sizeof(faces[i][2]));
            dm_memcpy(normals[i], normals[num_faces-1], sizeof(normals[i]));
            num_faces--;
            i--;
        }
        
        // reconstruct polytope
        for(uint32_t i=0; i<num_loose; i++)
        {
            if(num_faces >= DM_PHYSICS_EPA_MAX_FACES) break;
            
            dm_memcpy(faces[num_faces][0], loose_edges[i][0], sizeof(faces[num_faces][0]));
            dm_memcpy(faces[num_faces][1], loose_edges[i][1], sizeof(faces[num_faces][1]));
            dm_memcpy(faces[num_faces][2], support, sizeof(faces[num_faces][2]));
            dm_vec3_sub_vec3(loose_edges[i][0], loose_edges[i][1], dum1);
            dm_vec3_sub_vec3(loose_edges[i][0], support, dum2);
            dm_vec3_cross(dum1, dum2, normals[num_faces]);
            dm_vec3_norm(normals[num_faces], normals[num_faces]);
            
            static const float bias = 0.000001f;
            if(dm_vec3_dot(faces[num_faces][0], normals[num_faces]) + bias < 0)
            {
                float temp[3];
                dm_memcpy(temp, faces[num_faces][0], sizeof(temp));
                dm_memcpy(faces[num_faces][0], faces[num_faces][1], sizeof(temp));
                dm_memcpy(faces[num_faces][1], temp, sizeof(temp));
                dm_vec3_scale(normals[num_faces], -1, normals[num_faces]);
            }
            
            num_faces++;
        }
    }
    
    DM_LOG_ERROR("EPA failed to converge after %u iterations", DM_PHYSICS_EPA_MAX_FACES);
    return false;
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
        dm_memcpy(ref_pt, points[i], sizeof(ref_pt));
        
        dm_vec3_sub_vec3(points[ids[i]], ref_pt, dum1);
        dm_vec3_cross(dum1, neg_normal, dum2);
        dm_vec3_norm(dum2, normal);
        distance = -dm_vec3_dot(normal, ref_pt);
        
        dm_memcpy(planes[i+1].normal, normal, sizeof(normal));
        planes[i+1].distance=distance;
    }
}

void dm_support_face_box(dm_ecs_system_entity_container entity, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, float direction[3], float points[10][3], uint32_t* num_pts, dm_plane planes[5], uint32_t* num_planes, float normal[3])
{
    float pos[] = {
        (t_block + entity.block_indices[t_id])->pos_x[entity.component_indices[c_id]],
        (t_block + entity.block_indices[t_id])->pos_y[entity.component_indices[c_id]],
        (t_block + entity.block_indices[t_id])->pos_z[entity.component_indices[c_id]],
    };
    
    float rot[] = { 
        (t_block + entity.block_indices[t_id])->rot_i[entity.component_indices[c_id]],
        (t_block + entity.block_indices[t_id])->rot_j[entity.component_indices[c_id]],
        (t_block + entity.block_indices[t_id])->rot_k[entity.component_indices[c_id]],
        (t_block + entity.block_indices[t_id])->rot_r[entity.component_indices[c_id]],
    };
    
    float box_min[] = {
        (c_block + entity.block_indices[t_id])->internal_0[entity.component_indices[c_id]],
        (c_block + entity.block_indices[t_id])->internal_1[entity.component_indices[c_id]],
        (c_block + entity.block_indices[t_id])->internal_2[entity.component_indices[c_id]],
    };
    
    float box_max[] = {
        (c_block + entity.block_indices[t_id])->internal_3[entity.component_indices[c_id]],
        (c_block + entity.block_indices[t_id])->internal_4[entity.component_indices[c_id]],
        (c_block + entity.block_indices[t_id])->internal_5[entity.component_indices[c_id]],
    };
    
    float inv_rot[3];
    dm_quat_inverse(rot, inv_rot);
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
        float s = DM_SIGN(proximity);
        proximity *= s;
        if(proximity <= best_proximity) continue;
        
        best_proximity = proximity;
        best_sgn = s;
        best_axis = i;
    }
    
    float max_t[3];
    float min_t[3];
    
    dm_memcpy(max_t, box_max, sizeof(box_max));
    dm_memcpy(min_t, box_min, sizeof(box_min));
    
    switch(best_axis)
    {
        case 0:
        {
            if(best_sgn > 0)
            {
                points[0][0] = max_t[0]; points[0][1] = min_t[1]; points[0][2] = min_t[2];
                points[1][0] = max_t[0]; points[1][1] = min_t[1]; points[1][2] = max_t[2];
                points[2][0] = max_t[0]; points[2][1] = max_t[1]; points[2][2] = max_t[2];
                points[3][0] = max_t[0]; points[3][1] = max_t[1]; points[3][2] = min_t[2];
            }
            else
            {
                points[0][0] = min_t[0]; points[0][1] = min_t[1]; points[0][2] = max_t[2];
                points[1][0] = min_t[0]; points[1][1] = min_t[1]; points[1][2] = min_t[2];
                points[2][0] = min_t[0]; points[2][1] = max_t[1]; points[2][2] = min_t[2];
                points[3][0] = min_t[0]; points[3][1] = max_t[1]; points[3][2] = max_t[2];
            }
        } break;
        
        case 1:
        {
            if(best_sgn > 0)
            {
                points[0][0] = min_t[0]; points[0][1] = max_t[1]; points[0][2] = min_t[2];
                points[1][0] = max_t[0]; points[1][1] = max_t[1]; points[1][2] = min_t[2];
                points[2][0] = max_t[0]; points[2][1] = max_t[1]; points[2][2] = max_t[2];
                points[3][0] = min_t[0]; points[3][1] = max_t[1]; points[3][2] = max_t[2];
            }
            else
            {
                points[0][0] = max_t[0]; points[0][1] = min_t[1]; points[0][2] = min_t[2];
                points[1][0] = min_t[0]; points[1][1] = min_t[1]; points[1][2] = min_t[2];
                points[2][0] = min_t[0]; points[2][1] = min_t[1]; points[2][2] = max_t[2];
                points[3][0] = max_t[0]; points[3][1] = min_t[1]; points[3][2] = max_t[2];
            }
        } break;
        
        case 2:
        {
            if(best_sgn > 0)
            {
                points[0][0] = max_t[0]; points[0][1] = min_t[1]; points[0][2] = max_t[2];
                points[1][0] = min_t[0]; points[1][1] = min_t[1]; points[1][2] = max_t[2];
                points[2][0] = min_t[0]; points[2][1] = max_t[1]; points[2][2] = max_t[2];
                points[3][0] = max_t[0]; points[3][1] = max_t[1]; points[3][2] = max_t[2];
            }
            else
            {
                points[0][0] = min_t[0]; points[0][1] = min_t[1]; points[0][2] = min_t[2];
                points[1][0] = max_t[0]; points[1][1] = min_t[1]; points[1][2] = min_t[2];
                points[2][0] = max_t[0]; points[2][1] = max_t[1]; points[2][2] = min_t[2];
                points[3][0] = min_t[0]; points[3][1] = max_t[1]; points[3][2] = min_t[2];
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
    
    dm_memcpy(planes[0].normal, normal, sizeof(planes[0].normal));
    planes[0].distance = -dm_vec3_dot(planes[0].normal, points[0]);
    
    dm_support_face_box_planes(planes, points, normal);
    
    *num_pts    = 4;
    *num_planes = 5;
}

void dm_support_face_entity(dm_ecs_system_entity_container entity, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, float direction[3], float points[10][3], uint32_t* num_pts, dm_plane planes[5], uint32_t* num_planes, float normal[3])
{
    dm_collision_shape shape = (c_block + entity.block_indices[c_id])->shape[entity.component_indices[c_id]];
    
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE:
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        dm_support_face_box(entity, t_id, c_id, t_block, c_block, direction, points, num_pts, planes, num_planes, normal);
        break;
        
        default:
        DM_LOG_ERROR("Unknown or unsupported collider type");
        return;
    }
}

bool dm_physics_collide_sphere_other(dm_ecs_system_entity_container entity_a, dm_ecs_system_entity_container entity_b, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, dm_simplex* simplex)
{
    dm_collision_shape shape = (c_block + entity_b.block_indices[c_id])->shape[entity_b.component_indices[c_id]];
    
    switch(shape)
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

void dm_physics_collide_poly_sphere(dm_ecs_system_entity_container box, dm_ecs_system_entity_container sphere, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, dm_simplex* simplex)
{
    float penetration[3];
    if(!dm_physics_epa(box, sphere, t_id, c_id, t_block, c_block, penetration, simplex)) return;
    
    float    points_box[10][3];
    uint32_t num_pts;
    float    normal_box[3];
    dm_plane planes[5];
    uint32_t num_planes;
}

void dm_physics_collide_poly_poly(dm_ecs_system_entity_container poly_a, dm_ecs_system_entity_container poly_b, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, dm_simplex* simplex)
{
    float penetration[3];
    if(!dm_physics_epa(poly_a, poly_b, t_id, c_id, t_block, c_block, penetration, simplex)) return;
    
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
    
    dm_support_face_entity(poly_a, t_id, c_id, t_block, c_block, norm_pen, points_a, &num_pts_a, planes_a, &num_planes_a, normal_a);
    dm_support_face_entity(poly_b, t_id, c_id, t_block, c_block, neg_pen, points_b, &num_pts_b, planes_b, &num_planes_b, normal_b);
}

bool dm_physics_collide_poly_other(dm_ecs_system_entity_container entity_a, dm_ecs_system_entity_container entity_b, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, dm_simplex* simplex)
{
    dm_collision_shape shape = (c_block + entity_b.block_indices[c_id])->shape[entity_b.component_indices[c_id]];
    
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE:
        dm_physics_collide_poly_sphere(entity_a, entity_b, t_id, c_id, t_block, c_block, simplex);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        dm_physics_collide_poly_poly(entity_a, entity_b, t_id, c_id, t_block, c_block, simplex);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
    
    return true;
}

bool dm_physics_collide_entities(dm_ecs_system_entity_container entity_a, dm_ecs_system_entity_container entity_b, dm_ecs_id t_id, dm_ecs_id c_id, dm_component_transform_block* t_block, dm_component_collision_block* c_block, dm_simplex* simplex, dm_physics_manager* manager)
{
    dm_contact_manifold* manifold = &manager->manifolds[manager->num_manifolds++];
    *manifold = (dm_contact_manifold){ 0 };
    
    dm_collision_shape shape = (c_block + entity_a.block_indices[c_id])->shape[entity_a.component_indices[c_id]];
    
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE:
        return dm_physics_collide_sphere_other(entity_a, entity_b, t_id, c_id, t_block, c_block, simplex);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        return dm_physics_collide_poly_other(entity_a, entity_b, t_id, c_id, t_block, c_block, simplex);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
}

// narrowphase
bool dm_physics_narrowphase(dm_ecs_system_manager* system, dm_physics_manager* manager, dm_context* context)
{
    dm_ecs_id t_id = context->ecs_manager.default_components.transform;
    dm_ecs_id c_id = context->ecs_manager.default_components.collision;
    
    dm_component_transform_block* t_block = context->ecs_manager.components[t_id].data;
    dm_component_collision_block* c_block = context->ecs_manager.components[c_id].data;
    
    for(uint32_t i=0; i<manager->num_possible_collisions; i++)
    {
        dm_collision_pair collision_pair = manager->possible_collisions[i];
        
        dm_ecs_system_entity_container entity_a = collision_pair.entity_a;
        dm_ecs_system_entity_container entity_b = collision_pair.entity_b;
        
        dm_simplex simplex = { 0 };
        
        if(!dm_physics_gjk(entity_a, entity_b, t_id, c_id, t_block, c_block, &simplex)) continue;
        
        (c_block + entity_a.block_indices[c_id])->flag[entity_a.component_indices[c_id]] = DM_COLLISION_FLAG_YES;
        (c_block + entity_b.block_indices[c_id])->flag[entity_b.component_indices[c_id]] = DM_COLLISION_FLAG_YES;
        
        if(!dm_physics_collide_entities(entity_a, entity_b, t_id, c_id, t_block, c_block, &simplex, manager)) return false;
    }
    
    return true;
}

/************
SYSTEM FUNCS
**************/
bool dm_physics_system_run(dm_ecs_system_timing timing, dm_ecs_id system_id, void* context_v)
{
    dm_context* context = context_v;
    dm_ecs_system_manager* system = &context->ecs_manager.systems[timing][system_id];
    dm_physics_manager* manager = system->system_data;
    
    dm_timer t = { 0 };
    // broadphase
    dm_timer_start(&t, context);
    if(!dm_physics_broadphase(system, manager, context)) return false;
    DM_LOG_WARN("Physics broadphase took: %lf ms", dm_timer_elapsed_ms(&t, context));
    
    // narrowphase
    dm_timer_start(&t, context);
    if(!dm_physics_narrowphase(system, manager, context)) return false;
    DM_LOG_WARN("Physics narrowphase took: %lf ms", dm_timer_elapsed_ms(&t, context));
    
    // update
    
    manager->num_manifolds = 0;

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
    
    
#define DM_PHYSICS_SYS_NUM_COMPS 3
    dm_ecs_id comps[DM_PHYSICS_SYS_NUM_COMPS] = {
        context->ecs_manager.default_components.transform,
        context->ecs_manager.default_components.physics,
        context->ecs_manager.default_components.collision
    };
    
    dm_ecs_system_timing timing = DM_ECS_SYSTEM_TIMING_UPDATE_BEGIN;
    context->ecs_manager.default_systems.physics = dm_ecs_register_system(comps, DM_PHYSICS_SYS_NUM_COMPS, timing, dm_physics_system_run, dm_physics_system_shutdown, context);
    
    dm_ecs_system_manager* physics_system = &context->ecs_manager.systems[timing][context->ecs_manager.default_systems.physics];
    physics_system->system_data = dm_alloc(sizeof(dm_physics_manager));
    
    dm_physics_manager* manager = physics_system->system_data;
    
    manager->collision_capacity = DM_PHYSICS_DEFAULT_COLLISION_CAPACITY;
    manager->manifold_capacity  = DM_PHYSICS_DEFAULT_MANIFOLD_CAPACITY;
    manager->possible_collisions = dm_alloc(sizeof(dm_collision_pair) * manager->collision_capacity);
    manager->manifolds = dm_alloc(sizeof(dm_contact_manifold) * manager->manifold_capacity);
    
    return true;
}
