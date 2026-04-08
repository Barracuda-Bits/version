// EXTERNAL INCLUDES
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
// INTERNAL INCLUDES
#include "args.h"
#include "vmem.h"
#include "arena.h"

//********************************************************************************************
bool verbose = false;
//********************************************************************************************
static char* arena_strdup(arena_t* arena, const char* src)
{
    if (!src) return NULL;

    size_t len = strlen(src) + 1;
    char* dst = (char*)arena_alloc(arena, len, 8);
    if (!dst) return NULL;

    memcpy(dst, src, len);
    return dst;
}
//********************************************************************************************
static bool args_has_next(size_t i, size_t argc)
{
    return (i + 1) < argc;
}
//********************************************************************************************
static bool args_handle_int(int* out, const char* value)
{
    if (!value) return 0;
    *out = atoi(value);
    return true;
}
//********************************************************************************************
static void args_handle_string(
    arena_t* arena,
    char** target,
    const char* value
)
{
    *target = arena_strdup(arena, value);
}
//********************************************************************************************
static void args_handle_app_option(
    arena_t* arena,
    config_t* config,
    const char* value
)
{
    config->app = arena_strdup(arena, value);

    if (config->app && config->app[0] >= 'a' && config->app[0] <= 'z')
    {
        config->app[0] -= ('a' - 'A');
    }
}
//********************************************************************************************
config_t* args_parse(
    arena_t* arena,
    size_t argc,
    char** argv
)
{
	config_t* config = arena_alloc(arena, sizeof(config_t), 8);
	MEMORY_ZERO(config, sizeof(config_t));

	config->app = "N/A";
	config->author = "N/A";
	config->engine = "N/A";
	config->type = "N/A";
	config->prefix = "VER";
	config->output = "";
    config->year = "";

    if (!config)
    {
        fprintf(stderr, "Error: Could not allocate memory for config.\n");
        exit(1);
	}

    for (int i = 1; i < argc; ++i)
    {
        const char* arg = argv[i];

        if (!strcmp(arg, "-o") && args_has_next(i, argc))
        {
            args_handle_string(arena, &config->output, argv[++i]);
        }
        else if (!strcmp(arg, "-n") && args_has_next(i, argc))
        {
            args_handle_app_option(arena, config, argv[++i]);
        }
        else if (!strcmp(arg, "-p") && args_has_next(i, argc))
        {
            args_handle_string(arena, &config->prefix, argv[++i]);
        }
        else if (!strcmp(arg, "-e") && args_has_next(i, argc))
        {
            args_handle_string(arena, &config->engine, argv[++i]);
        }
        else if (!strcmp(arg, "-a") && args_has_next(i, argc))
        {
            args_handle_string(arena, &config->author, argv[++i]);
        }
        else if (!strcmp(arg, "-s") && args_has_next(i, argc))
        {
            args_handle_string(arena, &config->year, argv[++i]);
        }
        else if (!strcmp(arg, "-t") && args_has_next(i, argc))
        {
            args_handle_string(arena, &config->type, argv[++i]);
        }
        else if (!strcmp(arg, "-cwd") && args_has_next(i, argc))
        {
            chdir(argv[++i]);
        }
        else if (!strcmp(arg, "--rn") && args_has_next(i, argc))
        {
            args_handle_int(&config->release_num, argv[++i]);
        }
        else if (!strcmp(arg, "-steamid") && args_has_next(i, argc))
        {
            args_handle_int(&config->steam_id, argv[++i]);
        }
        else if (!strcmp(arg, "--verbose"))
        {
            config->verbose = true;
			verbose = true;
        }
        else if (!strcmp(arg, "-h"))
        {
            printf(
                "Usage:\nversion"
                " -o <output dir>"
                " -n <app name>"
                " -p <prefix>"
                " -e <engine>"
                " -a <author>"
                " -s <start year>"
                " -t <Debug/Release>"
                " -steamid <steam id>"
                " [--verbose]"
                " [-cwd <dir>]\n"
            );
            exit(0);
        }
    }

	return config;
}
//********************************************************************************************

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
