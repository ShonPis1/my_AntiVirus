#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <limits.h>

#ifdef _WIN32
#include "dirent.h" // dirent implementation for Visual Studio
#else
#include <dirent.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define AMOUNT_ARGS 3
#define FAILURE_CODE -1
#define LOG_FILE_NAME "myAntiVirusLog.txt"
#define QUICK_SCAN_PERCENT 0.2

enum SCANNING_OPTIONS {
	NORMAL,
	FAST
};

enum SCAN_RESULT {
	CLEAN,
	INFECTED,
	SCAN_ERROR
};

bool argumentsChecker(int amount, char** args);
bool directoryExists(const char* path);
bool fileExists(const char* path);
int menu(const char* folderPath, const char* signaturePath, FILE* logFile);
int countFiles(DIR* directory);
char** getFileNames(DIR* directory, int filesCount, FILE* logFile);
int compareFileNames(const void* a, const void* b);
long calcFileSize(FILE* file);
char* getBFileContent(FILE* file, FILE* logFile, long* size);
int scanFile(const char* signature, long signatureSize, FILE* file, int option, const char** location, FILE* logFile);
bool findSignature(const char* fileContent, long fileSize, const char* signature, long signatureSize, double fromPerc, double toPerc);
void printAndLog(FILE* logFile, const char* format, ...);

int main(int argc, char** argv)
{
	if (!argumentsChecker(argc, argv))
	{
		printf("Usage: %s <directory> <virus_signature_file>\n", argv[0]);
		return FAILURE_CODE;
	}
	const char* directoryPath = argv[1];
	const char* virusSignaturePath = argv[2];

	//create the log path
	char logPath[PATH_MAX];
	if (snprintf(logPath, sizeof(logPath), "%s/%s", directoryPath, LOG_FILE_NAME) >= (int)sizeof(logPath))
	{
		printf("the directory path is too long!\n");
		return FAILURE_CODE;
	}
	FILE* logFile = fopen(logPath, "w+");
	if (!logFile)
	{
		printf("error to create the log file! \n");
		return FAILURE_CODE;
	}

	int choice = menu(directoryPath, virusSignaturePath, logFile); //getting the selected scan option

	//getting file signature (as raw bytes, it may contain zeros)
	FILE* signatureFile = fopen(virusSignaturePath, "rb");
	if (!signatureFile)
	{
		printAndLog(logFile, "Failed to open the signature file: %s\n", virusSignaturePath);
		fclose(logFile);
		return FAILURE_CODE;
	}
	long signatureSize = 0;
	char* signatureContent = getBFileContent(signatureFile, logFile, &signatureSize);
	fclose(signatureFile);
	if (!signatureContent || signatureSize == 0)
	{
		printAndLog(logFile, "The signature file is empty or unreadable!\n");
		free(signatureContent);
		fclose(logFile);
		return FAILURE_CODE;
	}

	// getting the file's names
	DIR* directory = opendir(directoryPath);
	int filesAmount = countFiles(directory);
	char** fileNames = getFileNames(directory, filesAmount, logFile);
	closedir(directory);
	qsort(fileNames, filesAmount, sizeof(char*), compareFileNames);

	char filePath[PATH_MAX]; //PATH_MAX is the size of the buffer of the path string
	for (int i = 0; i < filesAmount; i++)
	{
		if (strcmp(fileNames[i], LOG_FILE_NAME) == 0) //the log file is not part of the scan
			continue;

		// creating path for each file
		if (snprintf(filePath, sizeof(filePath), "%s/%s", directoryPath, fileNames[i]) >= (int)sizeof(filePath))
		{
			printAndLog(logFile, "The path is too long: %s/%s\n", directoryPath, fileNames[i]);
			continue;
		}

		FILE* file = fopen(filePath, "rb");
		if (!file)
		{
			printAndLog(logFile, "Failed to open the file: %s\n", filePath);
			continue;
		}

		const char* location = "";
		int state = scanFile(signatureContent, signatureSize, file, choice, &location, logFile);
		fclose(file);

		if (state == INFECTED)
			printAndLog(logFile, "%s Infected!%s\n", filePath, location);
		else if (state == CLEAN)
			printAndLog(logFile, "%s Clean!\n", filePath);
		else
			printAndLog(logFile, "%s Could not be scanned!\n", filePath);
	}
	printf("\nSee the results on %s\n", logPath);
	fclose(logFile);
	free(signatureContent);

	for (int i = 0; i < filesAmount; i++)
	{
		free(fileNames[i]);
	}
	free(fileNames);
	return 0;
}

/*checks if the entered arguments to the main are valid
input: the amount of arguments and the arguments
output: if the args are valid or not*/
bool argumentsChecker(int amount, char** args)
{
	if (amount != AMOUNT_ARGS)
		return false;

	if (!directoryExists(args[1]))
		return false;
	if (!fileExists(args[2]))
		return false;

	return true;
}

/*checks if the path is a directory and exists
* input: the path of the directory
* output: if it dir or not
*/
bool directoryExists(const char* path)
{
	DIR* checkedDir = opendir(path);
	if (checkedDir)
	{
		//The directory exists
		closedir(checkedDir);
		return true;
	}
	// Not a directory,or may be a file or does not exist
	printf("dir doesnt exist\n");
	return false;
}

/*checks if the path is is file and exists
input: the path of the file
output: if the file is exists or not */
bool fileExists(const char* path)
{
	FILE* checkedFile = fopen(path, "r");
	if (checkedFile)
	{
		// File exists
		fclose(checkedFile);
		return true;
	}
	// File does not exist
	printf("file doesnt exist\n");
	return false;
}

/*prints the menu and gets the user scan option and for the log file
input: the directory's and file's path and the log file
output: the option normal or fast*/
int menu(const char* folderPath, const char* signaturePath, FILE* logFile)
{
	char choice = '1';
	printf("Welcome to my Virus Scan! \n\n");
	fprintf(logFile, "Welcome Anti-Virus began!\n\n");

	printAndLog(logFile, "Folder to scan: %s\n", folderPath);
	printAndLog(logFile, "Virus signature: %s\n\n", signaturePath);

	printf("Press 0 for a normal scan or any other key for a quick scan: ");

	if (scanf(" %c", &choice) != 1) //no input means a quick scan
		choice = '1';
	fprintf(logFile, "Scanning option:\n");
	if (choice == '0')
		fprintf(logFile, "Normal Scan\n\n");
	else
		fprintf(logFile, "Quick Scan\n\n");

	printf("Scanning began...\n");
	printf("This process may take several minutes...\n\n");
	printf("Scanning:\n");
	fprintf(logFile, "Results:\n");
	if (choice == '0')
	{
		return NORMAL;
	}
	return FAST;
}

/*counts the amount of files in the entered dir
input: the directory
output: the amount*/
int countFiles(DIR* directory)
{
	int fileCounter = 0;
	struct dirent* dir;

	while ((dir = readdir(directory)) != NULL) //enter into the dir
	{
		if (dir->d_type == DT_REG) //checking if it's a regular file
			fileCounter++;
	}

	rewinddir(directory);
	return fileCounter;
}

/*gets all the file names in the entered dir
input: the dir and amount of files in and the log file
output: array of all the names*/
char** getFileNames(DIR* directory, int filesCount, FILE* logFile)
{
	struct dirent* dir;
	char** fileNames;

	fileNames = (char**)malloc(sizeof(char*) * (filesCount > 0 ? filesCount : 1)); //creating an array for storing the file's names
	if (!fileNames)
	{
		printAndLog(logFile, "allocation failed\n");
		exit(FAILURE_CODE); //ending the program
	}

	for (int i = 0; i < filesCount && (dir = readdir(directory)) != NULL;)
	{
		if (dir->d_type == DT_REG) //checking if the file is regular
		{
			fileNames[i] = (char*)malloc(strlen(dir->d_name) + 1); //added 1 for the null
			if (!(fileNames[i]))
			{
				printAndLog(logFile, "allocation failed\n");
				exit(FAILURE_CODE); //ending the program
			}

			strcpy(fileNames[i], dir->d_name);
			i++;
		}
	}
	rewinddir(directory);
	return fileNames;
}

/*compares two file names, used by qsort to sort the names alphabetically
input: pointers to two names
output: negative / zero / positive like strcmp*/
int compareFileNames(const void* a, const void* b)
{
	return strcmp(*(char* const*)a, *(char* const*)b);
}

/*calcs the size in bytes of file
input: the file
output: the size, or -1 on failure*/
long calcFileSize(FILE* file)
{
	if (fseek(file, 0, SEEK_END) != 0)
		return -1;
	long fileSize = ftell(file);
	rewind(file);
	return fileSize;
}

/*reads the whole content of a binary file into a buffer
input: the file, the log file, a pointer that receives the size in bytes
output: the content (the caller frees it), or NULL if the file can't be read*/
char* getBFileContent(FILE* file, FILE* logFile, long* size)
{
	*size = calcFileSize(file);
	if (*size < 0)
		return NULL;

	char* fileContent = (char*)malloc((size_t)*size + 1);
	if (!fileContent)
	{
		printAndLog(logFile, "allocation is failed!!\n");
		exit(FAILURE_CODE); //ending the program
	}

	if (fread(fileContent, 1, (size_t)*size, file) != (size_t)*size)
	{
		free(fileContent);
		return NULL;
	}
	fileContent[*size] = '\0';

	return fileContent;
}

/*reads the content of a file and scans for the signature within it
the normal scan checks the whole file, the quick scan checks only the first and the last 20%
input: the signature and its size, the file, the user option, a pointer that receives where
the signature was found (empty string / " (first 20%)" / " (last 20%)"), the log file
output: CLEAN, INFECTED or SCAN_ERROR if the file can't be read*/
int scanFile(const char* signature, long signatureSize, FILE* file, int option, const char** location, FILE* logFile)
{
	long size = 0;
	char* fileContent = getBFileContent(file, logFile, &size);
	if (!fileContent)
		return SCAN_ERROR;

	int result = CLEAN;
	*location = "";

	if (option == NORMAL)
	{
		if (findSignature(fileContent, size, signature, signatureSize, 0, 1))
			result = INFECTED;
	}
	else
	{
		if (findSignature(fileContent, size, signature, signatureSize, 0, QUICK_SCAN_PERCENT))
		{
			result = INFECTED;
			*location = " (first 20%)";
		}
		else if (findSignature(fileContent, size, signature, signatureSize, 1 - QUICK_SCAN_PERCENT, 1))
		{
			result = INFECTED;
			*location = " (last 20%)";
		}
	}

	free(fileContent);
	return result;
}

/*searches the signature of the virus in a part of the content of the checked file.
the comparison is done byte by byte with the sizes, so zero bytes in the
file or in the signature are handled correctly (unlike strstr)
input: the content of the file and its size, the signature and its size,
initial percentage of the file, final percentage of the file
output: if the signature exists in that part of the file or not*/
bool findSignature(const char* fileContent, long fileSize, const char* signature, long signatureSize, double fromPerc, double toPerc)
{
	long start = (long)(fileSize * fromPerc);
	long end = (long)(fileSize * toPerc);

	if (signatureSize <= 0 || end - start < signatureSize)
		return false; //the signature can't fit in this part

	for (long index = start; index <= end - signatureSize; index++)
	{
		if (fileContent[index] == signature[0] && memcmp(fileContent + index, signature, (size_t)signatureSize) == 0)
			return true; //the file infected
	}
	return false; //file is clear
}

/*prints a message to the screen and writes it to the log file (printf style)
input: the log file, the format and its arguments
output: none*/
void printAndLog(FILE* logFile, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);

	va_start(args, format);
	vfprintf(logFile, format, args);
	va_end(args);
}
