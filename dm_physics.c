#include "dm.h"

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
int dm_physics_broadphase_sort(void* c, const void* a, const void* b)
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
    dm_timer t = { 0 };
    dm_timer_start(&t, context);
    
    float center_sum[3];
    float center_sq_sum[3];
    int axis = 0;
    
    uint32_t component_count = system->component_count;
    
    dm_component_collision_block* c_block;
    dm_component_transform_block* t_block;
    
    uint32_t (*entity_component_indices)[DM_ECS_MAX] = context->ecs_manager.entity_component_indices;
    uint32_t  t_id = context->ecs_manager.default_components.transform;
    uint32_t  c_id = context->ecs_manager.default_components.collision;
    
    uint32_t  t_comp_index, t_block_index, t_index, t_i;
    uint32_t  c_comp_index, c_block_index, c_index, c_i;
    dm_entity entity;
    
    float a,b = 0.0f;
    float quat[N4];
    float rot[M3];
    float center[N3];
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        t_i = i * component_count;
        c_i = i * component_count + 2;
        
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
    
    DM_LOG_DEBUG("Get axis took: %lf ms", dm_timer_elapsed_ms(&t, context));
    
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
    
    dm_timer t = { 0 };
    dm_timer_start(&t, context);
    
    dm_ecs_system_entity_container* test = &system->entity_containers[100];
    
    // sort
    switch(axis)
    {
        case 0:
        {
            data.min = block->aabb_global_min_x;
            qsort_s(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
        } break;
        
        case 1:
        {
            data.min = block->aabb_global_min_y;
            qsort_s(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
        } break;
        
        case 2:
        {
            data.min = block->aabb_global_min_z;
            qsort_s(system->entity_containers, system->entity_count, sizeof(dm_ecs_system_entity_container), dm_physics_broadphase_sort, &data);
        } break;
    }
    DM_LOG_DEBUG("Sorting took: %lf ms", dm_timer_elapsed_ms(&t, context));
    
    // sweep
    float max_i, min_j;
    dm_ecs_system_entity_container entity_a, entity_b;
    const size_t size = sizeof(dm_component_collision_block);
    uint32_t a_c_index, a_b_index, b_c_index, b_b_index;
    
    float* maxes, *mins;
    switch(axis)
    {
        case 0:
        mins  = block->aabb_global_min_x;
        maxes = block->aabb_global_max_x;
        break;
        
        case 1:
        mins  = block->aabb_global_min_y;
        maxes = block->aabb_global_max_y;
        break;
        
        case 2:
        mins  = block->aabb_global_min_z;
        maxes = block->aabb_global_max_z;
        break;
    }
    
    dm_collision_flag* flags = block->flag;
    
    dm_timer_start(&t, context);
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        entity_a = system->entity_containers[i];
        
        a_c_index = entity_a.component_indices[data.index];
        a_b_index = entity_a.block_indices[data.index];
        
        max_i = ((float*)((char*)maxes + a_b_index * size))[a_c_index];
        
        for(uint32_t j=i+1; j<system->entity_count; j++)
        {
            entity_b = system->entity_containers[j];
            
            b_c_index = entity_b.component_indices[data.index];
            b_b_index = entity_b.block_indices[data.index];
            
            min_j = ((float*)((char*)mins + b_b_index * size))[b_c_index];
            if(min_j > max_i) break;
            
            manager->possible_collisions[manager->num_possible_collisions].entity_a = entity_a;
            manager->possible_collisions[manager->num_possible_collisions].entity_b = entity_b;
            manager->num_possible_collisions++;
            
            ((dm_collision_flag*)((char*)flags + a_b_index * size))[a_c_index] = DM_COLLISION_FLAG_POSSIBLE;
            ((dm_collision_flag*)((char*)flags + b_b_index * size))[b_c_index] = DM_COLLISION_FLAG_POSSIBLE;
            
            float load = (float)manager->num_possible_collisions / (float)manager->collision_capacity;
            if(load < DM_PHYSICS_LOAD_FACTOR) continue;
            
            manager->collision_capacity *= DM_PHYSICS_RESIZE_FACTOR;
            manager->possible_collisions = dm_realloc(manager->possible_collisions, sizeof(dm_collision_pair) * manager->collision_capacity);
        }
    }
    DM_LOG_DEBUG("Sweeping took: %lf ms", dm_timer_elapsed_ms(&t, context));
    
    return true;
}

/***********
NARROWPHASE
*************/
// 3d support funcs
void dm_support_func_sphere(float pos[3], float cen[3], void* data, float d[3], float support[3])
{
    float radius = *(float*)data;
    
    support[0] = (d[0] * radius) + (pos[0] + cen[0]); 
    support[1] = (d[1] * radius) + (pos[1] + cen[1]); 
    support[2] = (d[2] * radius) + (pos[2] + cen[2]);
}

void dm_support_func_box(float pos[3], float rot[4], float cen[3], void* data, float d[3], float support[3])
{
    float inv_rot[4];
    float p[3];
    float box[6];
    dm_memcpy(box, data, sizeof(box));
    
    dm_quat_inverse(rot, inv_rot);
    dm_vec3_rotate(d, inv_rot, d);
    
    support[0] = (d[0] > 0) ? box[3] : box[0];
    support[1] = (d[1] > 0) ? box[4] : box[1];
    support[2] = (d[2] > 0) ? box[5] : box[2];
    
    dm_vec3_rotate(support, rot, support);
    dm_vec3_add_vec3(pos, cen, p);
    dm_vec3_add_vec3(support, p, support);
    dm_vec3_rotate(d, rot, d);
}

// 2d support funcs

// gjk
void dm_physics_gjk_support(float pos[3], float rot[4], float cen[3], void* data, dm_collision_shape shape, float direction[3], float support[3])
{
    switch(shape)
    {
        case DM_COLLISION_SHAPE_SPHERE: 
        dm_support_func_sphere(pos, cen, data, direction, support);
        break;
        
        case DM_COLLISION_SHAPE_BOX: 
        dm_support_func_box(pos, rot, cen, data, direction, support);
        break;
        
        default:
        DM_LOG_ERROR("Collision shape not supported, or unknown shape! Probably shouldn't be here...");
        break;
    }
}

void dm_support(dm_entity entity_a, dm_entity entity_b, float direction[3], float out[3], dm_context* context)
{
#if 0 
    float dir_neg[3];
    float support_a[3];
    float support_b[3];
    
    dm_vec3_negate(direction, dir_neg);
    
    dm_component_transform transform_a = dm_ecs_entity_get_transform(entity_a, context);
    dm_component_transform transform_b = dm_ecs_entity_get_transform(entity_b, context);
    
    dm_component_collision collision_a = dm_ecs_entity_get_collision(entity_a, context);
    dm_component_collision collision_b = dm_ecs_entity_get_collision(entity_b, context);
    
    dm_physics_gjk_support(transform_a.pos, transform_a.rot, collision_a.center, collision_a.internal, collision_a.shape, direction, support_a);
    dm_physics_gjk_support(transform_b.pos, transform_b.rot, collision_b.center, collision_b.internal, collision_b.shape, direction, support_b);
    
    dm_vec3_sub_vec3(support_a, support_b, out);
#endif
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

bool dm_physics_gjk(dm_entity entity_a, dm_entity entity_b, dm_simplex* simplex, dm_context* context)
{
    // initial guess should not matter, but algorithm WILL FAIL if initial guess is
    // perfectly aligned with global unit axes
    
    float direction[3] = { 0,1,0 };
    
    // start simplex
    float support[3];
    
    dm_support(entity_a, entity_b, direction, support, context);
    dm_vec3_negate(support, direction);
    dm_simplex_push_front(support, simplex);
    
    for(uint32_t iter=0; iter<DM_PHYSICS_MAX_GJK_ITER; iter++)
    {
        if(simplex->size > 4) DM_LOG_ERROR("GJK simplex greater than 4...?");
        
        dm_support(entity_a, entity_b, direction, support, context);
        if(dm_vec3_dot(support, direction) < 0 ) return false;
        
        dm_simplex_push_front(support, simplex);
        
        // if we have a collision, find penetration depth
        if(dm_next_simplex(direction, simplex)) return true;
    }
    
    DM_LOG_ERROR("GJK failed to converge after %u iterations", DM_PHYSICS_MAX_GJK_ITER);
    return false;
}

bool dm_physics_narrowphase(dm_ecs_system_manager* system, dm_physics_manager* manager, dm_context* context)
{
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
    
    // broadphase
    if(!dm_physics_broadphase(system, manager, context)) return false;
    
    // narrowphase
    if(!dm_physics_narrowphase(system, manager, context)) return false;
    
    // update
    
    return true;
}

void dm_physics_system_shutdown(dm_ecs_system_timing timing, dm_ecs_id system_id, dm_context* context)
{
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
