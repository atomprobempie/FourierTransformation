/*
    TODO:
        restart from temp file
        recidistance check for float
*/
/*
    Developer:
    Thanks to:


    License:
*/

#if defined __GNUC__
    #include <sys/stat.h>
    #include <unistd.h>
#elif defined _MSC_VER
    #include <Windows.h>
    #include <intrin.h>
#endif

#include <io.h>
#if defined _MSC_VER
    int F_OK = 00;
    int W_OK = 02;
    int R_OK = 04;
#endif
#include <direct.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

#include <thread>

#include <time.h>

#if defined __GNUC__
    #define _USE_MATH_DEFINES
#elif defined _MSC_VER
    const double M_PI = 3.141592653589793238462643383279;
#endif
#include <cmath>
#include <vector>
#include <iomanip>
#include <stdint.h>

void getHelp();
bool isInt(std::string input);
void getPaths(std::string &sourcePath, std::string &exportPath);
void correctPath(std::string& path);
const std::string checkFileAccess(std::string path, int arg);
const bool createDir(std::string path);
bool checkExportPath(std::string exportPath, bool forceCreatePath);
bool readInputFile(const std::string path, std::vector<float>& dataList);
void createReciLattice(std::vector<float>& reciList, int start, int ends, float distance);
void calcPartSize(std::vector<unsigned int>& boundsList, const unsigned int numbers, const unsigned int& cores);
void DFT(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList);
void DFTwithBackup(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList, std::string backupPath);
const std::string getProgressBar(float percent);
void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme);
const std::string getCurrentTime();
bool saveToFile(const std::vector<float>& reciList, const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath);

int main(int argc, char* argv[]) {
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Direct Fourier Transformation of 3D points. - development version" << std::endl << std::endl;
    std::cout << "Use ?help as source file for more information and starting with arguments." << std::endl;
    std::cout << "A wider terminal is recommend (around 100), change it in the terminal settings." << std::endl;
    std::cout << "WARNING: This program will try to use all cores! Your system should run stable though." << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
    std::cout << std::endl;

    //get paths
    std::string sourcePath;
    std::string exportPath = "export/";
    std::string backupPath = "backup/";
    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    std::vector<float> reciList;
    int reciStart = -10;
    int reciEnds = 10;
    float reciDistance = 1;

    std::vector< std::vector<float> > outputList; //outputList is the list who "manage" all threadLists
    std::vector<std::string> backupPathList;
    bool forceCreatePath = false; //force creating export path
    bool useBackup = true; //enables writing current results to hard disk, to prevent crashes
    bool RestartingCalc = true; //no restarting even if a correct temp was found
    bool autoClose = false; //closes automatically the program
    unsigned int custThreads = 0;

    {
        //path managing
        if (argc == 1) { //if no paths are given with the arguments
            getPaths(sourcePath, exportPath); //ask for import and export path
        } else {
            sourcePath = argv[1]; //source path is the first argument
            if (sourcePath == "?help") { //show help
                getHelp();
                return 0;
            }

            for (int i = 2; i < argc; i++) { //get arguments and paths
                std::string curString(argv[i]);
                if (((curString[curString.size() - 1] == '/') || (curString[curString.size() - 1] == '\\')) && (std::string(argv[i - 1]) == "-e") ) { //export path
                    exportPath = argv[i];
                } else if (((curString[curString.size() - 1] == '/') || (curString[curString.size() - 1] == '\\')) && (std::string(argv[i - 1]) == "-p") ) { //backup path
                    backupPath = argv[i];
                } else if (curString == "-f") { //force create path
                    forceCreatePath = true;
                } else if (curString == "-b") { //use backup files
                    useBackup = false;
                } else if (curString == "-c") { //no restarting of previous calculations
                    RestartingCalc = false;
                } else if ( (isInt(std::string(argv[i - 3]))) && (isInt(std::string(argv[i - 2]))) && (isInt(std::string(argv[i - 1]))) && (curString == "-s") ) { //set reciprocal space minimum and maximum (distance is 1)
                    reciStart = std::stoi( std::string(argv[i - 3]) );
                    reciEnds = std::stoi( std::string(argv[i - 2]) );
                    reciDistance = std::stof( std::string(argv[i - 1]) );
                } else if (curString == "-o") { //auto close at the end
                    autoClose = true;
                } else if (isInt(curString)) { //try forcing using this value of threads
                    if ((std::stoi(curString) >= 1) && (std::string(argv[i - 1]) == "-t")) {
                        custThreads = std::stoi(curString);
                    }
                }
            }
        }
        if (sourcePath == "?help") { //show help
            getHelp();
            return 0;
        }

        //check reci start/ end
        if (reciStart >= reciEnds) {
            std::cout << "ERROR: reciprocal lattice start is not lower as the end" << std::endl; //status msg
            std::cout << "CLOSED" << std::endl; //status msg
            return -1;
        }

        //correct possible input mistakes
        correctPath(sourcePath);
        correctPath(exportPath);
        correctPath(backupPath);

        std::cout << "Source file: " << sourcePath << std::endl;
        std::cout << "Export dir:  " << exportPath << std::endl;
        if (useBackup) {
            std::cout << "Temp dir:  " << backupPath << std::endl;
        }
    }

    {
        //check paths
        std::cout << "START: checking given paths" << std::endl; //status msg
        std::string tmpmsg = checkFileAccess(sourcePath, 1);
        if (tmpmsg != "") { //check source file path, if a error is returned
            std::cout << "ERROR: source file is " << tmpmsg << std::endl; //status msg
            std::cout << "CLOSED" << std::endl; //status msg
            return -1;
        }

        if (checkExportPath(exportPath, forceCreatePath) == false) {
            std::cout << "CLOSED" << std::endl; //status msg
            return -1;
        }
        if (useBackup) {
            if ((_access_s(backupPath.c_str(), F_OK) != 0) && (errno == ENOENT)) { //check existing of backup path
                std::cout << "Note: backup path is not existing." << std::endl; //status msg
                std::cout << "START: creating backup directory" << std::endl; //status msg
                if (!createDir(backupPath)) { //create directory
                    std::cout << "ERROR: creating backup directory" << std::endl; //status msg
                    std::cout << "WARNING: backup backup is disabled"; //status msg
                    useBackup = false;
                } else {
                    std::cout << "DONE: Creating backup directory" << std::endl; //status msg
                }
            } else { //is existing
                tmpmsg = checkFileAccess(backupPath, 2); //check exist and write access, returns an error reason if an error occurred
                if (tmpmsg != "") {
                    std::cout << "ERROR: backup path is " << tmpmsg << std::endl; //status msg
                    std::cout << "WARNING: backup backup is disabled"; //status msg
                    useBackup = false;
                }
            }
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

    //std::cout << "START: write source data to file" << std::endl; //status msg
    //std::cout << getProgressBar(-2) << std::endl; //show the snail
    //save source data here if its needed
    //std::cout << "DONE: write source data to file" << std::endl; //status msg

    //create reciprocal lattice
    createReciLattice(reciList, reciStart, reciEnds, reciDistance);
    std::cout << "Reciprocal Lattice:" << std::endl;
    std::cout << "  Start: " << reciStart << std::endl;
    std::cout << "  End: " << reciEnds << std::endl;

    //set the max threads which can be used; inclusive the main thread
    unsigned int tmpMaxThreads = std::thread::hardware_concurrency();
    if (custThreads >= 2) {
        tmpMaxThreads = custThreads;
    }
    if (tmpMaxThreads != 0) { //test if it was possible to detect the max threads
        if (tmpMaxThreads > ((reciList.size() / 3) / 2)) { //if using more threads as indices
            tmpMaxThreads = ((reciList.size() / 3) / 2); //use at least indices / 2 threads
        }
    } else { //if it was not possible use at least one extra thread
        tmpMaxThreads = 2;
    }
    const unsigned int maxThreads = tmpMaxThreads; //get the max threads which will be supported
    std::cout << "Note: " << maxThreads << " threads will be used for calculating." << std::endl; //status msg

    //init the outputlist
    for (unsigned int i = 0; i < maxThreads; i++) { //init the threadLists, every thread will save into his own threadList
        outputList.push_back(std::vector<float>());
    }

    {
        //DFT
        std::vector<unsigned int> boundsList; //init the bounds vector
        calcPartSize(boundsList, (reciList.size() / 3), maxThreads); //save the bounds into boundsList

        //init the temp paths
        for (unsigned int i = 0; i < (boundsList.size() - 1); i++) {
            backupPathList.push_back(backupPath + std::to_string(boundsList[i]) + ".tmp");
        }

        std::thread *threads = new std::thread[maxThreads - 1]; //init the calc threads
        //configure threads
        for (unsigned int i = 1; i < maxThreads; ++i) {
            if (useBackup) {
                threads[i - 1] = std::thread(DFTwithBackup, std::ref(dataList), std::ref(reciList), boundsList[i], boundsList[i + 1], std::ref(outputList[i]), backupPathList[i]); //std::ref forces the input as reference because thread doesnt allow this normally
            } else {
                threads[i - 1] = std::thread(DFT, std::ref(dataList), std::ref(reciList), boundsList[i], boundsList[i + 1], std::ref(outputList[i])); //std::ref forces the input as reference because thread doesnt allow this normally
            }
        }
        std::thread progressT = std::thread(DFTprogress, std::ref(outputList), (reciList.size() / 3), 2, 3); //DFTprogress thread

        std::cout << "START: calculating DFT" << std::endl; //status msg
        //start threads

        time_t start, end;
        time(&start);
        if (useBackup) {
            DFTwithBackup(dataList, reciList, boundsList[0], boundsList[1], outputList[0], backupPathList[0]);
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
            std::string tmpmsg = checkFileAccess(exportPath, 2); //check exist and write access, returns an error reason if an error occurred
            if (tmpmsg != "") { //if theres an error
                std::cout << "ERROR: saving DFT data" << std::endl; //status msg
                std::cout << "\texport path is " << tmpmsg << std::endl; //status msg
                std::cout << "Please choose another path. ?cancel for abort saving results.";

                std::cout << "\tIf you do not set an export path, the export folder in your execution folder will be used." << std::endl;
                std::cout << "Export path: ";
                std::getline(std::cin, exportPath);
                std::cin.get(); //"eat" the return key
                if (exportPath == "") { //if no export path, use default
                    exportPath = "export/";
                }
                std::cout << std::endl;
                if (exportPath == "?cancel") { //user cancel
                    std::cout << "User canceled." << std::endl; //status msg
                    std::cout << "CLOSED" << std::endl; //status msg
                    return 1;
                }

                correctPath(exportPath); //correct possible input mistakes
                std::cout << "Export dir:  " << exportPath << std::endl; //status msg
                if (checkExportPath(exportPath, forceCreatePath) == false) { //check and create if needed export path
                    std::cout << "ERROR: saving DFT data" << std::endl; //status msg
                    continue;
                }
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

    {
        if (useBackup) {
            bool cleanUp = true; //check var for checking if the cleaning up was succeed
            std::cout << "START: clean up temporary files" << std::endl; //status msg
            for (auto curPath : backupPathList) { //remove all backup files
                cleanUp = ((remove(curPath.c_str()) == 0) && cleanUp);
            }
            if (!cleanUp) {
                std::cout << "ERROR: clean up temporary files" << std::endl; //status msg
                std::cout << "\tmaybe could not clean up all temporary files" << std::endl; //status msg
            } else {
                std::cout << "DONE: clean up temporary files" << std::endl; //status msg
            }
        }
    }

    if (!autoClose) {
        std::cout << "Press return to close the program."; //status msg
        std::cin.get();
    }
    return 0;
}

void getHelp() { //help and info section
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "Direct Fourier Transformation of 3D points." << std::endl << std::endl;
    std::cout << "Version: development" << std::endl;
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
    std::cout << "  If no export path is given it will be exported to export/" << std::endl;
    std::cout << std::endl;
    std::cout << "START WITH ARGUMENTS" << std::endl;
    std::cout << "  You can start this program even with arguments. The source file needs to be the first argument:" << std::endl;
    std::cout << "  <source file> -e <export file> -p <temporary path> -f -b -c -s <start> <end> -o -t <threads>" << std::endl;
    std::cout << "Source File: (required)" << std::endl;
    std::cout << "  Use as absolute path or relative path to the executable folder (use only \"/\" !)"<< std::endl;
    std::cout << "  Further if a directory has a space in it surround it with \" " << std::endl;
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
    std::cout << "-s Set reciprocal space" << std::endl;
    std::cout << "  If -s is set two numbers must follow after it, first is the start and the second is the end of the" << std::endl;
    std::cout << "  reciprocal space." << std::endl;
    std::cout << "-o Automatically close the program: (optional)" << std::endl;
    std::cout << "  If -o is set it will close automatically the program." << std::endl;
    std::cout << "-t Thread numbers: (optional)" << std::endl;
    std::cout << "  If -t is set a number must follow after it, it tries to force using this value of threads." << std::endl;
    std::cout << "  At least the minimum is one threads." << std::endl;
    std::cout << std::endl;
    std::cout << "Example: ./FourierTransformation \"my import\"/path/to/source.pos -e \"my export\"/path/ -p temporary/path/ -f -b -c -s -20 20 -o -t 24" << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
}

bool isInt(std::string input) {
    for (auto curChar : input) {
        if ((curChar < 48) || (curChar > 57)) {
            return false;
        }
    }
    return true;
}

void getPaths(std::string &sourcePath, std::string &exportPath) { //ask for import and export path
    std::cout << "Note: no arguments given" << std::endl; //status msg
    std::cout << "\tYou can use absolute paths or a relative path based on the execution folder" << std::endl;
    std::cout << "\tUse /" << std::endl;
    std::cout << "Source file: ";
    std::getline(std::cin, sourcePath);
    std::cout << std::endl;

    std::cout << "\tIf you do not set an export path, the export folder in your execution folder will be used." << std::endl;
    std::cout << "Export path: ";
    std::getline(std::cin, exportPath);
    if (exportPath == "") { //if no export path, use default
        exportPath = "export/";
    }
    std::cout << std::endl;
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

const std::string checkFileAccess(std::string path, int arg) { //arg = 1: read access; arg = 2: write access; arg = 3: read & write access
    int accres;

    //check exist
    accres = _access_s(path.c_str(), F_OK);
    if (accres != 0) {
        if (errno == ENOENT) {
            return "not existing.";
        } else if (errno == EACCES) {
            return "not accessible.";
        } else {
            return "not accessible.";
        }
    }

    //check read
    if (arg & 1) {
        accres = _access_s(path.c_str(), R_OK);
        if (accres != 0) {
            return "not readable (access denied).";
        }
    }

    //check write
    if (arg & 2) {
        accres = _access_s(path.c_str(), W_OK);
        if (accres != 0) {
            if (errno == EACCES) {
                return "not writable (access denied).";
            } else if (errno == EROFS) {
                return "not writable (read-only filesystem).";
            } else {
                return "not writable.";
            }
        }
    }
    return ""; //no error detected
}

const bool createDir(std::string path) {
    std::string curPath = path;
    size_t pos = path.find("/");
    curPath = path.substr(0, pos + 1);
    if (curPath[1] == ':') { //if theres an drive letter dont prevent to create it
        pos = path.find("/", pos + 1);
        curPath = path.substr(0, pos + 1);
    }
    while (pos != std::string::npos) { //try to create each directory of the path, if its existing try the next, else return error
        #if defined _linux
        if (mkdir(curPath.c_str(), S_IRWXU) != 0) { //} prevent wrong text blocks in codeblocks
        #elif defined _WIN32 || _WIN64
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

bool checkExportPath(std::string exportPath, bool forceCreatePath) {
    if ((_access_s(exportPath.c_str(), F_OK) != 0) && (errno == ENOENT)) { //check existing of export path
        std::cout << "Note: export path is not existing." << std::endl; //status msg
        if (!forceCreatePath) { //no forcing creation of export path
            std::cout << "Should the file created? [y/n] (no will abort all pending operations and close the program)" << std::endl;
            char tmpchar;
            std::cin >> tmpchar;
            std::cin.get(); //"eat" the return key
            if (tmpchar != 'y') {
                std::cout << "User canceled." << std::endl; //status msg
                std::cout << "CLOSED" << std::endl; //status msg
                return 1;
            }
        }
        std::cout << "START: creating export directory" << std::endl; //status msg
        if (!createDir(exportPath)) { //create directory
            std::cout << "ERROR: creating export directory" << std::endl; //status msg
            return false;
        }
        std::cout << "DONE: Creating export directory" << std::endl; //status msg
    } else { //is existing
        std::string tmpmsg = checkFileAccess(exportPath, 2); //check exist and write access, returns an error reason if an error occurred
        if (tmpmsg != "") {
            std::cout << "ERROR: export path is " << tmpmsg << std::endl; //status msg
            return false;
        }
    }
    return true;
}

bool readInputFile(const std::string path, std::vector<float>& dataList) { ///NOTE: changes dataList
    std::ifstream inputFile;
    inputFile.open(path, std::ifstream::in | std::ifstream::binary);

    std::string tmpmsg = checkFileAccess(path, 1);
    if (tmpmsg != "") {
        std::cout << "ERROR: import data" << std::endl; //status msg
        std::cout << "\timport data is " << tmpmsg << std::endl; //status msg
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
        #if defined __GNUC__
            number = (__builtin_bswap32(number));
        #elif defined _MSC_VER
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

void createReciLattice(std::vector<float>& reciList, int start, int ends, float distance) { //create reciprocal space
    distance = 1;

    for (int i = start; i <= ends; i += distance) {
        for (int k = start; k <= ends; k += distance) {
            for (int o = start; o <= ends; o += distance) {
                reciList.push_back(i);
                reciList.push_back(k);
                reciList.push_back(o);
            }
        }
    }
}

void calcPartSize(std::vector<unsigned int>& boundsList, const unsigned int numbers, const unsigned int& cores) {
    unsigned int partsize = numbers / cores;

    boundsList.push_back(0); //start bound is 0
    for (unsigned int i = 1; i <= cores; i++) {
        if (i <= (numbers % cores)) { //add one extra calculation part if the threads cannot allocate overall the same part size; e.g. 4 threads, 5 calc.parts then the first thread will be calc 2
            boundsList.push_back(boundsList[i - 1] + partsize + 1);
        } else {
            boundsList.push_back(boundsList[i - 1] + partsize);
        }
    }
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

void DFTwithBackup(const std::vector<float>& dataList, const std::vector<float>& reciList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList, std::string backupPath) {
    std::ofstream backupFile;
    backupFile.open(backupPath, std::ofstream::out | std::ofstream::binary | std::ofstream::trunc);

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
    progressBar << secondPart; //smelt both parts to one
    return progressBar.str();
}

void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme) { //showbarTheme: 0 = absolut; 1 = percentage; 2 = percentage + absolut; 3 = percentage + absolut + remaining time
    time_t start, ends;
    time(&start);

    unsigned int curRawProgress = 0;
    unsigned int tmpCurRawProgress = 0;
    while(curRawProgress < finishValue) { //work until the finishValue (100%) is reached
        #if defined __GNUC__
        sleep(updateTime); //in seconds
        #elif defined _MSC_VER
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
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[25];
    localtime_s(&tstruct, &now);
    strftime(buf, sizeof(buf), "%Y_%m_%d-%H_%M_%S", &tstruct);
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
            curRawProgress++;
            std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress); //show progress

            #if defined __GNUC__
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
            #elif defined _MSC_VER
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
