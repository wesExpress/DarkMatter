#include "dm_filesystem.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "dm_logger.h"
#include "dm_mem.h"

bool dm_filesystem_exists(const char* path)
{
	struct stat buffer;
	return stat(path, &buffer) == 0;
}

bool dm_filesystem_open(const char* path, dm_file_mode mode, bool is_binary, dm_file_handle* handle)
{
	handle->is_valid = false;
	handle->handle = 0;
	const char* mode_str;

	if ( ((mode & DM_FILE_MODE_READ) != 0) && ((mode & DM_FILE_MODE_WRITE) != 0) )
	{
		mode_str = is_binary ? "w+b" : "w+";
	}
	else if ( ((mode & DM_FILE_MODE_READ) != 0) && ((mode & DM_FILE_MODE_WRITE) == 0) )
	{
		mode_str = is_binary ? "rb" : "r";
	}
	else if (((mode & DM_FILE_MODE_READ) == 0) && ((mode & DM_FILE_MODE_WRITE) != 0))
	{
		mode_str = is_binary ? "wb" : "w";
	}
	else
	{
		DM_LOG_ERROR("Invalid mode passed while trying to open file: %s", path);
		return false;
	}

	FILE* file = fopen(path, mode_str);
	if (!file)
	{
		DM_LOG_ERROR("Error opening file: %s", path);
		return false;
	}

	handle->handle = file;
	handle->is_valid = true;

	return true;
}

void dm_filesystem_close(dm_file_handle* handle)
{
	if (handle->handle)
	{
		fclose((FILE*)handle->handle);
		handle->handle = 0;
		handle->is_valid = false;
	}
}

bool dm_filesystem_read_line(dm_file_handle* handle, char** line_buffer)
{
	if (handle->handle)
	{
#define bigbuff 32000
		char buffer[bigbuff];
		if (fgets(buffer, bigbuff, (FILE*)handle->handle) != 0)
		{
			size_t len = strlen(buffer);
			*line_buffer = dm_alloc(sizeof(char) * len + 1);
			strcpy(*line_buffer, buffer);
			return true;
		}
		else
		{
			DM_LOG_ERROR("fgets failed when reading from file!");
			return false;
		}
	}
	else
	{
		DM_LOG_ERROR("Trying to read line from invalid file!");
		return false;
	}
}

bool dm_filesystem_write_line(dm_file_handle* handle, const char* text)
{
	if (handle->handle)
	{
		int result = fputs(text, (FILE*)handle->handle);
		if (result != EOF)
		{
			result = fputc('\n', (FILE*)handle->handle);
		}

		fflush((FILE*)handle->handle);
		return result != EOF;
	}
	else
	{
		DM_LOG_ERROR("Trying to write to invalid file!");
		return false;
	}
}

bool dm_filesystem_read(dm_file_handle* handle, size_t data_size, void* out_data, size_t* bytes_read)
{
	if (handle->handle && out_data)
	{
		*bytes_read = fread(out_data, 1, data_size, (FILE*)handle->handle);
		if (*bytes_read != data_size)
		{
			DM_LOG_ERROR("Something went wrong when reading bytes from file.");
			return false;
		}
		return true;
	}
	else
	{
		DM_LOG_ERROR("Trying to read invalid file!");
		return false;
	}
}

bool dm_filesystem_read_all(dm_file_handle* handle, uint8_t** out_bytes, size_t* bytes_read)
{
	if (handle->handle)
	{
		fseek((FILE*)handle->handle, 0, SEEK_END);
		size_t size = ftell((FILE*)handle->handle);
		rewind((FILE*)handle->handle);

		*out_bytes = dm_alloc(sizeof(uint8_t) * size);
		*bytes_read = fread(*out_bytes, 1, size, (FILE*)handle->handle);
		if (*bytes_read != size)
		{
			DM_LOG_ERROR("Something went wrong when reading all bytes from file.");
			return false;
		}
		return true;
	}
	else
	{
		DM_LOG_ERROR("Trying to read all bytes from invalid file!");
		return false;
	}
}

bool dm_filesystem_write(dm_file_handle* handle, size_t data_size, const void* data, size_t* bytes_written)
{
	if (handle->handle)
	{
		*bytes_written = fwrite(data, 1, data_size, (FILE*)handle->handle);
		if (*bytes_written != data_size)
		{
			DM_LOG_ERROR("Something went wrong when writing to file.");
			return false;
		}
		fflush((FILE*)handle->handle);
		return true;
	}
	else
	{
		DM_LOG_ERROR("Trying to write to invalid file!");
		return false;
	}
}