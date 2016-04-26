/*
    Developer:
    Thanks to:


    License:
*/
#define __MINGW__ ( __MINGW32__ || __MINGW64__ )
#if __GNUC__
    #include <sys/stat.h>
    #include <unistd.h>
    #include <dirent.h>
#endif
#if _MSC_VER
    #include <direct.h>
    #include <io.h>
    #include <Windows.h>
    #include <intrin.h>
    #include <strsafe.h>
    #include <Shlwapi.h>
    #pragma comment(lib, "Shlwapi.lib")
#endif
#if __MINGW__
    #include <direct.h>
#endif

#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include <regex>

#include <thread>

#include <time.h>

#ifndef M_PI
    #define M_PI (3.141592653589793238462)
#endif
#ifndef floatRegex
    #define floatRegex ("((-?)[[:d:]]+)(\\.(([[:d:]]+)?))?((e|E)(-?)[:d:]+)?")
#endif
#include <cmath>
#include <vector>
#include <iomanip>
#include <stdint.h>
#include "FileStatsStruct.h"

void getHelp();
void getPaths(std::string &sourcePath, std::string &exportPath);
void getReciprocalValues(std::string& reciStart, std::string& reciEnds, std::string& reciDistance);
void correctPath(std::string& path);
const bool createDir(const std::string path);
bool checkBackUpPath(const std::string backupPath, bool& wasBackupPathCreated);
bool checkExportPath(std::string& exportPath, bool forceCreatePath);
uint32_t getHash(const std::string path);
bool readInputFile(const std::string path, std::vector<float>& dataList);
std::string checkBackup(const std::string backupPath, const uint32_t hashValue, const std::string reciStart, const std::string reciEnds, const std::string reciDistance);
void createReciLattice(std::vector<float>& reciList, const float start, const float ends, const float distance);
void calcPartSize(std::vector<unsigned int>& boundsList, const unsigned int numbers, const unsigned int maxThreads);
bool loadBackup(std::vector<unsigned int>& boundsList, std::vector< std::vector<float> >& outputList, const unsigned int ends, const std::string backupPath, const std::string backupCfgFilePath, std::vector<std::string>& backupPathList, unsigned int& maxThreads);
void DFT(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList);
void DFTwithBackup(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList, const std::string backupPath);
const std::string getProgressBar(float percent);
void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme);
const std::string getCurrentTime();
bool saveToFile(const std::vector<float>& reciList, const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath);
bool cleanBackup(const std::vector<std::string>& backupPathList, const std::string backupPath, const bool wasBackupPathCreated);

int main(int argc, char* argv[]) {
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Direct Fourier Transformation of 3D points. - 1.0" << std::endl << std::endl;
    std::cout << "Use ?help as source file for more information and starting with arguments." << std::endl;
    std::cout << "A wider terminal is recommend (around 100), change it in the terminal settings." << std::endl;
    std::cout << "WARNING: This program will try to use all cores! Your system should run stable though." << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    //declare all important things
    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    std::vector<float> reciList;
    std::vector< std::vector<float> > outputList; //outputList is the list who "manage" all threadLists
    std::vector<std::string> backupPathList;
    std::string sourcePath;
    std::string exportPath = "export/";
    std::string backupPath = "backup/";
    std::string reciStart; //reciprocal space start
    std::string reciEnds; //reciprocal space end
    std::string reciDistance; //reciprocal space distance
    bool forceCreatePath = false; //force creating export path
    bool useBackup = true; //enables writing current results to hard disk, to prevent crashes
    bool restartingCalc = true; //no restarting even if a correct temp was found
    bool autoClose = false; //closes automatically the program
    unsigned int custThreads = 0; //custom threads value
    bool wasBackupPathCreated = false;

    {
        //path managing
        if (argc == 1) { //if no paths are given with the arguments
            getPaths(sourcePath, exportPath); //ask for import and export path
            if (sourcePath == "?help") { //show help
                getHelp();
                return 0;
            }
            getReciprocalValues(reciStart, reciEnds, reciDistance); //set reciprocal lattice values
        } else {
            int neededArgs = 1; //needed arguments: source path, reciprocal space
            sourcePath = argv[1]; //source path is the first argument
            if (((sourcePath == "?help") || (sourcePath == "-help") || (sourcePath == "help")) && argc == 2) { //show help
                getHelp();
                return 0;
            }

            for (int i = 2; i < argc; i++) { //get arguments and paths
                std::string curString(argv[i]);
                if ( std::regex_match(curString, std::regex floatRegex) && std::regex_match(argv[i - 1], std::regex floatRegex) && std::regex_match(argv[i - 2], std::regex floatRegex) && (std::string(argv[i - 3]) == "-s") ) { //set reciprocal space minimum (int) and maximum (int) and distance (float)
                    neededArgs++;
                    reciStart = argv[i - 2];
                    reciEnds = argv[i - 1];
                    reciDistance = curString;
                } else if (std::string(argv[i - 1]) == "-e") { //export path
                    exportPath = argv[i];
                } else if (std::string(argv[i - 1]) == "-p") { //backup path
                    backupPath = argv[i];
                } else if (curString == "-f") { //force create path
                    forceCreatePath = true;
                } else if (curString == "-b") { //use backup files
                    useBackup = false;
                } else if (curString == "-c") { //no restarting of previous calculations
                    restartingCalc = false;
                } else if (curString == "-o") { //auto close at the end
                    autoClose = true;
                } else if (std::regex_match(curString, std::regex ("[[:d:]]+")) && (std::string(argv[i - 1]) == "-t")) { //try forcing using this value of threads
                    if (std::stoi(curString) >= 1) {
                        custThreads = std::stoi(curString);
                    }
                }
            }
            if (neededArgs < 2) {
                std::cout << "ERROR: not enough arguments set, start with -help for more information" << std::endl; //status msg
                std::cout << "CLOSED" << std::endl; //status msg
                return -1;
            }
        }

        if ((std::stof(reciStart) >= std::stof(reciEnds)) || (std::stof(reciDistance) <= 0) || (((std::stof(reciEnds) - std::stof(reciStart)) / std::stof(reciDistance)) < 1)) { //(((std::stof(reciEnds) - std::stof(reciStart)) / reciDistance) < 1) : no correct reciprocal space could be init start: 0 end: 4 but distance is 5
            std::cout << "ERROR: reciprocal values are not correct." << std::endl; //status msg
            std::cout << "CLOSED" << std::endl; //status msg
            return -1;
        }

        //correct possible input mistakes
        correctPath(sourcePath);
        if (sourcePath[sourcePath.length() - 1] == '/') {
            sourcePath.erase(sourcePath.size() - 1,1);
        }
        correctPath(exportPath);
        if (exportPath[exportPath.size() - 1] != '/') {
            exportPath = exportPath + '/';
        }
        correctPath(backupPath);
        if (backupPath[backupPath.size() - 1] != '/') {
            backupPath = backupPath + '/';
        }

        std::cout << std::endl;
        std::cout << "Source file: " << sourcePath << std::endl;
        std::cout << "Export dir:  " << exportPath << std::endl;
        if (useBackup) {
            std::cout << "Temp dir:  " << backupPath << std::endl;
        }
        std::cout << std::endl;
    }

    {
        //check paths
        std::cout << "START: checking given paths" << std::endl; //status msg
        FileStatsStruct fSS(sourcePath);
        if ((fSS.existing != 0) || (fSS.readable != 0)) { //check source file path, if a error is returned
            std::string curMsg;
            if (fSS.existing != 0) {
                curMsg = fSS.existingStr;
            } else {
                curMsg = fSS.readableStr;
            }
            std::cout << "ERROR: source file is " << curMsg << "." << std::endl;
            std::cout << "CLOSED" << std::endl; //status msg
            return -1;
        }

        if (checkExportPath(exportPath, forceCreatePath) == false) {
            std::cout << "CLOSED" << std::endl; //status msg
            return 1;
        }

        if (useBackup) {
            useBackup = checkBackUpPath(backupPath, wasBackupPathCreated);
        }
        std::cout << "DONE: checking given paths" << std::endl; //status msg
    }

    //import source
    std::cout << "START: import data" << std::endl; //status msg
    std::cout << "\r" << getProgressBar(-2); //show progress snail
    if (!readInputFile(sourcePath, dataList)) { //read data from file and save it into dataList
        std::cout << "CLOSED" << std::endl; //status msg
        return 1;
    }
    std::cout << "\r" << getProgressBar(100); //show progress //progress is 100
    std::cout << std::endl;
    std::cout << "DONE: import data" << std::endl; //status msg

    std::cout << "START: generate hash" << std::endl; //status msg
    const uint32_t sourceHash = getHash(sourcePath);
    std::cout << "DONE: generate hash" << std::endl; //status msg

    //create reciprocal lattice
    createReciLattice(reciList, std::stof(reciStart), std::stof(reciEnds), std::stof(reciDistance));
    std::cout << "Reciprocal Lattice:" << std::endl;
    std::cout << "  Start: " << reciStart << std::endl;
    std::cout << "  End: " << reciEnds << std::endl;
    std::cout << "  Distance: " << reciDistance << std::endl;

    {
        //DFT
        //set the max threads which can be used; inclusive the main thread
            std::string backupCfgFilePath; //path to the configuration backup file
            unsigned int maxThreads = 2; //max threads which will be supported
            std::vector<unsigned int> boundsList;
        if (restartingCalc && ((backupCfgFilePath = checkBackup(backupPath, sourceHash, reciStart, reciEnds, reciDistance)) != "")) {
            std::cout << "NOTE: backup found" << std::endl; //status msg
            std::cout << "START: loading backup" << std::endl; //status msg
            loadBackup(boundsList, outputList, (reciList.size() / 3), backupPath, backupCfgFilePath, backupPathList, maxThreads);
            std::cout << "DONE: loading backup" << std::endl; //status msg
        } else { //no backup found
            //set maxThread
            if (custThreads >= 1) {
                maxThreads = custThreads;
            } else {
                maxThreads = std::thread::hardware_concurrency();
            }
            //check maxThread
            if (maxThreads != 0) {//if setting maxThreads has gone wrong OR if using more threads as indices
                if (maxThreads > ((reciList.size() / 3) / 2)) {
                    maxThreads = ((reciList.size() / 3) / 2); //use at least indices / 2 threads
                }
            } else { //if something goes wrong use at least one thread
                maxThreads = 1;
            }

            calcPartSize(boundsList, (reciList.size() / 3), maxThreads); //save the bounds into boundsList, every thread get his own area

            //init the temp paths and save backup config file
            backupPathList.push_back(backupPath + "backupcfg_" + getCurrentTime() + ".ctmp"); //first entry it the config path
            std::ofstream backupConfigFile;
            backupConfigFile.open(backupPathList[0], std::ofstream::out);
            backupConfigFile << sourceHash << std::endl;
            backupConfigFile << reciStart << std::endl << reciEnds << std::endl << reciDistance << std::endl;
            for (unsigned int i = 0; i < (boundsList.size() - 1); i +=2) {
                std::string curFileName = std::to_string(boundsList[i]) + "_" + getCurrentTime() + ".tmp";
                backupPathList.push_back(backupPath + curFileName);
                backupConfigFile << curFileName << std::endl;
            }
            backupConfigFile.close();

            //init the outputlist
            for (unsigned int i = 0; i < maxThreads; i++) { //init the threadLists, every thread will save into his own threadList
                outputList.push_back(std::vector<float>());
            }
        }
        std::cout << "Note: " << maxThreads << " threads will be used for calculating." << std::endl; //status msg

        //init the outputlist
        for (unsigned int i = 0; i < maxThreads; i++) { //init the threadLists, every thread will save into his own threadList
            outputList.push_back(std::vector<float>());
        }

        //configure threads
        std::thread *threads = new std::thread[maxThreads - 1]; //init the calc threads, seems to be possible even with maxThreads = 1
        for (unsigned int i = 1; i < maxThreads; ++i) {
            if (useBackup) {
                threads[i - 1] = std::thread(DFTwithBackup, std::ref(dataList), std::ref(reciList), boundsList[i * 2], boundsList[i * 2 + 1], std::ref(outputList[i]), backupPathList[i + 1]); //std::ref forces the input as reference because thread doesnt allow this normally; backupPathList + 1 cause the first entry it the config path
            } else {
                threads[i - 1] = std::thread(DFT, std::ref(dataList), std::ref(reciList), boundsList[i * 2], boundsList[i * 2 + 1], std::ref(outputList[i])); //std::ref forces the input as reference because thread doesnt allow this normally
            }
        }
        std::thread progressT = std::thread(DFTprogress, std::ref(outputList), (reciList.size() / 3), 2, 3); //DFTprogress thread

        std::cout << "START: calculating DFT" << std::endl; //status msg
        //start threads

        time_t start, end;
        time(&start);
        if (useBackup) {
            DFTwithBackup(dataList, reciList, boundsList[0], boundsList[1], outputList[0], backupPathList[1]);
        } else {
            DFT(dataList, reciList, boundsList[0], boundsList[1], outputList[0]); //use the main thread for calculating too
        }
        for (unsigned int i = 0; i < maxThreads - 1; ++i) { //join maxThreads - 1 threads
            threads[i].join();
        }
        progressT.join(); //join DFTprogress thread

        time(&end);
        std::cout << "DONE: calculating DFT" << std::endl; //status msg
        std::cout << "Note: needed time: " << std::setprecision(1) << (float) (difftime(end, start) / 60) << " minutes" << std::endl; //status msg
    } //end - DFT

    {
        //saving DFT results
        bool userCancel = false;
        while (!userCancel) { //as long its done or user canceled
            std::cout << "START: saving DFT data" << std::endl; //status msg

            if (checkExportPath(exportPath, forceCreatePath) == false) { //check and create if needed export path
                std::cout << "CLOSED" << std::endl; //status msg
                return 1;
            }

            int maxTries = 2;
            for (int tries = 0; !(userCancel = saveToFile(reciList, outputList, exportPath)) && (tries < maxTries); tries++) { //save DFT results; trying maxTries times if needed if its done userCancel will be true
                std::cout << "ERROR: saving DFT data" << std::endl; //status msg
                if (tries < maxTries) {
                    std::cout << "\tRetrying saving DFT data" << std::endl; //status msg
                }
            }
        }
        std::cout << "DONE: saving DFT data" << std::endl; //status msg
    }

    if (useBackup) {
        std::cout << "START: clean up temporary files" << std::endl; //status msg
        if (cleanBackup(backupPathList, backupPath, wasBackupPathCreated)) {
            std::cout << "DONE: clean up temporary files" << std::endl; //status msg
        } else {
            std::cout << "ERROR: clean up temporary files" << std::endl; //status msg
            std::cout << "\tmaybe could not clean up all temporary files" << std::endl; //status msg
        }
    }

    if (!autoClose) {
        std::cout << "Press return to close the program."; //status msg
        std::cin.get();
    }
    std::cout << "CLOSED" << std::endl;
    return 0;
}

void getHelp() { //help and info section
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Direct Fourier Transformation of 3D points." << std::endl << std::endl;
    std::cout << "Version: 1.0" << std::endl;
    std::cout << "Developer: " << std::endl;
    std::cout << "Thanks to:" << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "  " << std::endl;
    std::cout << "Language: C++" << std::endl;
    std::cout << "License: " << std::endl;
    std::cout << std::endl;
    std::cout << "HELP" << std::endl;
    std::cout << "  The source file will be read as 32bit big endian binary numbers with this structure:" << std::endl;
    std::cout << "    <X Coord.> <Y Coord.> <Z Coord.> <mass>" << std::endl;
    std::cout << "  With the Coordinates a 3D Direct Fourier Transformation will be performed (no Fast Fourier)" << std::endl;
    std::cout << "  If a correct backup was found it will normally restart from the backup data." << std::endl;
    std::cout << "  The result is a reciprocal lattice with the log10 of the intensity." << std::endl;
    std::cout << "  Exported as a 32bit big endian binary file with the file ending .pos and this format:" << std::endl;
    std::cout << "    <X Coord.> <Y Coord.> <Z Coord.> <log10 of intensity>" << std::endl;
    std::cout << std::endl;
    std::cout << "START WITH ARGUMENTS" << std::endl;
    std::cout << "  You can start this program even with arguments. The source file needs to be the first argument:" << std::endl;
    std::cout << "  <source file> -s <start> <end> <distance> -e <export file> -p <temporary path> -f -b -c -o -t <threads>" << std::endl;
    std::cout << "Source File: (required)" << std::endl;
    std::cout << "  Use as absolute path or relative path to the executable folder (use only \"/\" !)"<< std::endl;
    std::cout << "  Further if a directory has a space in it surround it with \" " << std::endl;
    std::cout << "-s Set reciprocal space: (required)" << std::endl;
    std::cout << "  If -s is set three numbers must follow after it, start, end and the distance of the reciprocal space." << std::endl;
    std::cout << "-e Export File: (optional)" << std::endl;
    std::cout << "  The result will be exported to the given path or if no path is given to export/" << std::endl;
    std::cout << "  The file is for humans readable and has this structure:" << std::endl;
    std::cout << "  (<X real part>, <X imaginary part>) (<Y r. part>, <Y i. part>) (<Z r. part>, <Z i. part>)" << std::endl;
    std::cout << "-p Custom temporary path: (optional)" << std::endl;
    std::cout << "  If -p is set after it the temporary file path must follow, default path is temp/." << std::endl;
    std::cout << "  -p is not forcing using the temporary files." << std::endl;
    std::cout << "-f Force creating files: (optional)" << std::endl;
    std::cout << "  If -f is set it will not ask if a missing path should be created." << std::endl;
    std::cout << "  It will just do it." << std::endl;
    std::cout << "-b Use NOT backup files: (optional)" << std::endl;
    std::cout << "  If -t is set it will NOT create backup files with the current result." << std::endl;
    std::cout << "  If something is gone wrong it can not be continue then." << std::endl;
    std::cout << "-c Not continue calculation: (optional)" << std::endl;
    std::cout << "  If -o is set it it will not try to continue a saved, started calculation." << std::endl;
    std::cout << "-o Automatically close the program: (optional)" << std::endl;
    std::cout << "  If -o is set it will close automatically the program at the end." << std::endl;
    std::cout << "-t Thread numbers: (optional)" << std::endl;
    std::cout << "  If -t is set a number must follow after it, it tries to force using this value of threads." << std::endl;
    std::cout << "  At least the minimum is one threads." << std::endl;
    std::cout << std::endl;
    std::cout << "Example: ./FourierTransformation \"my import\"/path/to/source.pos -s -20 20 1 -e \"my export\"/path/ -p temporary/path/ -f -b -c -o -t 24" << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
}

void getPaths(std::string &sourcePath, std::string &exportPath) { //ask for import and export path
    std::cout << "Note: no arguments given" << std::endl; //status msg
    std::cout << "\tYou can use absolute paths or a relative path based on the execution folder" << std::endl;
    std::cout << "\tUse /" << std::endl;
    while (sourcePath == "") {
        std::cout << "Source file: ";
        std::getline(std::cin, sourcePath);
    }

    std::cout << "\tIf you do not set an export path, the export folder in your execution folder will be used." << std::endl;
    std::cout << "Export path: ";
    std::getline(std::cin, exportPath);
    if (exportPath == "") { //if no export path, use default
        exportPath = "export/";
    }
    std::cout << std::endl;
}

void getReciprocalValues(std::string& reciStart, std::string& reciEnds, std::string& reciDistance) {
    std::cout << "Please set the reciprocal lattice values:" << std::endl;

    bool correctInput = false;
    while (!correctInput) {
        std::cout << "Start: ";
        std::string input;
        std::getline(std::cin, input);
        if (std::regex_match(input, std::regex ("(-?)[[:d:]]+"))) {
            correctInput = true;
            reciStart = input;
        }
    }
    correctInput = false;
    while (!correctInput) {
        std::cout << "Ends: ";
        std::string input;
        std::getline(std::cin, input);
        if (std::regex_match(input, std::regex ("(-?)[[:d:]]+"))) {
            correctInput = true;
            reciEnds = input;
        }
    }
    correctInput = false;
    while (!correctInput) {
        std::cout << "Distance: ";
        std::string input;
        std::getline(std::cin, input);
        if (std::regex_match(input, std::regex floatRegex)) {
            correctInput = true;
            reciDistance = input;
        }
    }
}

void correctPath(std::string& path) {
    size_t pos = path.find("\\");
    while(pos != std::string::npos) { //replace all \ with /
        path.replace(pos, 1, "/");
        pos = path.find("\\");
    }
    pos = path.find("//");
    while(pos != std::string::npos) { //replace // with /
        path.replace(pos, 2, "/");
        pos = path.find("//");
    }
    if (path[0] == '/') { //deletes a / if its the first char
        path.erase(0,1);
    }
}

const bool createDir(const std::string path) {
    std::string curPath = path;
    size_t pos = path.find("/");
    curPath = path.substr(0, pos + 1);
    if (curPath[1] == ':') { //if theres a drive letter, prevent creation
        pos = path.find("/", pos + 1);
        curPath = path.substr(0, pos + 1);
    }
    while (pos != std::string::npos) { //try to create each directory of the path, if its existing try the next, else return error
        #if __linux
        if (mkdir(curPath.c_str(), S_IRWXU) != 0) { //} prevent wrong text blocks in codeblocks
        #elif (_WIN32 || _WIN64)
        if (_mkdir(curPath.c_str()) != 0) { //try to create all directories
        #endif
            if (errno != EEXIST) { //if theres an error different as the already "existing" error
                return false;
            }
        }
        pos = path.find("/", pos + 1); //get the next part
        curPath = path.substr(0, pos + 1);
    }
    return true;
}

bool checkBackUpPath(const std::string backupPath, bool& wasBackupPathCreated) {
    FileStatsStruct fSS(backupPath);
    if ((fSS.existing != 0) && (fSS.existingErrno == ENOENT)) { //check existing of backup path
        std::cout << "Note: backup path is not existing." << std::endl; //status msg
        std::cout << "START: creating backup directory" << std::endl; //status msg
        if (!createDir(backupPath)) { //create directory
            std::cout << "ERROR: creating backup directory" << std::endl; //status msg
            std::cout << "WARNING: backup is disabled"; //status msg
            return false;
        } else {
            wasBackupPathCreated = true;
            std::cout << "DONE: Creating backup directory" << std::endl; //status msg
            fSS = FileStatsStruct(backupPath); //refresh informations
        }
    }
    if ((fSS.existing != 0) || (fSS.writeable != 0)) { //check backup path, if an error was returned
        std::string curMsg;
        if (fSS.existing != 0) {
            curMsg = fSS.existingStr;
        } else {
            curMsg = fSS.writeableStr;
        }
        std::cout << "ERROR: backup path is " << curMsg << "." << std::endl;
        std::cout << "WARNING: backup is disabled"; //status msg
        return false;
    }
    return true;
}

bool checkExportPath(std::string& exportPath, bool forceCreatePath) {
    bool pathIsOk = false;
    while (!pathIsOk) {
        FileStatsStruct fSS(exportPath);
        if ((fSS.existing != 0) && (fSS.existingErrno == ENOENT)) { //check existing
            std::cout << "Note: export path is " << fSS.existingStr << "." << std::endl; //status msg
            if (!forceCreatePath) { //no forcing creation of export path
                bool correctInput = false;
                while (!correctInput) {
                    std::cout << "Should the file created? [y/n] (no will abort all pending operations and close the program)" << std::endl;
                    char tmpchar;
                    std::cin >> tmpchar;
                    std::cin.get(); //"eat" the return key
                    if (tmpchar == 'n') {
                        std::cout << "User canceled." << std::endl; //status msg
                        return false;
                    } else if (tmpchar == 'y') {
                        correctInput = true;
                    }
                }
            }
            std::cout << "START: creating export directory" << std::endl; //status msg
            if (!createDir(exportPath)) { //create directory
                std::cout << "ERROR: creating export directory" << std::endl; //status msg
            } else {
                std::cout << "DONE: creating export directory" << std::endl; //status msg
                fSS = FileStatsStruct(exportPath);
            }
        }

        if ((fSS.existingErrno != 0) || (fSS.writeable != 0)) { //check source directory path: writeable, directory type
            std::string curMsg;
            if (fSS.existingErrno != 0) { //existing but maybe not accessable
                curMsg = fSS.existingStr;
            } else if (fSS.writeable != 0) { //not writeable
                curMsg = fSS.writeableStr;
            }
            std::cout << "ERROR: export path is " << curMsg << "." << std::endl;
            std::cout << "Please choose another path. ?cancel for abort saving results.";
            std::cout << "\tIf you do not set an export path, the export folder in your execution folder will be used." << std::endl;
            std::cout << "Export path: ";
            std::getline(std::cin, exportPath);
            std::cin.get(); //"eat" the return key
            std::cout << std::endl;
            if (exportPath == "") { //if no export path, use default
                exportPath = "export/";
            }
            if (exportPath == "?cancel") { //user cancel
                std::cout << "User canceled." << std::endl; //status msg
                return false;
            }

            correctPath(exportPath); //correct possible input mistakes
            if (exportPath[exportPath.length() - 1] != '/') {
                exportPath = exportPath + '/';
            }
            std::cout << "Export dir:  " << exportPath << std::endl; //status msg
        } else {
            pathIsOk = true;
        }
    }
    return true;
}

uint32_t getHash(const std::string path) { //small working hash function
    std::ifstream inputFile;
    inputFile.open(path, std::ifstream::in | std::ifstream::binary);
    uint32_t number;
    uint32_t hashValue = 0;
    for (unsigned int i = 0; inputFile.read((char*)&number, sizeof(uint32_t)); i++) { //instead of i++ maybe (i % 4) (but slower)
        hashValue ^= (number ^ i);
    }
    inputFile.close();
    return hashValue;
}

bool readInputFile(const std::string path, std::vector<float>& dataList) {
    std::ifstream inputFile;
    inputFile.open(path, std::ifstream::in | std::ifstream::binary);

    FileStatsStruct fSS(path);
    if ((fSS.existing != 0) || ((fSS.fileType & S_IFMT) != S_IFREG) || (fSS.readable != 0)) { //check source file path, if a error is returned
        std::string curMsg;
        if (fSS.existing != 0) {
            curMsg = fSS.existingStr;
        } else if ((fSS.fileType & S_IFMT) != S_IFREG) {
            curMsg = "not a regular file";
        } else {
            curMsg = fSS.readableStr;
        }
        std::cout << "ERROR: import data" << std::endl; //status msg
        std::cout << "\timport data is " << curMsg << std::endl; //status msg
        return false;
    }

    inputFile.seekg(0, inputFile.end);
    std::streamoff dataSize = (inputFile.tellg() / 4); //4 bytes are 32 bit
    inputFile.seekg(0, inputFile.beg);
    if (dataSize % 4 != 0) { //if there are no 4 * X bytes the file can be corrupted because the default input file has 4 number per each point
        std::cout << "WARNING: Source file may be corrupted!" << std::endl; //status msg
    }
    uint32_t number;
    for (unsigned int i = 0; inputFile.read((char*)&number, sizeof(uint32_t)); i++) { //instead of i++ maybe (i % 4) (but slower)
        //convert big endian to little endian because the input is 32bit float big endian
        #if __GNUC__
            number = (__builtin_bswap32(number));
        #elif _MSC_VER
            number = (_byteswap_ulong(number));
        #endif
        if ((i % 4) != 3) { //dont save every 4th input number (its the mass)
            dataList.push_back(reinterpret_cast<float&>(number));
        }
    }
    if (!inputFile.good() && !inputFile.eof()) {
        std::cout << "ERROR: import data" << std::endl; //status msg
        std::cout << "\tError while reading the source file." << std::endl; //status msg
        return false;
    }
    inputFile.close();
    return true;
}

std::string checkBackup(const std::string backupPath, const uint32_t hashValue, const std::string reciStart, const std::string reciEnds, const std::string reciDistance) {
    std::vector<std::string> ctempList;

    #if __GNUC__
        struct dirent* curPartDir;
        struct stat sb;
        DIR* directory;

        directory = opendir(backupPath.c_str());
        if (directory != NULL) { //if its a directory and can be opened
            while ((curPartDir = readdir(directory))) {
                std::string curFile = (curPartDir->d_name);
                stat((backupPath + curFile).c_str(), &sb);
                if ((curFile.size() > 5) && ((sb.st_mode & S_IFMT) == S_IFREG)) { //prevent shorter names
                    if (curFile.substr(curFile.size() - 5, 5) == ".ctmp") { //if the file has the configuration temp ending
                        ctempList.push_back(curFile);
                    }
                }
            }
        }
        else {
            return "";
        }
        closedir(directory);
    #elif _MSC_VER
        std::string path = backupPath + "*.ctmp"; //if the file has the configuration temp ending
        TCHAR szDir[MAX_PATH];
        StringCchCopy(szDir, MAX_PATH, (std::wstring(path.begin(), path.end())).c_str());
        WIN32_FIND_DATA fileAttributes;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        hFind = FindFirstFile(szDir, &fileAttributes);

        if (INVALID_HANDLE_VALUE == hFind) { //if theres an error by findfirstfile
            return "";
        }

        do {
            if (fileAttributes.dwFileAttributes == FILE_ATTRIBUTE_NORMAL) { //if its a regular file
                std::wstring nameW = std::wstring(fileAttributes.cFileName); //convert cFileName to wstring
                ctempList.push_back(std::string(nameW.begin(), nameW.end()));
            }
        } while (FindNextFile(hFind, &fileAttributes) != 0); //get next file

        if (GetLastError() != ERROR_NO_MORE_FILES) {
            return "";
        }
        FindClose(hFind);
    #endif

    //get the correct config backup file if existing over the hash
	std::string hashStr = std::to_string(hashValue);
	std::string backupFile = ""; //configuration backup file name
	std::ifstream inputFile;
	for (std::string curFile : ctempList) {
		inputFile.open(backupPath + curFile, std::ifstream::in);
		std::string curLine;
		std::getline(inputFile, curLine);
		if (curLine == hashStr) {
			backupFile = curFile;
		}
		inputFile.close();
	}
	if (backupFile == "") {
		return "";
	}

    //next check, if the the backup config file has all important values
    inputFile.open(backupPath + backupFile, std::ifstream::in);
    std::string curLine;
    int i;
    for (i = 0; (std::getline(inputFile, curLine)); i++) {
        switch (i) {
            case 0 : //hash
                break;
            case 1 : //reciStart
                if (!std::regex_match(curLine, std::regex ("(-?)[[:d:]]+")) || (curLine != reciStart)) {
                    return "";
                }
                break;
            case 2 : //reciEnds
                if (!std::regex_match(curLine, std::regex ("(-?)[[:d:]]+")) || (curLine != reciEnds)) {
                    return "";
                }
                break;
            case 3 : //reciDistance
                if (!std::regex_match(curLine, std::regex floatRegex) || (curLine != reciDistance)) {
                    return "";
                }
                break;
            default : //temp pathes
                FileStatsStruct fSS(backupPath + curLine);
                if ((fSS.existing != 0) || (fSS.readable != 0)) { //is existing and readable
                    return "";
                }

                unsigned int pos = curLine.find('_', 0);
                if (pos == std::string::npos) {
                    return "";
                }
                if (!std::regex_match((curLine.substr(0, pos)), std::regex ("[[:d:]]+"))) { //if the backup paths have the start values
                    return "";
                }
                break;
        }
    }
    if (i < 4) { //not enough values
        return "";
    }
    inputFile.close();
    return (backupPath + backupFile);
}

void createReciLattice(std::vector<float>& reciList, const float start, const float ends, const float distance) { //create reciprocal space
    for (float i = start; i <= ends; i += distance) {
        for (float k = start; k <= ends; k += distance) {
            for (float o = start; o <= ends; o += distance) {
                reciList.push_back(i);
                reciList.push_back(k);
                reciList.push_back(o);
            }
        }
    }
}

void calcPartSize(std::vector<unsigned int>& boundsList, const unsigned int numbers, const unsigned int maxThreads) {
    unsigned int partsize = numbers / maxThreads;

    unsigned int curPart = 0;
    for (unsigned int i = 0; i < maxThreads; i++) {
        boundsList.push_back(curPart); //start value
        curPart = curPart + partsize + ((i < numbers % maxThreads) ? 1 : 0); //+ ((i < numbers % maxThreads) ? 1 : 0) : adds one extra calculation part if the threads cannot allocate overall the same part size; e.g. 4 threads, 5 calc.parts then the first thread will be calc 2
        boundsList.push_back(curPart); //end value
    }
}

bool loadBackup(std::vector<unsigned int>& boundsList, std::vector< std::vector<float> >& outputList, const unsigned int ends, const std::string backupPath, const std::string backupCfgFilePath, std::vector<std::string>& backupPathList, unsigned int& maxThreads) {
    //next check, if the the backup config file has all important values
    backupPathList.push_back(backupCfgFilePath);
    std::ifstream backupCfg;
    backupCfg.open(backupCfgFilePath, std::ifstream::in);
    std::string curLine;
    std::getline(backupCfg, curLine); //hash
    std::getline(backupCfg, curLine); //startreci
    std::getline(backupCfg, curLine); //endreci
    std::getline(backupCfg, curLine); //recidistance
    int i;
    for (i = 0; (std::getline(backupCfg, curLine)); i++) {
        backupPathList.push_back(backupPath + curLine);

        unsigned int curStart = std::stoi(curLine.substr(0, curLine.find('_', 0))); //already checked in checkBackup

        std::ifstream curBackupFile;
        curBackupFile.open(backupPath + curLine, std::ifstream::in | std::ifstream::binary);
        float number;
        std::vector<float> curThreadList;
        unsigned int alreadyDone;
        for (alreadyDone = 0; curBackupFile.read((char*)&number, sizeof(float)); alreadyDone++) { //instead of i++ maybe (i % 4) (but slower)
            curThreadList.push_back(number);
        }
        if (!curBackupFile.good() && !curBackupFile.eof()) {
            return false;
        }
        curBackupFile.close();

        outputList.push_back(curThreadList);
        if (curStart != 0) { //prevent adding end if its the first thread
            boundsList.push_back(curStart); //end value
        }
        boundsList.push_back(curStart + alreadyDone); //start value; original start value + already done values
    }
    boundsList.push_back(ends); //adding the last end

    maxThreads = i;
    backupCfg.close();
    return true;
}

void DFT(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList) {
    const unsigned int srcSize = (dataList.size() / 3); //
    float tempRe = 0; //real part
    float tempIm = 0; //imaginary part

    for (unsigned int s = start; s < ends; s++) {
        tempRe = 0;
        tempIm = 0;
        for (unsigned int k = 0; k < srcSize; k++) {
            tempRe += std::cos( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
            tempIm += std::sin( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
        }
        threadOutputList.push_back(std::sqrt(tempIm * tempIm + tempRe * tempRe));  //norm of real and imaginary
    }
}

void DFTwithBackup(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList, const std::string backupPath) {
    std::ofstream backupFile;
    backupFile.open(backupPath, std::ofstream::out | std::ofstream::binary | std::ofstream::app);

    const unsigned int srcSize = (dataList.size() / 3); //
    float tempRe = 0; //real part
    float tempIm = 0; //imaginary part
    float result = 0;

    for (unsigned int s = start; s < ends; s++) {
        tempRe = 0;
        tempIm = 0;
        for (unsigned int k = 0; k < srcSize; k++) {
            tempRe += std::cos( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
            tempIm += std::sin( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
        }
        result = std::sqrt(tempIm * tempIm + tempRe * tempRe); //norm of real and imaginary
        backupFile.write((char*)& result, sizeof(float));
        threadOutputList.push_back(result);
    }
    backupFile.close();
}

const std::string getProgressBar(const float percent) { //progressbar with numbers after the decimal point
    std::stringstream progressBar;
    if ((percent < 0) || (percent > 100)) {
        return "                    ...  _o**  ...                   ..."; //if illegal percent value show a snail ;)
    }
    //create the "empty" part of the bar + percent view
    progressBar.width((110 - (percent)) / 2 - ((percent == 100) ? 1 : 0) - ( ((((int) percent) == percent) && ((int) percent % 2 != 1) ? 1 : 0)));
    progressBar.fill(' ');
    progressBar << std::fixed << std::setprecision(((percent != 100) ? 1 : 0)) << percent << "%" << ((percent != 100) ? "" : " "); //last output fixes a doubled '%' at 100% cause of float output before
    std::string secondPart = progressBar.str();
    progressBar.str(std::string());
    //create the "full" part of the bar
    progressBar.width((percent + 4) / 2);
    progressBar.fill('=');
    progressBar << " ";
    progressBar << secondPart; //melt both parts to one
    return progressBar.str();
}

void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme) { //showbarTheme: 0 = absolut; 1 = percentage; 2 = percentage + absolut; 3 = percentage + absolut + remaining time
    time_t start, ends;
    time(&start);

    unsigned int curRawProgress = 0;
    unsigned int tmpCurRawProgress = 0;
    while(curRawProgress < finishValue) { //work until the finishValue (100%) is reached
        #if __GNUC__
        sleep(updateTime); //in seconds
        #elif _MSC_VER
        Sleep(updateTime * 1000); //in seconds
        #endif
        tmpCurRawProgress = 0;
        for(auto curVec : outputList) {
            tmpCurRawProgress += (curVec.size());
        }
        if (tmpCurRawProgress != curRawProgress) { //show only if the progress has changed
            curRawProgress = tmpCurRawProgress;
            switch(showBarTheme) { //show progressbar
            case 0: //only absolute value
                std::cout << "\r" << curRawProgress << " / " << finishValue;
                break;
            case 1: //only progress bar
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress);
                break;
            case 2: //progress bar + absolute value
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress) << " | " << curRawProgress << " / " << finishValue;
                break;
            default: //progress bar + absolute value + remaining time
                time(&ends);
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress) << " | " << curRawProgress << " / " << finishValue;
                std::cout << std::fixed << " | " << (int) ((difftime(ends, start) / curRawProgress) * (finishValue - curRawProgress) / 60) << "m";
                break;
            }
        }
    }
    std::cout << std::endl;
}

const std::string getCurrentTime() { //return the current time. format: year_month_day_hour_minute_second
    time_t now = time(0);
    char buf[25];
    #if (__MINGW__ || _MSC_VER)
        struct tm  tstruct;
        localtime_s(&tstruct, &now);
        strftime(buf, sizeof(buf), "%Y_%m_%d-%H_%M_%S", &tstruct);
    #elif __GNUC__
        struct tm* tstruct;
        tstruct = localtime(&now);
        strftime(buf, sizeof(buf), "%Y_%m_%d-%H_%M_%S", tstruct);
    #endif
    return buf;
}

bool saveToFile(const std::vector<float>& reciList, const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath) {
    std::string DFToutputFile;
    DFToutputFile = DFToutputPath + "DFToutput_" + getCurrentTime() + ".pos"; //filename with current time

    std::ofstream outputFile;
    outputFile.open(DFToutputFile, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc); //ios::trunc just for better viewing (is default)
    if (!outputFile.good()) {
        std::cout << "ERROR: saving DFT data" << std::endl; //status msg
        std::cout << "\tError while creating output file." << std::endl; //status msg
        return false;
    }

    std::cout << "Note: saving DFT results to " << DFToutputFile << std::endl;

    unsigned int finishValue = (reciList.size() / 3);
    unsigned int curRawProgress = 0;

    for(auto threadList : outputList) { //loop through all thread lists
        for(auto curValue : threadList) { //loop through all numbers of the thread list
            curValue = std::log10(curValue);
            curRawProgress++;
            std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress); //show progress

            #if __GNUC__
                float curReciValue = reciList[(curRawProgress - 1) * 3];
                uint32_t curConvertedValue = (__builtin_bswap32( reinterpret_cast<uint32_t&>(curReciValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));

                curReciValue = reciList[(curRawProgress - 1) * 3 + 1];
                curConvertedValue = (__builtin_bswap32( reinterpret_cast<uint32_t&>(curReciValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));

                curReciValue = reciList[(curRawProgress - 1) * 3 + 2];
                curConvertedValue = (__builtin_bswap32( reinterpret_cast<uint32_t&>(curReciValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));

                curConvertedValue = (__builtin_bswap32( reinterpret_cast<uint32_t&>(curValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));
            #elif _MSC_VER
                float curReciValue = reciList[(curRawProgress - 1) * 3];
                uint32_t curConvertedValue = (_byteswap_ulong( reinterpret_cast<uint32_t&>(curReciValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));

                curReciValue = reciList[(curRawProgress - 1) * 3 + 1];
                curConvertedValue = (_byteswap_ulong( reinterpret_cast<uint32_t&>(curReciValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));

                curReciValue = reciList[(curRawProgress - 1) * 3 + 2];
                curConvertedValue = (_byteswap_ulong( reinterpret_cast<uint32_t&>(curReciValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));

                curConvertedValue = (_byteswap_ulong( reinterpret_cast<uint32_t&>(curValue) )); //convert little endian to big endian
                outputFile.write((char*) &curConvertedValue, sizeof(uint32_t));
            #endif
            if (!outputFile.good()) {
                std::cout << "ERROR: saving DFT data" << std::endl; //status msg
                std::cout << "\tError while writing into output file." << std::endl; //status msg
                return false;
            }
        }
    }
    std::cout << std::endl;
    outputFile.close();
    return true;
}

bool cleanBackup(const std::vector<std::string>& backupPathList, const std::string backupPath, const bool wasBackupPathCreated) {
    bool cleanUp = true; //check var for checking if the cleaning up was succeed
    for (auto curPath : backupPathList) { //remove all backup files
        cleanUp = ((std::remove(curPath.c_str()) == 0) && cleanUp);
    }
    if (wasBackupPathCreated) { //remove backupPath if it was created
        cleanUp = ((std::remove(backupPath.c_str()) == 0) && cleanUp);
    } else { //test if the backup path is empty then delete the backup path too
        #if __GNUC__
            DIR* directory = opendir(backupPath.c_str());
            if (directory != NULL) { //if its a directory and can be opened
                int i = 0;
                for (i = 0; (i < 3) && (readdir(directory)); i++) {

                }
                if (i <= 2) { //empty
                    cleanUp = ((std::remove(backupPath.c_str()) == 0) && cleanUp);
                }
            }
            closedir(directory);
        #elif _MSC_VER
            TCHAR szDir[MAX_PATH];
            StringCchCopy(szDir, MAX_PATH, (std::wstring(backupPath.begin(), backupPath.end())).c_str());

            if (PathIsDirectoryEmpty(szDir)) {
                std::cout << GetLastError();
                cleanUp = ((std::remove(backupPath.c_str()) == 0) && cleanUp);
            }
        #endif
    }
    return cleanUp;
}
