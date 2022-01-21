#include "dm_map.h"
#include "dm_mem.h"
#include "dm_logger.h"
#include "string.h"

dm_map_link_list* dm_map_allocate_list();
dm_map_link_list* dm_map_list_insert(dm_map_link_list* list, dm_map_item* item);
dm_map_item* dm_map_list_remove(dm_map_link_list* list);
void dm_map_delete_list(dm_map_link_list* list);
dm_map_link_list** dm_map_create_overflow(dm_map* map);
void dm_map_delete_overflow(dm_map* map);

void dm_map_collision(dm_map* map, unsigned long index, dm_map_item* item);

unsigned long simple_hash_function(const char* str)
{
    unsigned long index = 0;
    for (int i=0; str[i]; i++)
    {
        index += str[i];
    }
    return index % CAPACITY;
}

dm_map* dm_map_create(int size)
{
    dm_map* map = (dm_map*)dm_alloc(sizeof(dm_map), DM_MEM_MAP);
    map->size = size;
    map->count = 0;
    map->items = (dm_map_item**)calloc(map->size, sizeof(dm_map_item*));
    for (int i=0; i<size;i++) map->items[i] = NULL;
    map->overflows = dm_map_create_overflow(map);
    return map;
}

dm_map_item* dm_map_create_item(char* key, char* value)
{
    dm_map_item* item = (dm_map_item*)dm_alloc(sizeof(dm_map_item));
    item->key = (char*)dm_alloc(strlen(key)+1);
    item->value = (char*)dm_alloc(strlen(value)+1);

    strcpy(item->key, key);
    strcpy(item->value, value);

    return item;
}

void dm_map_delete_item(dm_map_item* item)
{
    dm_free(item->key);
    dm_free(item->value);
    dm_free(item);
}

void dm_map_delete(dm_map* map)
{
    for(int i=0; i<map->size; i++)
    {
        if(map->items[i]) dm_map_delete_item(map->items[i]);
    }
    dm_map_delete_overflow(map);
    dm_free(map->items);
    dm_free(map);
}

void dm_map_insert(dm_map* map, char* key, char* value)
{
    int index = simple_hash_function(key);

    if(!map->items[index])
    {
        if(map->count == map->size)
        {
            DM_LOG_ERROR("Trying to insert into full map!");
            return;
        }

         dm_map_item* item = dm_map_create_item(key, value);
         map->items[index] = item;
         map->count++;
    }
    else
    {
        if(strcmp(map->items[index]->key, key) == 0)
        {
            strcpy(map->items[index]->value, value);
            return;
        }
        else
        {
            dm_map_item* item = dm_map_create_item(key, value);
            dm_map_collision(map, index, item);
            return;
        }
    }
}

char* dm_map_search(dm_map* map, char* key)
{
    int index = simple_hash_function(key);
    dm_map_item* item = map->items[index];
    dm_map_link_list* head = map->overflows[index];

    while(item)
    {
        if(strcmp(item->key, key)==0) return item->value;
        if(!head) return NULL;
        item = head->item;
        head = head->next;
    }

    return NULL;
}

void dm_map_print(dm_map* map)
{
    for(int i =0; i<map->size; i++)
    {
        if(map->items[i])
        {
            DM_LOG_DEBUG("Index: %d; Key: %s; Value: %s", i, map->items[i]->key, map->items[i]->value);
        }
    }
}

void dm_map_search_print(dm_map* map, char* key)
{
    char* val;
    if((val = dm_map_search(map, key)) == NULL) DM_LOG_DEBUG("Key not found.");
    else DM_LOG_DEBUG("Key: %s; Value: %s", key, val);
}

// collisions
void dm_map_collision(dm_map* map, unsigned long index, dm_map_item* item)
{
    dm_map_link_list* head = map->overflows[index];

    if(!head)
    {
        head = dm_map_allocate_list();
        head->item = item;
        map->overflows[index] = head;
    }
    else
    {
        map->overflows[index] = dm_map_list_insert(head, item);
    }
}

dm_map_link_list* dm_map_allocate_list()
{
    dm_map_link_list* list = (dm_map_link_list*)dm_alloc(sizeof(dm_map_link_list));
    return list;
}

dm_map_link_list* dm_map_list_insert(dm_map_link_list* list, dm_map_item* item)
{
    if(!list)
    {
        dm_map_link_list* head = dm_map_allocate_list();
        head->item = item;
        head->next = NULL;
        list = head;
        return list;
    }
    else if(list->next == NULL)
    {
        dm_map_link_list* node = dm_map_allocate_list();
        node->item = item;
        node->next = NULL;
        list = node;
        return list;
    }

    // find end of list to append to
    dm_map_link_list* temp = list;
    while(temp->next->next) temp = temp->next;

    // attach the new node
    dm_map_link_list* node = dm_map_allocate_list();
    node->item = item;
    node->next = NULL;
    temp->next = node;

    return list;
}

dm_map_item* dm_map_list_remove(dm_map_link_list* list)
{
    if(!list) return NULL;
    if(!list->next) return NULL;

    dm_map_link_list* node = list->next;
    dm_map_link_list* temp = list;
    temp->next = NULL;
    list = node;

    dm_map_item* item = NULL;
    dm_memcpy(temp->item, item, sizeof(dm_map_item));
    dm_free(temp->item->key);
    dm_free(temp->item->value);
    dm_free(temp->item);
    dm_free(temp);

    return item;
}

void dm_map_delete_list(dm_map_link_list* list)
{
    dm_map_link_list* temp = list;

    while(list)
    {
        temp = list;
        list = list->next;
        dm_free(temp->item->key);
        dm_free(temp->item->value);
        dm_free(temp->item);
        dm_free(temp);
    }
}

dm_map_link_list** dm_map_create_overflow(dm_map* map)
{
    dm_map_link_list** buckets = (dm_map_link_list**)calloc(map->size, sizeof(dm_map_link_list*));
    for(int i=0; i<map->size;i++) buckets[i] = NULL;
    return buckets;
}

void dm_map_delete_overflow(dm_map* map)
{
    dm_map_link_list** buckets = map->overflows;
    for(int i =0; i< map->size; i++) dm_map_delete_list(buckets[i]);
    dm_free(buckets);
}