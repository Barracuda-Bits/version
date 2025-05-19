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

//********************************************************************************************
char* GetCommandOutput(const char* cmd, char* result, int resultSize)
{
    char buffer[128];
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return result;

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
    printf("Creating version...\n");

    char* n_a = "N/A";
	size_t n_a_len = strlen(n_a);

    char AuthorName[MAX_PARAM_SIZE];
    memcpy_s(AuthorName, MAX_PARAM_SIZE, n_a, n_a_len);
    char PrefixName[MAX_PARAM_SIZE];
    memcpy_s(PrefixName, MAX_PARAM_SIZE, GEN_PREFIX, strlen(GEN_PREFIX));
    char EngineName[MAX_PARAM_SIZE];
    memcpy_s(EngineName, MAX_PARAM_SIZE, n_a, n_a_len);
	char StartYear[MAX_PARAM_SIZE];
	memset(StartYear, 0, MAX_PARAM_SIZE);
    char OutputPath[_MAX_PATH];
	memset(OutputPath, 0, _MAX_PATH);

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-o") == 0 && (i + 1) < argc)
            strcpy_s(OutputPath, _MAX_PATH, argv[i + 1]);
        else if (strcmp(argv[i], "-p") == 0 && (i + 1) < argc)
            strcpy_s(PrefixName, MAX_PARAM_SIZE, argv[i + 1]);
        else if (strcmp(argv[i], "-e") == 0 && (i + 1) < argc)
            strcpy_s(EngineName, MAX_PARAM_SIZE, argv[i + 1]);
        else if (strcmp(argv[i], "-a") == 0 && (i + 1) < argc)
            strcpy_s(AuthorName, MAX_PARAM_SIZE, argv[i + 1]);
		else if (strcmp(argv[i], "-s") == 0 && (i + 1) < argc)
            strcpy_s(StartYear, MAX_PARAM_SIZE, argv[i + 1]);
		else if (strcmp(argv[i], "-h") == 0)
		{
			printf("Usage: version -o <output path> -p <prefix> -e <engine name> -a <author name> -s <start year>\n");
			return 0;
		}
    }

    char Fetch[MAX_PARAM_SIZE];
	GetCommandOutput("git fetch", Fetch, MAX_PARAM_SIZE);
    if (strstr(Fetch, "fatal"))
    {
        printf("Error: Git is not installed or not in PATH\n");
        return 1;
    }

	// integer based git parsing
	size_t VersionMajor = 0;
    size_t VersionMinor = 0;
    size_t VersionPatch = 0;
    VersionMajor = GetCommandLineCount("git tag");
    VersionMinor = GetCommandLineCount("git rev-list main") + (GetCommandLineCount("git status --porcelain") > 0);
    VersionPatch = GetCommandLineCount("git rev-list patch");

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
		printf("Error: Start year is not set, taking this year.\n");
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

    printf("--------------------\n");
	printf("Parameters collected\n");
    printf("--------------------\n");

    printf("Author: %s\n", AuthorName);
    printf("Copy Notice: %s\n", CopyNotice);
    printf("Engine: %s\n", EngineName);
    printf("Git Version: %s\n", GitTag);
	printf("Git Version (int): %d.%d.%d\n",
        VersionMajor,
        VersionMinor,
        VersionPatch
    );
    printf("Git Branch: %s\n", GitBranch);
    printf("Git Commit: %s\n", GitCommit);

    char GitDateBuffer[MAX_DATE_SIZE] = "N/A";
	char GitTimeBuffer[MAX_DATE_SIZE] = "N/A";

    time_t GitTS = atoi(GitDate);
    struct tm GitDateTime;
    gmtime_s(&GitDateTime, &GitTS);

    strftime(GitDateBuffer, MAX_DATE_SIZE, "%Y-%m-%d", &GitDateTime);
    strftime(GitTimeBuffer, MAX_DATE_SIZE, "%H:%M:%S", &GitDateTime);

    printf("Git Date (UTC): %s", GitDateBuffer);
    printf("Build Date (UTC): %s", GitTimeBuffer);

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
        "#define %s_ENGINE_NAME \"%s\"\n"
        "#define %s_CPY_NOTE \"%s\"\n"
        "#define %s_AUTHOR \"%s\"\n\n"
        "#define %s_GIT_TAG \"%s\"\n"
        "#define %s_GIT_VERSION \"%d.%d.%d\"\n"
        "#define %s_GIT_BRANCH \"%s\"\n"
        "#define %s_GIT_COMMIT \"%s\"\n"
        "#define %s_GIT_DATE \"%s\"\n"
        "#define %s_GIT_TIME \"%s\"\n\n"
        "#define %s_BUILD_DATE \"%s\"\n"
        "#define %s_BUILD_TIME \"%s\"\n\n"
		"#define %s_IS_HOTFIX %d\n\n"
		"#endif // VERSION_H\n",
		NowDateBuffer, NowTimeBuffer,
        PrefixName, EngineName,
        PrefixName, CopyNotice,
        PrefixName, AuthorName,
        PrefixName, GitTag,
        PrefixName, VersionMajor, VersionMinor, VersionPatch,
        PrefixName, GitBranch,
        PrefixName, GitCommit,
        PrefixName, GitDateBuffer,
        PrefixName, GitTimeBuffer,
        PrefixName, NowDateBuffer,
        PrefixName, NowTimeBuffer,
		PrefixName, (VersionPatch > 0)
    );

    char versionFilePath[_MAX_PATH];
    _snprintf_s(
        versionFilePath,
        sizeof(versionFilePath),
		sizeof(versionFilePath) - 1,
        "%sversion.h",
        OutputPath
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
