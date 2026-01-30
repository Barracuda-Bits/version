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

// EXTERNAL INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#ifdef _WIN32
#   include <direct.h>
#   define popen _popen
#   define pclose _pclose
#   define chdir _chdir
#   define PATH_SEP '\\'
#else
#   include <unistd.h>
#   define PATH_SEP '/'
#endif

#define MAX_LEN 256
#define MAX_OUTPUT 8192

typedef struct
{
    char app[MAX_LEN];
    char engine[MAX_LEN];
    char author[MAX_LEN];
    char prefix[MAX_LEN];
    char year[MAX_LEN];
    char output[MAX_LEN];
    int release_num;
    int verbose;
    int steam_id;
} config_t;

static config_t config = {
    .app = "N/A",
    .engine = "N/A",
    .author = "N/A",
    .prefix = "VER",
    .year = "",
    .output = "",
	.release_num = 0,
    .verbose = 0,
    .steam_id = -1
};

static inline void LOG(
    const char* fmt,
    ...
)
{
    if (!config.verbose)
        return;

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

//********************************************************************************************
char* escape_shell_arg(const char* arg)
{
    if (arg == NULL)
        return "";

    size_t len = strlen(arg);
    // Worst case: every character is a single quote needing 4 extra bytes + 2 for outer quotes
    size_t max_len = len * 4 + 3;
    char* escaped = malloc(max_len);

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
//********************************************************************************************
void trim_newline(char* str)
{
    size_t len = strlen(str);
    if (len && str[len - 1] == '\n')
        str[len - 1] = '\0';
}
//********************************************************************************************
int run_command(
    const char* cmd,
    char* out,
    size_t max_len
)
{
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return -1;

    char* p = out;
    size_t total = 0;
    while (fgets(p, max_len - total, pipe))
    {
        size_t len = strlen(p);
        total += len;
        p += len;
        if (total >= max_len)
            break;
    }
    pclose(pipe);
    trim_newline(out);
    return 0;
}
//********************************************************************************************
int run_command_count_lines(
    const char* cmd
)
{
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return -1;
    int lines = 0;
    char buf[128];
    while (fgets(buf, sizeof(buf), pipe))
        lines++;
    pclose(pipe);
    return lines;
}
//********************************************************************************************
void generate_version_header(void)
{
    char last_tag_commit[MAX_LEN] = "";
    char branch[MAX_LEN] = "";
    char commit[MAX_LEN] = "";
    char tag[MAX_LEN] = "dev";
    char git_time[MAX_LEN] = "0";

    char current_branch[MAX_LEN] = "";
    run_command(
        "git rev-parse --abbrev-ref HEAD",
        current_branch,
        sizeof(current_branch)
    );

	// switch to main branch if not already on it
    if (strcmp(current_branch, "main") != 0)
    {
        LOG("Switching to main branch...\n");
        if (run_command("git checkout main", NULL, 0) < 0)
        {
            fprintf(stderr, "Error: Could not switch to main branch.\n");
            exit(1);
        }
	}

    if (run_command(
        "git rev-list --tags --max-count=1",
        last_tag_commit,
        sizeof(last_tag_commit)) < 0)
        strcpy(last_tag_commit, "");

    run_command(
        "git describe --tags --abbrev=0",
        tag,
        sizeof(tag)
    );
    run_command(
        "git rev-parse --abbrev-ref HEAD",
        branch,
        sizeof(branch)
    );
    run_command(
        "git rev-parse --short HEAD",
        commit,
        sizeof(commit)
    );
    run_command(
        "git log -1 --format=%ct",
        git_time,
        sizeof(git_time)
    );

    // Versions
    int major = run_command_count_lines("git tag");
    char rev_cmd[MAX_LEN];

    if (strlen(last_tag_commit) > 0)
    {
        char* input = escape_shell_arg(last_tag_commit);
        if (!input || strlen(input) == 0)
        {
            fprintf(stderr, "Error: Could not escape last tag commit.\n");
            exit(1);
		}
        snprintf(rev_cmd, sizeof(rev_cmd), "git rev-list %s..HEAD", input);
        free(input);
    }
    else
    {
        // Count all commits (initial version)
        snprintf(rev_cmd, sizeof(rev_cmd), "git rev-list HEAD");
    }
	int release = config.release_num;
    int minor = run_command_count_lines(rev_cmd);
    int unstaged = run_command_count_lines("git status --porcelain") > 0;
    int patch_branch_exists = run_command_count_lines("git show-ref --verify refs/heads/patch") > 0;
    int patch = run_command_count_lines("git rev-list patch") +
        unstaged - (patch_branch_exists * minor);

    time_t now = time(NULL);
    struct tm* now_tm = gmtime(&now);

    char build_date[32];
    char build_time[32];
    strftime(
        build_date,
        sizeof(build_date),
        "%Y-%m-%d",
        now_tm
    );
    strftime(
        build_time,
        sizeof(build_time),
        "%H:%M:%S",
        now_tm
    );

    char git_date_fmt[32] = "N/A";
    char git_time_fmt[32] = "N/A";
    if (atoi(git_time) > 0)
    {
        time_t git_ts = (time_t)atoi(git_time);
        struct tm* git_tm = gmtime(&git_ts);
        strftime(
            git_date_fmt,
            sizeof(git_date_fmt),
            "%Y-%m-%d",
            git_tm
        );
        strftime(
            git_time_fmt,
            sizeof(git_time_fmt),
            "%H:%M:%S",
            git_tm
        );
    }

    // Copyright
    char cpy[64];
    int cur_year = now_tm->tm_year + 1900;
    const char* start_year = strlen(config.year) ? config.year : "2025";
    snprintf(
        cpy,
        sizeof(cpy),
        "\u00A9 %s - %d",
        start_year,
        cur_year
    );

    // Output path
    char output_path[MAX_LEN * 2];
    snprintf(
        output_path,
        sizeof(output_path),
        "%sversion.h",
        config.output
    );

    FILE* out = fopen(output_path, "w");
    if (!out)
    {
        fprintf(stderr, "Error: Could not write to %s\n", output_path);
        exit(1);
    }

    LOG("/*\n"
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
        "#define %s_GIT_VERSION \"%d.%d.%d.%d\"\n"
        "#define %s_GIT_VERSION_RELEASE %d\n"
        "#define %s_GIT_VERSION_MAJOR %d\n"
        "#define %s_GIT_VERSION_MINOR %d\n"
        "#define %s_GIT_VERSION_PATCH %d\n"
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
        config.prefix, config.app,
        config.prefix, config.engine,
        config.prefix, config.author,
        config.prefix, cpy,
        config.prefix, tag,
        config.prefix, release, major, minor, patch,
        config.prefix, release,
        config.prefix, major,
        config.prefix, minor,
        config.prefix, patch,
        config.prefix, branch,
        config.prefix, commit,
        config.prefix, git_date_fmt,
        config.prefix, git_time_fmt,
        config.prefix, build_date,
        config.prefix, build_time,
        config.prefix,
#ifdef _DEBUG
        "Debug",
#else
        "Release",
#endif
        config.prefix, (patch > 0),
        config.prefix, config.steam_id
    );

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
        "#define %s_GIT_VERSION \"%d.%d.%d.%d\"\n"
        "#define %s_GIT_VERSION_RELEASE %d\n"
        "// bumped once for each associated tag\n"
        "#define %s_GIT_VERSION_MAJOR %d\n"
        "// bumped for each commit on main branch\n"
        "// minus what was bumped because of PATCH\n"
        "#define %s_GIT_VERSION_MINOR %d\n"
        "// bumped once for uncommited changes\n"
        "// and for each commit on PATCH branch\n"
        "#define %s_GIT_VERSION_PATCH %d\n"
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
        config.prefix, config.app,
        config.prefix, config.engine,
        config.prefix, config.author,
        config.prefix, cpy,
        config.prefix, tag,
        config.prefix, release, major, minor, patch,
        config.prefix, release,
        config.prefix, major,
        config.prefix, minor,
        config.prefix, patch,
        config.prefix, branch,
        config.prefix, commit,
        config.prefix, git_date_fmt,
        config.prefix, git_time_fmt,
        config.prefix, build_date,
        config.prefix, build_time,
        config.prefix,
#ifdef _DEBUG
        "Debug",
#else
        "Release",
#endif
        config.prefix, (patch > 0),
        config.prefix, config.steam_id
    );

    fclose(out);
    LOG("Version header written to: %s\n", output_path);

	// revert to original branch if we switched
    if (strcmp(current_branch, "main") != 0)
    {
        LOG("Reverting back to original branch: %s\n", current_branch);
        if (run_command("git checkout -", NULL, 0) < 0)
        {
            fprintf(stderr, "Error: Could not revert to original branch.\n");
            exit(1);
        }
	}
}
//********************************************************************************************
void parse_args(
    int argc,
    char** argv
)
{
    for (int i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-o") && i + 1 < argc)
        {
            strncpy(
                config.output,
                argv[++i],
                MAX_LEN
            );
            config.output[MAX_LEN - 1] = '\0';
        }
        else if (!strcmp(argv[i], "-n") && i + 1 < argc)
        {
            strncpy(
                config.app,
                argv[++i],
                MAX_LEN
            );

            if (config.app[0] >= 'a' && config.app[0] <= 'z')
            {
                config.app[0] -= ('a' - 'A');
            }

            config.app[MAX_LEN - 1] = '\0';
        }
        else if (!strcmp(argv[i], "-p") && i + 1 < argc)
        {
            strncpy(
                config.prefix,
                argv[++i],
                MAX_LEN
            );
            config.prefix[MAX_LEN - 1] = '\0';
        }
        else if (!strcmp(argv[i], "-e") && i + 1 < argc)
        {
            strncpy(
                config.engine,
                argv[++i],
                MAX_LEN
            );
            config.engine[MAX_LEN - 1] = '\0';
        }
        else if (!strcmp(argv[i], "-a") && i + 1 < argc)
        {
            strncpy(
                config.author,
                argv[++i],
                MAX_LEN
            );
            config.author[MAX_LEN - 1] = '\0';
        }
        else if (!strcmp(argv[i], "-s") && i + 1 < argc)
        {
            strncpy(
                config.year,
                argv[++i],
                MAX_LEN
            );
            config.year[MAX_LEN - 1] = '\0';
        }
        else if (!strcmp(argv[i], "-cwd") && i + 1 < argc)
        {
            chdir(argv[++i]);
        }
        else if (!strcmp(argv[i], "--rn"))
        {
            config.release_num = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "-steamid") && i + 1 < argc)
        {
            config.steam_id = atoi(argv[++i]);
        }
        else if (!strcmp(argv[i], "--verbose"))
        {
            config.verbose = 1;
        }
        else if (!strcmp(argv[i], "-h"))
        {
            printf(
                "Usage:\nversion"
                " -o <output dir>"
                " -n <app name>"
                " -p <prefix>"
                " -e <engine>"
                " -a <author>"
                " -s <start year>"
                " -steamid <steam id>"
                " [--verbose]"
                " [-cwd <dir>]\n"
            );
            exit(0);
        }
    }
}
//********************************************************************************************
int main(
    int argc,
    char** argv
)
{
    parse_args(argc, argv);
    generate_version_header();
    return 0;
}
//********************************************************************************************
