#include "gravity_system.h"
#include "components.h"
#include "debug_render_pass.h"

#define G 6.673e-11f

typedef struct gravity_manager_t
{
    dm_ecs_id transform, physics;
} gravity_manager;

void naive_gravity(dm_ecs_system_manager* system, dm_context* context);

/************
SYSTEM FUNCS
**************/
bool gravity_system_init(dm_ecs_id t_id, dm_ecs_id p_id, dm_context* context)
{
    dm_ecs_id comps[] = { t_id, p_id };
    
    dm_ecs_system_timing timing = DM_ECS_SYSTEM_TIMING_UPDATE_BEGIN;
    dm_ecs_id id = dm_ecs_register_system(comps, DM_ARRAY_LEN(comps), timing, gravity_system_run, gravity_system_shutdown, context);
    if(id==DM_ECS_INVALID_ID) { DM_LOG_FATAL("Could not initialize graity system."); return false; }
    
    dm_ecs_system_manager* gravity_system = &context->ecs_manager.systems[timing][id];
    
    gravity_system->system_data = dm_alloc(sizeof(gravity_manager));
    gravity_manager* manager = gravity_system->system_data;
    
    manager->transform = t_id;
    manager->physics = p_id;
    
    return true;
}

void gravity_system_shutdown(void* s, void* c)
{
}

bool gravity_system_run(void* s, void* c)
{
    dm_context* context = c;
    dm_ecs_system_manager* system = s;
    
    dm_timer t = { 0 };
    dm_timer_start(&t, context);
    naive_gravity(system, context);
    DM_LOG_INFO("Gravity took: %lf ms (%u entities)", dm_timer_elapsed_ms(&t, context), system->entity_count);
    
    return true;
}

/************
NAIVE GRAITY
**************/
void naive_gravity(dm_ecs_system_manager* system, dm_context* context)
{
    gravity_manager* manager = system->system_data;
    
    const dm_ecs_id t_id = manager->transform;
    const dm_ecs_id p_id = manager->physics;
    
    dm_ecs_system_entity_container entity_a, entity_b;
    
    component_transform_block* t_block = dm_ecs_get_component_block(t_id, context);
    component_physics_block*   p_block = dm_ecs_get_component_block(p_id, context);
    
    component_transform_block* a_t_block = NULL;
    component_physics_block*   a_p_block = NULL;
    component_transform_block* b_t_block = NULL;
    component_physics_block*   b_p_block = NULL;
    
    uint32_t a_t_index, a_p_index;
    uint32_t b_t_index, b_p_index;
    
    float mass_a, mass_b;
    float pos_a_x, pos_a_y, pos_a_z;
    float pos_b_x, pos_b_y, pos_b_z;
    
    float sep_x, sep_y, sep_z;
    float dis2, grav;
    float local_f_x, local_f_y, local_f_z;
    
    for(uint32_t i=0; i<system->entity_count; i++)
    {
        entity_a = system->entity_containers[i];
        
        a_t_block = t_block + entity_a.block_indices[t_id];
        a_p_block = p_block + entity_a.block_indices[p_id];
        
        a_t_index = entity_a.component_indices[t_id];
        a_p_index = entity_a.component_indices[p_id];
        
        pos_a_x = a_t_block->pos_x[a_t_index];
        pos_a_y = a_t_block->pos_y[a_t_index];
        pos_a_z = a_t_block->pos_z[a_t_index];
        
        mass_a = a_p_block->mass[a_p_index];
        
        float f_x = a_p_block->force_x[a_t_index];
        float f_y = a_p_block->force_y[a_t_index];
        float f_z = a_p_block->force_z[a_t_index];
        
        for(uint32_t j=i+1; j<system->entity_count; j++)
        {
            entity_b = system->entity_containers[j];
            
            b_t_block = t_block + entity_b.block_indices[t_id];
            b_p_block = p_block + entity_b.block_indices[p_id];
            
            b_t_index = entity_b.component_indices[t_id];
            b_p_index = entity_b.component_indices[p_id];
            
            pos_b_x = b_t_block->pos_x[b_t_index];
            pos_b_y = b_t_block->pos_y[b_t_index];
            pos_b_z = b_t_block->pos_z[b_t_index];
            
            mass_b = b_p_block->mass[b_p_index];
            
            // force calculation
            sep_x = pos_b_x - pos_a_x;
            sep_y = pos_b_y - pos_a_y;
            sep_z = pos_b_z - pos_a_z;
            
            dis2  = sep_x * sep_x;
            dis2 += sep_y * sep_y;
            dis2 += sep_z * sep_z;
            
            grav  = G * mass_a;
            grav *= mass_b;
            grav /= dis2;
            
            dis2 = dm_sqrtf(dis2);
            dis2 = 1.0f / dis2;
            
            sep_x *= dis2;
            sep_y *= dis2;
            sep_z *= dis2;
            
            local_f_x = sep_x * grav;
            local_f_y = sep_y * grav;
            local_f_z = sep_z * grav;
            
            // a gets force, b gets negative!
            a_p_block->force_x[a_p_index] += local_f_x;
            a_p_block->force_y[a_p_index] += local_f_y;
            a_p_block->force_z[a_p_index] += local_f_z;
            
            b_p_block->force_x[b_p_index] -= local_f_x;
            b_p_block->force_y[b_p_index] -= local_f_y;
            b_p_block->force_z[b_p_index] -= local_f_z;
        }
    }
}