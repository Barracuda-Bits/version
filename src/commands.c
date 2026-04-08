// EXTERNAL INCLUDES
#include <stdio.h>
#ifdef _WIN32
#	include <Windows.h>
#else
#	include <unistd.h>
#	include <sys/wait.h>
#	include <stdlib.h>
#	include <string.h>
#	include <stdbool.h>
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
#ifdef _WIN32
bool command_run(
	arena_t* arena,
	const char* cmd,
	result_handle_t* result
)
{
	if (!arena || !cmd) return false;

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

	SetHandleInformation(file, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	STARTUPINFOA si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdOutput = file;

	PROCESS_INFORMATION pi = { 0 };

	char cmdline[1024];
	MEMORY_ZERO(cmdline, sizeof(cmdline));

	snprintf(cmdline, sizeof(cmdline), "cmd.exe /C %s", cmd);

	BOOL ok = CreateProcessA(
		NULL,
		cmdline,
		NULL,
		NULL,
		TRUE,
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

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	LARGE_INTEGER size_li;
	if (!GetFileSizeEx(file, &size_li))
	{
		CloseHandle(file);
		return false;
	}

	size_t size = (size_t)size_li.QuadPart;

	char* buffer = arena_alloc(arena, size + 1, 8);
	if (!buffer)
	{
		CloseHandle(file);
		return false;
	}

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
#else
bool command_run(
	arena_t* arena,
	const char* cmd,
	result_handle_t* result
)
{
	if (!arena || !cmd) return false;

	int pipefd[2];
	if (pipe(pipefd) != 0)
		return false;

	pid_t pid = fork();
	if (pid < 0)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return false;
	}

	if (pid == 0)
	{
		close(pipefd[0]);

		dup2(pipefd[1], STDOUT_FILENO);

		close(pipefd[1]);

		execl("/bin/sh", "sh", "-c", cmd, (char*)NULL);

		_exit(127);
	}

	close(pipefd[1]);

	size_t capacity = 4096;
	size_t size = 0;
	char* buffer = arena_alloc(arena, capacity, 8);
	if (!buffer)
	{
		close(pipefd[0]);
		return false;
	}

	while (1)
	{
		if (size == capacity)
		{
			size_t new_cap = capacity * 2;
			char* new_buf = arena_alloc(arena, new_cap, 8);
			if (!new_buf)
			{
				close(pipefd[0]);
				return false;
			}

			memcpy(new_buf, buffer, size);
			buffer = new_buf;
			capacity = new_cap;
		}

		ssize_t n = read(pipefd[0], buffer + size, capacity - size);
		if (n < 0)
		{
			close(pipefd[0]);
			return false;
		}
		if (n == 0)
			break;

		size += (size_t)n;
	}

	close(pipefd[0]);

	waitpid(pid, NULL, 0);

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
#endif
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

/*
 * Copyright 2025 Barracuda Bits
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this work and associated documentation files (the "Work"), to deal in the
 * Work without restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies of the Work,
 * and to permit persons to whom the Work is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Work.
 *
 * THE WORK IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT, OR OTHERWISE, ARISING FROM, OUT OF, OR IN
 * CONNECTION WITH THE WORK OR THE USE OR OTHER DEALINGS IN THE WORK.
 */
