// EXTERNAL INCLUDES
#include <cstdio>
#include <iostream>
#include <string>
#include <array>
// INTERNAL INCLUDES

#define GEN_PREFIX "BE"
#define STRINGIFY(x) #x
#define GEN_START_YEAR STRINGIFY(2023)

std::string GetCommandOutput(const char* cmd)
{
    char buffer[128];
    std::string result = "";
    FILE* pipe = _popen(cmd, "r");
    if (!pipe) return "";
    
    while (fgets(buffer, sizeof buffer, pipe) != NULL)
        result += buffer;
    
    _pclose(pipe);
    return result;
}

int main(int argc, char* argv[])
{
	// Parse command line arguments
	std::string AuthorName = "N/A";
	std::string EngineName = "N/A";

	std::string OutputPath = "";
	
	for (int i = 0; i < argc; ++i)
	{
		// get output path
		if (strcmp(argv[i], "-o") == NULL)
		{
			OutputPath = argv[i + 1];
		}
		// get engine name
		if (strcmp(argv[i], "-e") == NULL)
		{
			EngineName = argv[i + 1];
		}
		// get author name
		if (strcmp(argv[i], "-a") == NULL)
		{
			AuthorName = argv[i + 1];
		}
	}

	// Execute shell command 'git version' and retrieve the output into variable
	std::string Fetch = GetCommandOutput("git fetch");
	if (Fetch.find("fatal") != std::string::npos)
	{
		std::cout << "Error: Git is not installed or not in PATH" << std::endl;
		return 1;
	}

	std::string GitTag = GetCommandOutput("git describe --tags --abbrev=0");
	std::string GitBranch = GetCommandOutput("git rev-parse --abbrev-ref HEAD");
	std::string GitCommit = GetCommandOutput("git rev-parse --short HEAD");
	std::string GitDate = GetCommandOutput("git log -1 --format=%ct");

	// Remove newline characters from the end of the strings
	GitTag.erase(GitTag.find_last_not_of("\n\r") + 1);
	GitBranch.erase(GitBranch.find_last_not_of("\n\r") + 1);
	GitCommit.erase(GitCommit.find_last_not_of("\n\r") + 1);
	GitDate.erase(GitDate.find_last_not_of("\n\r") + 1);
	
	// Handle empty
	if (GitTag.empty() || GitBranch == "dev")
		GitTag = "dev";
	if (GitBranch.empty())
		GitBranch = "N/A";
	if (GitCommit.empty())
		GitCommit = "N/A";
	if (GitDate.empty())
		GitDate = "N/A";

	// Get current date time and print it
	time_t NowTS = time(0);
	tm* NowDateTime = gmtime(&NowTS);
	
	std::string CopyNotice = "\xA9 " GEN_START_YEAR " - ";
	CopyNotice += std::to_string(NowDateTime->tm_year + 1900);

	// Convert time to string
	char NowDateBuffer[32] = { 0 };
	char NowTimeBuffer[32] = { 0 };

	std::strftime(NowDateBuffer, 32, "%Y-%m-%d", NowDateTime);
	std::strftime(NowTimeBuffer, 32, "%H:%M:%S", NowDateTime);
	
	time_t GitTS = std::stoi(GitDate);
	tm* GitDateTime = gmtime(&GitTS);

	char GitDateBuffer[32] = { 0 };
	char GitTimeBuffer[32] = { 0 };

	std::strftime(GitDateBuffer, 32, "%Y-%m-%d", NowDateTime);
	std::strftime(GitTimeBuffer, 32, "%H:%M:%S", NowDateTime);
	
	// Print the version information
	std::cout << "Author: " << AuthorName << std::endl;
	std::cout << "Copy Notice: " << CopyNotice << std::endl;
	std::cout << "Engine: " << EngineName << std::endl;
	std::cout << "Git Version: " << GitTag << std::endl;
	std::cout << "Git Branch: " << GitBranch << std::endl;
	std::cout << "Git Commit: " << GitCommit << std::endl;
	std::cout << "Git Date (UTC): " << asctime(gmtime(&GitTS));
	std::cout << "Build Date (UTC): " << asctime(gmtime(&NowTS));

	// Generate version.h
	std::string versionFile = "#pragma once\n\n";
	versionFile += "#define " GEN_PREFIX "_ENGINE_NAME \"" + EngineName + "\"\n";
	versionFile += "#define " GEN_PREFIX "_CPY_NOTE \"" + CopyNotice + "\"\n";
	versionFile += "#define " GEN_PREFIX "_AUTHOR \"" + AuthorName + "\"\n\n";
	versionFile += "#define " GEN_PREFIX "_GIT_TAG \"" + GitTag + "\"\n";
	versionFile += "#define " GEN_PREFIX "_GIT_BRANCH \"" + GitBranch + "\"\n";
	versionFile += "#define " GEN_PREFIX "_GIT_COMMIT \"" + GitCommit + "\"\n";
	versionFile += "#define " GEN_PREFIX "_GIT_DATE \"" + std::string(GitDateBuffer) + "\"\n";
	versionFile += "#define " GEN_PREFIX "_GIT_TIME \"" + std::string(GitTimeBuffer) + "\"\n\n";
	versionFile += "#define " GEN_PREFIX "_BUILD_DATE \"" + std::string(NowDateBuffer) + "\"\n";
	versionFile += "#define " GEN_PREFIX "_BUILD_TIME \"" + std::string(NowTimeBuffer) + "\"\n";
	
	// Write version.h
	FILE* file = nullptr;
	fopen_s(&file, (OutputPath + std::string("version.h")).c_str(), "w");
	if (file)
	{
		fwrite(versionFile.c_str(), sizeof(char), versionFile.size(), file);
		fclose(file);
	}

	return 0;
}
