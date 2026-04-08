// EXTERNAL INCLUDES
#include <stdio.h>
#ifdef _WIN32
#	include <Windows.h>
#endif
// INTERNAL INCLUDES
#include "commands.h"

typedef struct
{
	char* data;
	size_t size;
} _command_result_t;

// ***************************************************************
bool command_trim(_command_result_t* result)
{
	if (!result || !result->data)
	{
        return false;
	}

	if (result->size <= 0)
	{
        return false;
	}

	if (result->size && result->data[result->size - 1] == '\n')
	{
		result->data[result->size - 1] = '\0';
	}

	return true;
}
// ***************************************************************
bool command_run(
	arena_t* arena,
	const char* cmd,
	result_handle_t* result
)
{
	if (!arena || !cmd) return false;

	// Create temporary file
	char temp_path[MAX_PATH];
	char temp_file[MAX_PATH];

	if (!GetTempPathA(MAX_PATH, temp_path)) return false;
	if (!GetTempFileNameA(temp_path, "cmd", 0, temp_file)) return false;

	HANDLE file = CreateFileA(
		temp_file,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
		NULL
	);

	if (file == INVALID_HANDLE_VALUE)
		return false;

	// Ensure handle is inheritable
	SetHandleInformation(file, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	STARTUPINFOA si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = file;

	PROCESS_INFORMATION pi = { 0 };

	// Build command line for cmd.exe
	char cmdline[1024];
	MEMORY_ZERO(cmdline, sizeof(cmdline));

	snprintf(cmdline, sizeof(cmdline), "cmd.exe /C %s", cmd);

	BOOL ok = CreateProcessA(
		NULL,
		cmdline,
		NULL,
		NULL,
		TRUE, // inherit handles
		0,
		NULL,
		NULL,
		&si,
		&pi
	);

	if (!ok)
	{
		CloseHandle(file);
		return false;
	}

	// Wait for process
	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	// Get size
	LARGE_INTEGER size_li;
	if (!GetFileSizeEx(file, &size_li))
	{
		CloseHandle(file);
		return false;
	}

	size_t size = (size_t)size_li.QuadPart;

	// Allocate exact memory from arena
	char* buffer = arena_alloc(arena, size + 1, 8);
	if (!buffer)
	{
		CloseHandle(file);
		return false;
	}

	// Read file
	SetFilePointer(file, 0, NULL, FILE_BEGIN);

	DWORD read_bytes = 0;
	if (!ReadFile(file, buffer, (DWORD)size, &read_bytes, NULL))
	{
		CloseHandle(file);
		return false;
	}

	CloseHandle(file);

	buffer[size] = '\0';

	_command_result_t* _result = (_command_result_t*)arena_alloc(
		arena,
		sizeof(_command_result_t),
		alignof(size_t)
	);

	if (_result)
	{
		_result->data = buffer;
		_result->size = size;

		*result = (result_handle_t)_result;
	}

	return true;
}
// ***************************************************************
size_t command_count_lines(
	result_handle_t result
)
{
	_command_result_t* res = (_command_result_t*)result;

	if (!res || !res->data || res->size == 0)
		return 0;

	size_t line_count = 0;
	char* p = res->data;
	while (*p)
	{
		if (*p == '\n')
			line_count++;
		p++;
	}

	return line_count;
}
// ***************************************************************
char* command_line_get(
	result_handle_t result
)
{
	_command_result_t* res = (_command_result_t*)result;
	
	if (!res || !res->data || res->size == 0)
		return "";

	if (command_trim(res))
	{
		return res->data;
	}

	return "";
}
//********************************************************************************************
char* escape_shell_arg(arena_t* arena, const char* arg)
{
    if (arg == NULL)
        return "";

    size_t len = strlen(arg);
    // Worst case: every character is a single quote needing 4 extra bytes + 2 for outer quotes
    size_t max_len = len * 4 + 3;
    char* escaped = arena_alloc(arena, max_len, 1);

    if (!escaped)
        return "";

    char* p = escaped;
    *p++ = '\'';

    for (size_t i = 0; i < len; i++)
    {
        if (arg[i] == '\'')
        {
            // Close quote, insert '\'' and reopen
            strcpy(p, "'\\''");
            p += 4;
        }
        else
        {
            *p++ = arg[i];
        }
    }

    *p++ = '\'';
    *p = '\0';

    return escaped;
}
// ***************************************************************
