#ifndef __ARGS_H__
#define __ARGS_H__

// EXTERNAL INCLUDES
// INTERNAL INCLUDES
#include "arena.h"

typedef struct config
{
    char* app;
    char* engine;
    char* author;
    char* prefix;
    char* year;
    char* output;
	char* type;
    int release_num;
    int steam_id;
    bool verbose;
} config_t;

config_t* args_parse(
    arena_t* arena,
    size_t argc,
    char** argv
);

#endif // __ARGS_H__
