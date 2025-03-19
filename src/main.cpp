// EXTERNAL INCLUDES
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// INTERNAL INCLUDES

#define GEN_PREFIX "BE"
#define STRINGIFY(x) #x
#define MAX_PARAM_SIZE 128
#define MAX_DATE_SIZE 32
#define MAX_CPYN_SIZE 64
#define MAX_OUTPUT_SIZE 4096

//********************************************************************************************
char* GetCommandOutput(const char* cmd, char* result, int resultSize)
{
    char buffer[128];
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return result;

    char tempResult[4096] = "";
    while (fgets(buffer, sizeof buffer, pipe) != NULL)
    {
        if (strlen(tempResult) + strlen(buffer) < sizeof(tempResult))
            strcat(tempResult, buffer);
        else
            break;
    }

    _pclose(pipe);

    if (tempResult[0] != '\0')
        strncpy(result, tempResult, resultSize - 1);

    return result;
}
//********************************************************************************************
int main(int argc, char* argv[])
{
    printf("Creating version...\n");

    char AuthorName[MAX_PARAM_SIZE] = "N/A";
    char EngineName[MAX_PARAM_SIZE] = "N/A";
	char StartYear[MAX_PARAM_SIZE] = "";
    char OutputPath[_MAX_PATH] = "";

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-o") == 0 && (i + 1) < argc)
            strcpy(OutputPath, argv[i + 1]);
        else if (strcmp(argv[i], "-e") == 0 && (i + 1) < argc)
            strcpy(EngineName, argv[i + 1]);
        else if (strcmp(argv[i], "-a") == 0 && (i + 1) < argc)
            strcpy(AuthorName, argv[i + 1]);
		else if (strcmp(argv[i], "-s") == 0 && (i + 1) < argc)
			strcpy(StartYear, argv[i + 1]);
		else if (strcmp(argv[i], "-h") == 0)
		{
			printf("Usage: version -o <output path> -e <engine name> -a <author name> -s <start year>\n");
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

	char GitTag[MAX_PARAM_SIZE] = "dev";
	char GitBranch[MAX_PARAM_SIZE] = "N/A";
	char GitCommit[MAX_PARAM_SIZE] = "N/A";
	char GitDate[MAX_PARAM_SIZE] = "0";

	GetCommandOutput("git describe --tags --abbrev=0", GitTag, MAX_PARAM_SIZE);
	GetCommandOutput("git rev-parse --abbrev-ref HEAD", GitBranch, MAX_PARAM_SIZE);
	GetCommandOutput("git rev-parse --short HEAD", GitCommit, MAX_PARAM_SIZE);
	GetCommandOutput("git log -1 --format=%ct", GitDate, MAX_PARAM_SIZE);

	// remove newline characters
	if (GitTag) GitTag[strcspn(GitTag, "\n")] = 0;
	if (GitBranch) GitBranch[strcspn(GitBranch, "\n")] = 0;
	if (GitCommit) GitCommit[strcspn(GitCommit, "\n")] = 0;
	if (GitDate) GitDate[strcspn(GitDate, "\n")] = 0;

    time_t NowTS = time(0);
    struct tm* NowDateTime = gmtime(&NowTS);

	if (StartYear[0] == '\0')
	{
		printf("Error: Start year is not set, taking this year.\n");
		snprintf(
            StartYear,
            sizeof(StartYear),
            "%d",
            NowDateTime->tm_year + 1900
        );
	}

    char CopyNotice[MAX_CPYN_SIZE];
    snprintf(
        CopyNotice,
        sizeof(CopyNotice),
        "\xA9 %s - %d",
        StartYear,
        NowDateTime->tm_year + 1900
    );

    char NowDateBuffer[MAX_DATE_SIZE];
    char NowTimeBuffer[MAX_DATE_SIZE];
    strftime(NowDateBuffer, MAX_DATE_SIZE, "%Y-%m-%d", NowDateTime);
    strftime(NowTimeBuffer, MAX_DATE_SIZE, "%H:%M:%S", NowDateTime);

    printf("--------------------\n");
	printf("Parameters collected\n");
    printf("--------------------\n");

    printf("Author: %s\n", AuthorName);
    printf("Copy Notice: %s\n", CopyNotice);
    printf("Engine: %s\n", EngineName);
    printf("Git Version: %s\n", GitTag);
    printf("Git Branch: %s\n", GitBranch);
    printf("Git Commit: %s\n", GitCommit);

    char GitDateBuffer[MAX_DATE_SIZE] = "N/A";
	char GitTimeBuffer[MAX_DATE_SIZE] = "N/A";
    if (GitDate)
    {
        time_t GitTS = atoi(GitDate);
        struct tm* GitDateTime = gmtime(&GitTS);

        strftime(GitDateBuffer, MAX_DATE_SIZE, "%Y-%m-%d", GitDateTime);
        strftime(GitTimeBuffer, MAX_DATE_SIZE, "%H:%M:%S", GitDateTime);

        printf("Git Date (UTC): %s", asctime(GitDateTime));
        printf("Build Date (UTC): %s", asctime(NowDateTime));
    }

    char versionFile[MAX_OUTPUT_SIZE];
    snprintf(
        versionFile,
        sizeof(versionFile),
        "#ifndef __VERSION_H__\n"
		"#define __VERSION_H__\n\n"
        "#define %s_ENGINE_NAME \"%s\"\n"
        "#define %s_CPY_NOTE \"%s\"\n"
        "#define %s_AUTHOR \"%s\"\n\n"
        "#define %s_GIT_TAG \"%s\"\n"
        "#define %s_GIT_BRANCH \"%s\"\n"
        "#define %s_GIT_COMMIT \"%s\"\n"
        "#define %s_GIT_DATE \"%s\"\n"
        "#define %s_GIT_TIME \"%s\"\n\n"
        "#define %s_BUILD_DATE \"%s\"\n"
        "#define %s_BUILD_TIME \"%s\"\n\n"
		"#endif\n",
        GEN_PREFIX, EngineName,
        GEN_PREFIX, CopyNotice,
        GEN_PREFIX, AuthorName,
        GEN_PREFIX, GitTag,
        GEN_PREFIX, GitBranch,
        GEN_PREFIX, GitCommit,
        GEN_PREFIX, GitDateBuffer,
        GEN_PREFIX, GitTimeBuffer,
        GEN_PREFIX, NowDateBuffer,
        GEN_PREFIX, NowTimeBuffer
    );

    char versionFilePath[_MAX_PATH];
    snprintf(
        versionFilePath,
        sizeof(versionFilePath),
        "%sversion.h",
        OutputPath
    );

    FILE* file = fopen(versionFilePath, "w");
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
