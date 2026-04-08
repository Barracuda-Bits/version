// EXTERNAL INCLUDES
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
// INTERNAL INCLUDES
#include "commands.h"
#include "logging.h"
#include "args.h"

//********************************************************************************************
void version_header_generate(config_t* config, arena_t* arena)
{
    result_handle_t result_last_tag_commit = NULL;
    result_handle_t result_tag = NULL;
    result_handle_t result_branch = NULL;
    result_handle_t result_commit = NULL;
    result_handle_t result_commit_time = NULL;
    result_handle_t result_tag_list = NULL;
    result_handle_t result_commit_list = NULL;
    result_handle_t result_unstaged = NULL;
    result_handle_t result_merge_base = NULL;
    result_handle_t result_patch_commits = NULL;

    // Get current branch
    if (!command_run(arena, "git rev-parse --abbrev-ref HEAD", &result_branch))
    {
        LOG("Error: Could not determine current branch.");
        exit(1);
    }
    const char* current_branch = command_line_get(result_branch);

    // Last tag and its commit
    if (!command_run(arena, "git rev-list --tags --max-count=1", &result_last_tag_commit))
    {
        LOG("Error: Could not get last tag commit.");
        exit(1);
    }

    if (!command_run(arena, "git describe --tags --abbrev=0", &result_tag))
    {
        LOG("Error: Could not get last tag.");
        exit(1);
    }

    // Commit hash and timestamp
    if (!command_run(arena, "git rev-parse --short HEAD", &result_commit))
    {
        LOG("Error: Could not get current commit.");
        exit(1);
    }
    if (!command_run(arena, "git log -1 --format=%ct", &result_commit_time))
    {
        LOG("Error: Could not get commit timestamp.");
        exit(1);
    }

    // Tag list (for major calculation)
    if (!command_run(arena, "git tag --sort=creatordate", &result_tag_list))
    {
        LOG("Error: Could not get tag list.");
        exit(1);
    }

    // Unstaged changes
    if (!command_run(arena, "git status --porcelain", &result_unstaged))
    {
        LOG("Error: Could not get git status.");
        exit(1);
    }
    bool unstaged = command_count_lines(result_unstaged) > 0;

    // --- Version calculation ---

    // Major = number of tags (chronological)
    size_t major = command_count_lines(result_tag_list);

    // Minor = commits since last tag on main branch
    char rev_cmd[1024];
    const char* last_tag_commit = command_line_get(result_last_tag_commit);
    if (last_tag_commit && strlen(last_tag_commit) > 0)
    {
        char* escaped_tag = escape_shell_arg(arena, last_tag_commit);
        snprintf(rev_cmd, sizeof(rev_cmd), "git rev-list %s..main", escaped_tag);
    }
    else
    {
        snprintf(rev_cmd, sizeof(rev_cmd), "git rev-list main");
    }

    if (!command_run(arena, rev_cmd, &result_commit_list))
    {
        LOG("Error: Could not get commits since last tag.");
        exit(1);
    }
    size_t minor = command_count_lines(result_commit_list);

    // Patch = commits since merge-base with main + dirty state
    if (!command_run(arena, "git merge-base main HEAD", &result_merge_base))
    {
        LOG("Error: Could not determine merge base.");
        exit(1);
    }
    const char* merge_base = command_line_get(result_merge_base);
    if (!merge_base || strlen(merge_base) == 0)
    {
        LOG("Error: Invalid merge base.");
        exit(1);
    }
    snprintf(rev_cmd, sizeof(rev_cmd), "git rev-list %s..HEAD", merge_base);
    if (!command_run(arena, rev_cmd, &result_patch_commits))
    {
        LOG("Error: Could not get patch commits.");
        exit(1);
    }
    size_t patch = command_count_lines(result_patch_commits) + (unstaged ? 1 : 0);

    // Release version
    size_t release = config->release_num;

    // --- Timestamps ---
    time_t now = time(NULL);
    struct tm* now_tm = gmtime(&now);
    char build_date[32], build_time[32];
    strftime(build_date, sizeof(build_date), "%Y-%m-%d", now_tm);
    strftime(build_time, sizeof(build_time), "%H:%M:%S", now_tm);

    char git_date_fmt[32] = "N/A", git_time_fmt[32] = "N/A";
    const char* cmt_time = command_line_get(result_commit_time);
    if (cmt_time && atoi(cmt_time) > 0)
    {
        time_t git_ts = (time_t)atoi(cmt_time);
        struct tm* git_tm = gmtime(&git_ts);
        strftime(git_date_fmt, sizeof(git_date_fmt), "%Y-%m-%d", git_tm);
        strftime(git_time_fmt, sizeof(git_time_fmt), "%H:%M:%S", git_tm);
    }

    // Copyright
    int cur_year = now_tm->tm_year + 1900;
    // Use config->year if provided, otherwise current year
    const char* start_year = (config->year && strlen(config->year) > 0) ? config->year : NULL;

    // Format copyright
    char cpy[64];
    if (start_year)
    {
        snprintf(cpy, sizeof(cpy), "\u00A9 %s - %d", start_year, cur_year);
    }
    else
    {
        snprintf(cpy, sizeof(cpy), "\u00A9 %d", cur_year);
    }

    // Output path
    char output_path[1024];
    snprintf(output_path, sizeof(output_path), "%sversion.h", config->output);

    FILE* out = fopen(output_path, "w");
    if (!out)
    {
        LOG("Error: Could not write to % s", output_path);
        exit(1);
    }

    const char* tag = command_line_get(result_tag);

    fprintf(out,
        "/*\n"
        " *************************************\n"
        " * DO NOT MODIFY THIS FILE.\t\t\t *\n"
        " * Auto-generated version header.\t *\n"
        " * Generated on %sT%sZ *\n"
        " *************************************\n"
        "*/\n\n"
        "#ifndef VERSION_H\n"
        "#define VERSION_H\n\n"
        "#define %s_APPLICATION_NAME \"%s\"\n"
        "#define %s_ENGINE_NAME \"%s\"\n"
        "#define %s_AUTHOR \"%s\"\n"
        "#define %s_COPYRIGHT \"%s\"\n\n"
        "#define %s_GIT_TAG \"%s\"\n"
        "#define %s_GIT_VERSION \"%" PRIuPTR ".%" PRIuPTR ".%" PRIuPTR ".%" PRIuPTR "\"\n"
        "#define %s_GIT_VERSION_RELEASE %" PRIuPTR "\n"
        "#define %s_GIT_VERSION_MAJOR %" PRIuPTR "\n"
        "#define %s_GIT_VERSION_MINOR %" PRIuPTR "\n"
        "#define %s_GIT_VERSION_PATCH %" PRIuPTR "\n"
        "#define %s_GIT_BRANCH \"%s\"\n"
        "#define %s_GIT_COMMIT \"%s\"\n"
        "#define %s_GIT_DATE \"%s\"\n"
        "#define %s_GIT_TIME \"%s\"\n\n"
        "#define %s_BUILD_DATE \"%s\"\n"
        "#define %s_BUILD_TIME \"%s\"\n"
        "#define %s_BUILD_TYPE \"%s\"\n\n"
        "#define %s_IS_HOTFIX %d\n\n"
        "#define %s_STEAM_APPID %d\n\n"
        "#endif // VERSION_H\n",
        build_date, build_time,
        config->prefix, config->app,
        config->prefix, config->engine,
        config->prefix, config->author,
        config->prefix, cpy,
        config->prefix, strlen(tag) > 0 ? tag : "dev",
        config->prefix, release, major, minor, patch,
        config->prefix, release,
        config->prefix, major,
        config->prefix, minor,
        config->prefix, patch,
        config->prefix, current_branch,
        config->prefix, command_line_get(result_commit),
        config->prefix, git_date_fmt,
        config->prefix, git_time_fmt,
        config->prefix, build_date,
        config->prefix, build_time,
        config->prefix, config->type,
        config->prefix, (patch > 0llu),
        config->prefix, config->steam_id
    );

    fclose(out);
    LOG("Version header written to: %s", output_path);
}
//********************************************************************************************
int main(
    int argc,
    char** argv
)
{
	arena_t* arena = arena_create(MiB(2));
    config_t* config = args_parse(arena, argc, argv);
    version_header_generate(config, arena);
	arena_destroy(arena);
    return 0;
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
