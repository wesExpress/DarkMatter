#ifndef DM_PHYSICS_H
#define DM_PHYSICS_H

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

// collision handling
typedef struct dm_plane
{
    float normal[3];
    float distance;
} dm_plane;

#define DM_PHYSICS_DEFAULT_COLLISION_CAPACITY 16
#define DM_PHYSICS_DEFAULT_MANIFOLD_CAPACITY  16
#define DM_PHYSICS_LOAD_FACTOR                0.75f
#define DM_PHYSICS_RESIZE_FACTOR              1.5f

// system funcs
bool dm_physics_system_insert_entity(dm_entity entity, dm_ecs_system_timing timing, dm_ecs_id id, dm_context* context)
{
    dm_ecs_system_manager* manager = &context->ecs_manager.systems[timing][id];
    
    uint32_t sys_index = manager->entity_count;
    
    if(entity>=DM_ECS_COMPONENT_BLOCK_SIZE) return true;
    
    // transform
    uint32_t index;
    dm_component_transform_block* t_block = dm_ecs_entity_get_component_block(entity, context->ecs_manager.default_components.transform, &index, context);
    dm_system_component_transform_block* sys_t_block = (void*)((char*)manager->data + manager->block_size * (manager->block_count-1));
    
    sys_t_block->pos_x[sys_index] = &t_block->pos_x[index];
    sys_t_block->pos_y[sys_index] = &t_block->pos_y[index];
    sys_t_block->pos_z[sys_index] = &t_block->pos_z[index];
    
    sys_t_block->scale_x[sys_index] = &t_block->scale_x[index];
    sys_t_block->scale_y[sys_index] = &t_block->scale_y[index];
    sys_t_block->scale_z[sys_index] = &t_block->scale_z[index];
    
    sys_t_block->rot_i[sys_index] = &t_block->rot_i[index];
    sys_t_block->rot_j[sys_index] = &t_block->rot_j[index];
    sys_t_block->rot_k[sys_index] = &t_block->rot_k[index];
    sys_t_block->rot_r[sys_index] = &t_block->rot_r[index];
    
    // physics
    dm_component_physics_block* p_block = dm_ecs_entity_get_component_block(entity, context->ecs_manager.default_components.physics, &index, context);
    
    dm_system_component_physics_block* sys_p_block = (void*)((char*)manager->data + manager->block_size * (manager->block_count-1) + sizeof(dm_system_component_transform_block));
    
    sys_p_block->vel_x[sys_index] = &p_block->vel_x[index];
    sys_p_block->vel_y[sys_index] = &p_block->vel_y[index];
    sys_p_block->vel_z[sys_index] = &p_block->vel_z[index];
    
    sys_p_block->w_x[sys_index] = &p_block->w_x[index];
    sys_p_block->w_y[sys_index] = &p_block->w_y[index];
    sys_p_block->w_z[sys_index] = &p_block->w_z[index];
    
    sys_p_block->l_x[sys_index] = &p_block->l_x[index];
    sys_p_block->l_y[sys_index] = &p_block->l_y[index];
    sys_p_block->l_z[sys_index] = &p_block->l_z[index];
    
    sys_p_block->force_x[sys_index] = &p_block->force_x[index];
    sys_p_block->force_y[sys_index] = &p_block->force_y[index];
    sys_p_block->force_z[sys_index] = &p_block->force_z[index];
    
    sys_p_block->torque_x[sys_index] = &p_block->torque_x[index];
    sys_p_block->torque_y[sys_index] = &p_block->torque_y[index];
    sys_p_block->torque_z[sys_index] = &p_block->torque_z[index];
    
    sys_p_block->mass[sys_index] = &p_block->mass[index];
    sys_p_block->inv_mass[sys_index] = &p_block->inv_mass[index];
    
    sys_p_block->i_body_0[sys_index] = &p_block->i_body_0[index];
    sys_p_block->i_body_1[sys_index] = &p_block->i_body_1[index];
    sys_p_block->i_body_2[sys_index] = &p_block->i_body_2[index];
    
    sys_p_block->i_body_inv_0[sys_index] = &p_block->i_body_inv_0[index];
    sys_p_block->i_body_inv_1[sys_index] = &p_block->i_body_inv_1[index];
    sys_p_block->i_body_inv_2[sys_index] = &p_block->i_body_inv_2[index];
    
    sys_p_block->i_inv_0_0[sys_index] = &p_block->i_inv_0_0[index];
    sys_p_block->i_inv_0_1[sys_index] = &p_block->i_inv_0_1[index];
    sys_p_block->i_inv_0_2[sys_index] = &p_block->i_inv_0_2[index];
    
    sys_p_block->i_inv_1_0[sys_index] = &p_block->i_inv_1_0[index];
    sys_p_block->i_inv_1_1[sys_index] = &p_block->i_inv_1_1[index];
    sys_p_block->i_inv_1_2[sys_index] = &p_block->i_inv_1_2[index];
    
    sys_p_block->i_inv_2_0[sys_index] = &p_block->i_inv_2_0[index];
    sys_p_block->i_inv_2_1[sys_index] = &p_block->i_inv_2_1[index];
    sys_p_block->i_inv_2_2[sys_index] = &p_block->i_inv_2_2[index];
    
    sys_p_block->damping_v[sys_index] = &p_block->damping_v[index];
    sys_p_block->damping_w[sys_index] = &p_block->damping_w[index];
    
    sys_p_block->body_type[sys_index]     = &p_block->body_type[index];
    sys_p_block->movement_type[sys_index] = &p_block->movement_type[index];
    
    // collision
    dm_component_collision_block* c_block = dm_ecs_entity_get_component_block(entity, context->ecs_manager.default_components.collision, &index, context);
    
    dm_system_component_collision_block* sys_c_block = (void*)((char*)manager->data + manager->block_size * (manager->block_count-1) + sizeof(dm_system_component_transform_block) + sizeof(dm_system_component_physics_block));
    
    sys_c_block->aabb_local_min_x[sys_index] = &c_block->aabb_local_min_x[index];
    sys_c_block->aabb_local_min_y[sys_index] = &c_block->aabb_local_min_y[index];
    sys_c_block->aabb_local_min_z[sys_index] = &c_block->aabb_local_min_z[index];
    
    sys_c_block->aabb_local_max_x[sys_index] = &c_block->aabb_local_max_x[index];
    sys_c_block->aabb_local_max_y[sys_index] = &c_block->aabb_local_max_y[index];
    sys_c_block->aabb_local_max_z[sys_index] = &c_block->aabb_local_max_z[index];
    
    sys_c_block->aabb_global_min_x[sys_index] = &c_block->aabb_global_min_x[index];
    sys_c_block->aabb_global_min_y[sys_index] = &c_block->aabb_global_min_y[index];
    sys_c_block->aabb_global_min_z[sys_index] = &c_block->aabb_global_min_z[index];
    
    sys_c_block->aabb_global_max_x[sys_index] = &c_block->aabb_global_max_x[index];
    sys_c_block->aabb_global_max_y[sys_index] = &c_block->aabb_global_max_y[index];
    sys_c_block->aabb_global_max_z[sys_index] = &c_block->aabb_global_max_z[index];
    
    sys_c_block->center_x[sys_index] = &c_block->center_x[index];
    sys_c_block->center_y[sys_index] = &c_block->center_y[index];
    sys_c_block->center_z[sys_index] = &c_block->center_z[index];
    
    sys_c_block->internal_0[sys_index] = &c_block->internal_0[index];
    sys_c_block->internal_1[sys_index] = &c_block->internal_1[index];
    sys_c_block->internal_2[sys_index] = &c_block->internal_2[index];
    sys_c_block->internal_3[sys_index] = &c_block->internal_3[index];
    sys_c_block->internal_4[sys_index] = &c_block->internal_4[index];
    sys_c_block->internal_5[sys_index] = &c_block->internal_5[index];
    
    sys_c_block->shape[sys_index] = &c_block->shape[index];
    sys_c_block->flag[sys_index] = &c_block->flag[index];
    
    manager->entity_count++;
    
    return true;
}

bool dm_physics_system_run(dm_context* context)
{
    return true;
}

// impl
bool dm_physics_init(dm_ecs_id* physics_id, dm_ecs_id* collision_id, dm_context* context)
{
    *physics_id = dm_ecs_register_component(sizeof(dm_component_physics_block), context);
    if(*physics_id==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register physics component"); return true; }
    
    *collision_id = dm_ecs_register_component(sizeof(dm_component_collision_block), context);
    if(*collision_id==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not register collision component"); return true; }
    
    context->physics_manager.possible_collisions = dm_alloc(sizeof(dm_collision_pair) * DM_PHYSICS_DEFAULT_COLLISION_CAPACITY);
    context->physics_manager.manifolds = dm_alloc(sizeof(dm_contact_manifold) * DM_PHYSICS_DEFAULT_MANIFOLD_CAPACITY);
    
#define DM_PHYSICS_SYS_NUM_COMPS 3
    
    size_t block_sizes[DM_PHYSICS_SYS_NUM_COMPS] = {
        sizeof(dm_system_component_transform_block),
        sizeof(dm_system_component_physics_block),
        sizeof(dm_system_component_collision_block)
    };
    
    dm_ecs_id comps[DM_PHYSICS_SYS_NUM_COMPS] = {
        context->ecs_manager.default_components.transform,
        context->ecs_manager.default_components.physics,
        context->ecs_manager.default_components.collision
    };
    
    dm_ecs_id sys_id = dm_ecs_register_system(block_sizes, comps, DM_PHYSICS_SYS_NUM_COMPS, DM_ECS_SYSTEM_TIMING_UPDATE_BEGIN, dm_physics_system_insert_entity, dm_physics_system_run, context);
    
    return true;
}

void dm_physics_shutdown(dm_context* context)
{
    dm_free(context->physics_manager.possible_collisions);
    dm_free(context->physics_manager.manifolds);
}

/****************
3D SUPPORT FUNCS
******************/
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

/****************
2D SUPPORT FUNCS
******************/

/***
GJK
*****/
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

#endif //DM_PHYSICS_H
