#ifndef __COMMANDS_H__
#define __COMMANDS_H__

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "vmem.h"
#include "arena.h"

typedef struct result_handle* result_handle_t;

bool command_run(
	arena_t* arena,
	const char* cmd,
	result_handle_t* result
);

size_t command_count_lines(
	result_handle_t result
);

char* command_line_get(
	result_handle_t result
);

char* escape_shell_arg(
	arena_t* arena,
	const char* arg
);

#endif // __COMMANDS_H__
