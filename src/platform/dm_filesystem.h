#ifndef __DM_FILESYSTEM_H__
#define __DM_FILESYSTEM_H__

#include "dm_defines.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct dm_file_handle
{
	void* handle;
	bool is_valid;
} dm_file_handle;

typedef enum dm_file_mode
{
	DM_FILE_MODE_READ,
	DM_FILE_MODE_WRITE
} dm_file_mode;

bool dm_filesystem_exists(const char* path);
bool dm_filesystem_open(const char* path, dm_file_mode mode, bool is_binary, dm_file_handle* handle);
void dm_filesystem_close(dm_file_handle* handle);

bool dm_filesystem_read_line(dm_file_handle* handle, char** line_buffer);
bool dm_filesystem_write_line(dm_file_handle* handle, const char* text);
bool dm_filesystem_read(dm_file_handle* handle, size_t data_size, void* out_data, uint64_t* bytes_read);
bool dm_filesystem_read_all(dm_file_handle* handle, uint8_t** out_bytes, size_t* bytes_read);
bool dm_filesystem_write(dm_file_handle* handle, size_t data_size, const void* data, size_t* bytes_written);

#endif