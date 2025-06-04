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
// INTERNAL INCLUDES

#define GEN_PREFIX "VER"
#define MAX_PARAM_SIZE 128
#define MAX_DATE_SIZE 32
#define MAX_CPYN_SIZE 64
#define MAX_OUTPUT_SIZE 4096

#if defined(_MSC_VER)
#   define popen _popen
#   define pclose _pclose
#endif

#define MAX_PATH 260

#define bool int
#define true 1
#define false 0

static bool quiet = true;

#define print_console(...)          \
do                                  \
{                                   \
    if (!quiet)                     \
    {                               \
        printf(__VA_ARGS__);        \
    }                               \
} while (0)

//********************************************************************************************
char* GetCommandOutput(const char* cmd, char* result, int resultSize)
{
    char buffer[128];
    FILE* pipe = popen(cmd, "r");

    if (!pipe)
    {
        print_console("Error: Could not open pipe for command '%s'\n", cmd);
        return NULL;
	}

    char tempResult[4096];
	memset(tempResult, 0, sizeof(tempResult));
    while (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        if (strlen(tempResult) + strlen(buffer) < sizeof(tempResult))
            strcat_s(tempResult, 4096, buffer);
        else
            break;
    }

    pclose(pipe);

    if (tempResult[0] != '\0')
		strncpy_s(result, (size_t)resultSize, tempResult, (size_t)resultSize - 1);

    return result;
}
//********************************************************************************************
size_t GetCommandLineCount(const char* cmd)
{
    size_t count = 0;

    char buffer[256];
    FILE* fp = popen(cmd, "r");

    if (!fp)
    {
        print_console("Error: Could not open pipe for command '%s'\n", cmd);
        return 0;
	}

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        count++;
    }

    pclose(fp);

    return count;
}
//********************************************************************************************
int main(int argc, char* argv[])
{
    char* n_a = "N/A";
	size_t n_a_len = strlen(n_a);

    char ApplicationName[MAX_PARAM_SIZE];
    memcpy_s(ApplicationName, MAX_PARAM_SIZE, n_a, n_a_len);
    char AuthorName[MAX_PARAM_SIZE];
    memcpy_s(AuthorName, MAX_PARAM_SIZE, n_a, n_a_len);
    char PrefixName[MAX_PARAM_SIZE];
    memcpy_s(PrefixName, MAX_PARAM_SIZE, GEN_PREFIX, strlen(GEN_PREFIX));
    char EngineName[MAX_PARAM_SIZE];
    memcpy_s(EngineName, MAX_PARAM_SIZE, n_a, n_a_len);
	char StartYear[MAX_PARAM_SIZE];
	memset(StartYear, 0, MAX_PARAM_SIZE);
    char OutputPath[MAX_PATH];
	memset(OutputPath, 0, MAX_PATH);

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-o") == 0 && (i + 1) < argc)
            strcpy_s(OutputPath, MAX_PATH, argv[i + 1]);
		else if (strcmp(argv[i], "-n") == 0 && (i + 1) < argc)
			strcpy_s(ApplicationName, MAX_PARAM_SIZE, argv[i + 1]);
        else if (strcmp(argv[i], "-p") == 0 && (i + 1) < argc)
            strcpy_s(PrefixName, MAX_PARAM_SIZE, argv[i + 1]);
        else if (strcmp(argv[i], "-e") == 0 && (i + 1) < argc)
            strcpy_s(EngineName, MAX_PARAM_SIZE, argv[i + 1]);
        else if (strcmp(argv[i], "-a") == 0 && (i + 1) < argc)
            strcpy_s(AuthorName, MAX_PARAM_SIZE, argv[i + 1]);
		else if (strcmp(argv[i], "-s") == 0 && (i + 1) < argc)
            strcpy_s(StartYear, MAX_PARAM_SIZE, argv[i + 1]);
        else if (strcmp(argv[i], "--verbose") == 0)
        {
            quiet = false;
			print_console("Version tool: Verbose mode enabled.\n");
		}
		else if (strcmp(argv[i], "-h") == 0)
		{
			printf("Usage: version -o <output path> -n <application name> -p <prefix> -e <engine name> -a <author name> -s <start year> --verbose\n");
			return 0;
		}
    }

    print_console("Creating version...\n");

    char versionFilePath[MAX_PATH];
    _snprintf_s(
        versionFilePath,
        sizeof(versionFilePath),
        sizeof(versionFilePath) - 1,
        "%sversion.h",
        OutputPath
    );

	// check if first letter of application name is lowercase and convert to uppercase
	if (ApplicationName[0] >= 'a' && ApplicationName[0] <= 'z')
		ApplicationName[0] = (char)(ApplicationName[0] - ('a' - 'A'));

    char Fetch[MAX_PARAM_SIZE];
	GetCommandOutput("git fetch", Fetch, MAX_PARAM_SIZE);
    if (strstr(Fetch, "fatal"))
    {
        print_console("Error: Git is not installed or not in PATH\n");
        return 1;
    }

	// integer based git parsing
	size_t VersionMajor = 0;
    size_t VersionMinor = 0;
    size_t VersionPatch = 0;

    int UnstagedBump = (GetCommandLineCount("git status --porcelain") > 0);
    int PatchExists = GetCommandLineCount("git show-ref --verify refs/heads/patch");

    // compile major, minor, patch -> subtract version minor from patch, if patch exists
    // because patch will encompass minor commits as well which we don't want
    // uncommited changes will always be bumped on patch even if the branch does not exist
    VersionMajor = GetCommandLineCount("git tag");
    VersionMinor = GetCommandLineCount("git rev-list main");
    VersionPatch = GetCommandLineCount("git rev-list patch") + UnstagedBump - (PatchExists * VersionMinor);

    // String based git parsing
    char GitTag[MAX_PARAM_SIZE] = "dev";
	char GitBranch[MAX_PARAM_SIZE] = "N/A";
	char GitCommit[MAX_PARAM_SIZE] = "N/A";
	char GitDate[MAX_PARAM_SIZE] = "0";
	GetCommandOutput("git describe --tags --abbrev=0", GitTag, MAX_PARAM_SIZE);
	GetCommandOutput("git rev-parse --abbrev-ref HEAD", GitBranch, MAX_PARAM_SIZE);
	GetCommandOutput("git rev-parse --short HEAD", GitCommit, MAX_PARAM_SIZE);
	GetCommandOutput("git log -1 --format=%ct", GitDate, MAX_PARAM_SIZE);
	GitTag[strcspn(GitTag, "\n")] = 0;
	GitBranch[strcspn(GitBranch, "\n")] = 0;
	GitCommit[strcspn(GitCommit, "\n")] = 0;
	GitDate[strcspn(GitDate, "\n")] = 0;

    time_t NowTS = time(0);
    struct tm NowDateTime;
    gmtime_s(&NowDateTime, &NowTS);
	if (StartYear[0] == '\0')
	{
        print_console("Error: Start year is not set, taking this year.\n");
        _snprintf_s(
            StartYear,
            sizeof(StartYear),
            sizeof(StartYear) - 1,
            "%d",
            NowDateTime.tm_year + 1900
        );
	}
    char CopyNotice[MAX_CPYN_SIZE];
    _snprintf_s(
        CopyNotice,
        sizeof(CopyNotice),
		sizeof(CopyNotice) - 1,
        "\xA9 %s - %d",
        StartYear,
        NowDateTime.tm_year + 1900
    );

    char NowDateBuffer[MAX_DATE_SIZE];
    char NowTimeBuffer[MAX_DATE_SIZE];
    strftime(NowDateBuffer, MAX_DATE_SIZE, "%Y-%m-%d", &NowDateTime);
    strftime(NowTimeBuffer, MAX_DATE_SIZE, "%H:%M:%S", &NowDateTime);

    print_console("--------------------\n");
    print_console("Parameters collected\n");
    print_console("--------------------\n");

	print_console("Application Name: %s\n", ApplicationName);
    print_console("Author: %s\n", AuthorName);
    print_console("Copy Notice: %s\n", CopyNotice);
    print_console("Engine: %s\n", EngineName);
    print_console("Git Version: %s\n", GitTag);
	print_console("Git Version: %d.%d.%d\n",
        (int)VersionMajor,
        (int)VersionMinor,
        (int)VersionPatch
    );
    print_console("Git Branch: %s\n", GitBranch);
    print_console("Git Commit: %s\n", GitCommit);

    char GitDateBuffer[MAX_DATE_SIZE] = "N/A";
	char GitTimeBuffer[MAX_DATE_SIZE] = "N/A";

    time_t GitTS = atoi(GitDate);
    struct tm GitDateTime;
    gmtime_s(&GitDateTime, &GitTS);

    strftime(GitDateBuffer, MAX_DATE_SIZE, "%Y-%m-%d", &GitDateTime);
    strftime(GitTimeBuffer, MAX_DATE_SIZE, "%H:%M:%S", &GitDateTime);

    print_console("Git Date (UTC): %sT%sZ\n", GitDateBuffer, GitTimeBuffer);
    print_console("Build Date (UTC): %sT%sZ\n", NowDateBuffer, NowTimeBuffer);

    char versionFile[MAX_OUTPUT_SIZE];
    _snprintf_s(
        versionFile,
        sizeof(versionFile),
		sizeof(versionFile) - 1,
		"// DO NOT MODIFY THIS FILE.\n"
		"// This file is auto-generated by version tool.\n"
		"// Generation DTG: %sT%sZ\n\n"
        "#ifndef VERSION_H\n"
		"#define VERSION_H\n\n"
		"#define %s_APPLICATION_NAME \"%s\"\n"
        "#define %s_ENGINE_NAME \"%s\"\n"
        "#define %s_CPY_NOTE \"%s\"\n"
        "#define %s_AUTHOR \"%s\"\n\n"
        "#define %s_GIT_TAG \"%s\"\n"
        "#define %s_GIT_VERSION \"%d.%d.%d\"\n"
        "#define %s_GIT_VERSION_MAJOR %d\n"
        "#define %s_GIT_VERSION_MINOR %d\n"
        "#define %s_GIT_VERSION_PATCH %d\n"
        "#define %s_GIT_BRANCH \"%s\"\n"
        "#define %s_GIT_COMMIT \"%s\"\n"
        "#define %s_GIT_DATE \"%s\"\n"
        "#define %s_GIT_TIME \"%s\"\n\n"
        "#define %s_BUILD_DATE \"%s\"\n"
        "#define %s_BUILD_TIME \"%s\"\n\n"
		"#define %s_IS_HOTFIX %d\n\n"
		"#endif // VERSION_H\n",
		NowDateBuffer, NowTimeBuffer,
		PrefixName, ApplicationName,
        PrefixName, EngineName,
        PrefixName, CopyNotice,
        PrefixName, AuthorName,
        PrefixName, GitTag,
        PrefixName, (int)VersionMajor, (int)VersionMinor, (int)VersionPatch,
        PrefixName, (int)VersionMajor,
        PrefixName, (int)VersionMinor,
        PrefixName, (int)VersionPatch,
        PrefixName, GitBranch,
        PrefixName, GitCommit,
        PrefixName, GitDateBuffer,
        PrefixName, GitTimeBuffer,
        PrefixName, NowDateBuffer,
        PrefixName, NowTimeBuffer,
		PrefixName, (VersionPatch > 0)
    );

    FILE* file;
    fopen_s(&file, versionFilePath, "w");
    if (file)
    {
        fwrite(
            versionFile,
            sizeof(char),
            strlen(versionFile),
            file
        );
        fclose(file);
    }

    return 0;
}
//********************************************************************************************
