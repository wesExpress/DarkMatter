#include "dm.h"

#include <limits.h>
#include <float.h>
#include <assert.h>

#define DM_PHYSICS_EPA_TOLERANCE     0.00001f
#define DM_PHYSICS_MAX_CONTACTS      8
#define DM_PHYSICS_DIVISION_EPSILON  1e-8f
#define DM_PHYSICS_TEST_EPSILON      1e-4f
#define DM_PHYSICS_PERSISTENT_THRESH 0.001f
#define DM_PHYSICS_W_LIM             50.0f
#define DM_PHYSICS_MAX_MANIFOLDS     20

// 3d support funcs
void dm_physics_support_func_sphere(const float pos[3], const float cen[3], const float internals[6], const float dir[3], float support[3])
{
    const float radius = internals[0];
    float dir_norm[3] = { 0 };
    dm_vec3_norm(dir, dir_norm);
    
    float sup[3] = {
        (dir_norm[0] * radius) + (pos[0] + cen[0]),
        (dir_norm[1] * radius) + (pos[1] + cen[1]),
        (dir_norm[2] * radius) + (pos[2] + cen[2])
    };
    
    DM_VEC3_COPY(support, sup);
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
    float a[3] = { 0 }, b[3] = { 0 }, c[3] = { 0 }, d[3] = { 0 };
    float ab[3] = { 0 }, ac[3] = { 0 }, ad[3] = { 0 }, ao[3] = { 0 };
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
    float sep[3] = { 0 };
    dm_vec3_sub_vec3(pos[0], pos[1], sep);
    if(sep[1]==0 && sep[2]==0) direction[1] = 1;
    else direction[0] = 1;
    
    // start simplex
    float support[3] = { 0 };
    
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
            
            // might not be needed, not sure. better safe than sorry (stack overflows)
            if(num_faces >= DM_PHYSICS_EPA_MAX_FACES) break;
        }
    }
    
    DM_LOG_ERROR("EPA failed to converge after %u iterations", DM_PHYSICS_EPA_MAX_FACES);
    float depth = dm_vec3_dot(faces[closest_face][0], normals[closest_face]);
    dm_vec3_scale(normals[closest_face], depth, penetration);
    
    dm_memcpy(polytope,         faces,   sizeof(float) * 3 * 3 * num_faces);
    dm_memcpy(polytope_normals, normals, sizeof(float) * 3 * num_faces);
    *polytope_count = num_faces;
}

// support face funcs
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
    
    float inv_rot[4], dir[3];
    dm_quat_inverse(rot, inv_rot);
    dm_vec3_rotate(direction, inv_rot, dir);
    
    static const float axes[][3] = {
        {1,0,0},
        {0,1,0},
        {0,0,1}
    };
    
    float best_proximity = -FLT_MAX;
    float best_sgn = 0.0f;
    int   best_axis = -1;
    
    for(uint32_t i=0; i<3; i++)
    {
        float proximity = dm_vec3_dot(dir, axes[i]);
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
    
    assert(constraint->jacobian[0][0]==constraint->jacobian[0][0]);
}

void dm_physics_add_contact_point(const float on_a[3], const float on_b[3], const float normal[3], const float depth, const float pos[2][3], const float rot[2][4], const float vel[2][3], const float w[2][3], dm_contact_manifold* manifold)
{
    if(manifold->point_count>DM_PHYSICS_MAX_MANIFOLDS) return;
    
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
        
#define DM_PHYSICS_BAUMGARTE_COEF 0.1f
#define DM_PHYSICS_BAUMGARTE_SLOP 0.001f
        float ba[3];
        dm_vec3_sub_vec3(on_b, on_a, ba);
        float d = dm_vec3_dot(ba, neg_norm);
        
        b -= (DM_PHYSICS_BAUMGARTE_COEF * DM_PHYSICS_FIXED_DT_INV) * d;
    }
    
    // restitution
    {
#define DM_PHYSICS_REST_COEF 0.1f
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
    
    manifold->points[manifold->point_count++] = p;
}

// collisions
void dm_physics_collide_sphere_sphere(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], dm_contact_manifold* manifold)
{
    const float radius_a = internals[0][0];
    const float radius_b = internals[1][0];
    
    float axis[3], on_a[3], on_b[3];
    float d, depth;
    
    dm_vec3_sub_vec3(pos[1], pos[0], axis);
    d     = dm_vec3_mag(axis);
    depth = d - (radius_b - radius_a);
    dm_vec3_scale(axis, 1 / d, axis);
    
    dm_vec3_scale(axis, radius_a, on_a);
    dm_vec3_add_vec3(pos[0], on_a, on_a);
    
    dm_vec3_scale(axis, radius_b, on_b);
    dm_vec3_sub_vec3(pos[1], on_b, on_b);
    
    dm_physics_add_contact_point(on_a, on_b, axis, depth, pos, rots, vels, ws, manifold);
}

void dm_physics_collide_sphere_poly(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold)
{
    float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3]      = { 0 };
    float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3] = { 0 };
    uint32_t num_faces = 0;
    float penetration[3] = { 0 };
    
    dm_physics_epa(pos, rots, cens, internals, shapes, penetration, polytope, polytope_normals, &num_faces, simplex);
    
    if(penetration[0]!=penetration[0]) return;
    if(dm_vec3_mag(penetration)==0) return;
    
    float norm_pen[3] = { 0 };
    dm_vec3_norm(penetration, norm_pen);
    
    float points[10][3] = { 0 }; 
    float normal[3] = { 0 };
    float neg_pen[3] = { 0 }; 
    float sphere_support[3] = { 0 };
    dm_plane planes[10] = { 0 };
    uint32_t num_points = 0, num_planes = 0;
    
    dm_vec3_negate(norm_pen, neg_pen);
    
    dm_physics_support_func_sphere(pos[0], cens[0], internals[0], norm_pen, sphere_support);
    dm_support_face_box(pos[1], rots[1], cens[1], internals[1], neg_pen, points, &num_points, planes, &num_planes, normal);
    
    dm_plane ref_plane = { 0 };
    DM_VEC3_COPY(ref_plane.normal, normal);
    ref_plane.distance = dm_vec3_dot(points[0], normal);
    
    float div = ref_plane.distance - dm_vec3_dot(sphere_support, ref_plane.normal);
    float rcp = dm_vec3_dot(neg_pen, ref_plane.normal);
    float t = div / rcp;
    
    float on_b[3] = { 0 };
    dm_vec3_scale(neg_pen, t, on_b);
    dm_vec3_add_vec3(sphere_support, on_b, on_b);
    
    dm_physics_add_contact_point(sphere_support, on_b, norm_pen, 0, pos, rots, vels, ws, manifold);
}

bool dm_physics_collide_sphere_other(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold)
{
    switch(shapes[1])
    {
        case DM_COLLISION_SHAPE_SPHERE:
        dm_physics_collide_sphere_sphere(pos, rots, cens, internals, vels, ws, manifold);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        dm_physics_collide_sphere_poly(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
    
    return true;
}

void dm_physics_collide_poly_sphere(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold)
{
    float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3]      = { 0 };
    float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3] = { 0 };
    uint32_t num_faces = 0;
    float penetration[3] = { 0 };
    
    dm_physics_epa(pos, rots, cens, internals, shapes, penetration, polytope, polytope_normals, &num_faces, simplex);
    
    if(penetration[0]!=penetration[0]) return;
    if(dm_vec3_mag(penetration)==0) return;
    
    float norm_pen[3] = { 0 };
    dm_vec3_norm(penetration, norm_pen);
    
    float points[10][3] = { 0 };
    float normal[3] = { 0 };
    float neg_pen[3] = { 0 };
    float sphere_support[3] = { 0 };
    dm_plane planes[10] = { 0 };
    uint32_t num_points = 0, num_planes = 0;
    
    dm_vec3_negate(norm_pen, neg_pen);
    
    dm_support_face_box(pos[0], rots[0], cens[0], internals[0], norm_pen, points, &num_points, planes, &num_planes, normal);
    dm_physics_support_func_sphere(pos[1], cens[1], internals[1], neg_pen, sphere_support);
    
    dm_plane ref_plane = { 0 };
    DM_VEC3_COPY(ref_plane.normal, normal);
    ref_plane.distance = dm_vec3_dot(points[0], normal);
    
    float div = ref_plane.distance - dm_vec3_dot(sphere_support, ref_plane.normal);
    float rcp = dm_vec3_dot(norm_pen, ref_plane.normal);
    float t = div / rcp;
    
    float on_a[3] = { 0 };
    dm_vec3_scale(norm_pen, t, on_a);
    dm_vec3_add_vec3(sphere_support, on_a, on_a);
    
    dm_physics_add_contact_point(on_a, sphere_support, norm_pen, 0, pos, rots, vels, ws, manifold);
}

void dm_physics_collide_poly_poly(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold)
{
    float polytope[DM_PHYSICS_EPA_MAX_FACES][3][3]      = { 0 };
    float polytope_normals[DM_PHYSICS_EPA_MAX_FACES][3] = { 0 };
    uint32_t num_faces = 0;
    float penetration[3] = { 0 };
    
    dm_physics_epa(pos, rots, cens, internals, shapes, penetration, polytope, polytope_normals, &num_faces, simplex);
    
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
    
    dm_support_face_box(pos[0], rots[0], cens[0], internals[0], norm_pen, points_a, &num_pts_a, planes_a, &num_planes_a, normal_a);
    dm_support_face_box(pos[1], rots[1], cens[1], internals[1], neg_pen,  points_b, &num_pts_b, planes_b, &num_planes_b, normal_b);
    
    bool flipped = dm_fabs(dm_vec3_dot(norm_pen, normal_a)) > dm_fabs(dm_vec3_dot(norm_pen, normal_b));
    
    dm_plane ref_plane;
    float    clipped_face[10][3];
    uint32_t num_clipped = 0;
    
    if(flipped) 
    {
        DM_VEC3_COPY(ref_plane.normal, normal_b);
        ref_plane.distance = -dm_vec3_dot(normal_b, points_b[0]);
        
        dm_physics_sutherland_hodgman(points_a, num_pts_a, planes_b, num_planes_b, clipped_face, &num_clipped);
    }
    else
    {
        DM_VEC3_COPY(ref_plane.normal, normal_a);
        ref_plane.distance = -dm_vec3_dot(normal_a, points_a[0]);
        
        dm_physics_sutherland_hodgman(points_b, num_pts_b, planes_a, num_planes_a, clipped_face, &num_clipped);
    }
    
    float point[3];
    float on_a[3], on_b[3], dum[3];
    float contact_pen;
    
    for(uint32_t i=0; i<num_clipped; i++)
    {
        DM_VEC3_COPY(point, clipped_face[i]);
        
        contact_pen = -dm_fabs(dm_vec3_dot(point, ref_plane.normal) + ref_plane.distance);
        contact_pen = DM_MIN(contact_pen, pen_depth);
        
        dm_vec3_scale(norm_pen, contact_pen, dum);
        if(flipped)
        {
            dm_vec3_sub_vec3(point, dum, on_a);
            DM_VEC3_COPY(on_b, point);
        }
        else
        {
            DM_VEC3_COPY(on_a, point);
            dm_vec3_add_vec3(point, dum, on_b);
        }
        
        contact_pen = -(dm_fabs(contact_pen) + dm_fabs(pen_depth)) * 0.5f;
        
        dm_physics_add_contact_point(on_a, on_b, norm_pen, contact_pen, pos, rots, vels, ws, manifold);
    }
}

bool dm_physics_collide_poly_other(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold)
{
    switch(shapes[1])
    {
        case DM_COLLISION_SHAPE_SPHERE:
        dm_physics_collide_poly_sphere(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        dm_physics_collide_poly_poly(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
    
    return true;
}

bool dm_physics_collide_entities(const float pos[2][3], const float rots[2][4], const float cens[2][3], const float internals[2][6], const float vels[2][3], const float ws[2][3], const dm_collision_shape shapes[2], dm_simplex* simplex, dm_contact_manifold* manifold)
{
    switch(shapes[0])
    {
        case DM_COLLISION_SHAPE_SPHERE:
        return dm_physics_collide_sphere_other(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold);
        break;
        
        case DM_COLLISION_SHAPE_BOX:
        return dm_physics_collide_poly_other(pos, rots, cens, internals, vels, ws, shapes, simplex, manifold);
        break;
        
        default:
        DM_LOG_FATAL("Unknown collider type! Shouldn't be here so we are crashing");
        return false;
    }
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
    effective_mass += manifold->contact_data[0].inv_mass * dm_vec3_dot(constraint->jacobian[0], constraint->jacobian[0]);
    effective_mass += dm_vec3_dot(constraint->jacobian[1], dum1);
    effective_mass += manifold->contact_data[1].inv_mass * dm_vec3_dot(constraint->jacobian[2], constraint->jacobian[2]);
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