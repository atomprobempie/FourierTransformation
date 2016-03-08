/*
    TODO:
        temporary result file
        restart from temp file
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
void getPaths(std::string &sourcePath, std::string &exportPath);
void correctDirPath(std::string& path);
void correctFilePath(std::string& path);
const std::string checkFileAccess(std::string path, int arg);
const bool createDir(std::string path);
bool readInputFile(const std::string path, std::vector<float>& dataList);
void calcPartSize(std::vector<unsigned int>& boundsList, const unsigned int numbers, const unsigned int& cores);
void DFT(const std::vector<float>& dataList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList);
const std::string getProgressBar(float percent);
void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme);
const std::string getCurrentTime();
bool saveToFile(const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath);


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
    std::string exportPath;
    bool forceCreateExpPath = false; //force creating export path
    {
        //path managing
        if (argc == 1) { //if no paths are given with the arguments
            getPaths(std::ref(sourcePath), std::ref(exportPath)); //ask for import and export path
        } else {
            sourcePath = argv[1]; //source path is the first argument
            if (sourcePath == "?help") { //show help
                getHelp();
                return 0;
            }
            if (argc > 2) { //export path is second argument
                exportPath = argv[2];
            } else { //no export path given
                exportPath = "export/";
            }
            if (argc > 3) { //force creating export path is third argument
                if (strcmp(argv[3], "0") != 0 && strcmp(argv[3], "1") != 0) { //check if the argument is correct
                    std::cout << "ERROR: wrong third argument" << std::endl; //status msg
                    std::cout << "CLOSED" << std::endl; //status msg
                    return -1;
                }
                forceCreateExpPath = (strcmp(argv[3], "1") == 0);
            }
        }
        if (sourcePath == "?help") { //show help
            getHelp();
            return 0;
        }

        //correct some mistakes in the paths
        correctFilePath(std::ref(sourcePath));
        correctDirPath(std::ref(exportPath));

        //check paths
        std::cout << "Source file: " << sourcePath << std::endl;
        std::cout << "Export dir:  " << exportPath << std::endl;
        std::cout << "START: checking given paths" << std::endl; //status msg
        std::string tmpmsg = checkFileAccess(sourcePath, 1);
        if (tmpmsg != "") { //check source file path, if a error is returned
            std::cout << "ERROR: source file is " << tmpmsg << std::endl; //status msg
            std::cout << "CLOSED" << std::endl; //status msg
            return -1;
        }

        if ((_access_s(exportPath.c_str(), F_OK) != 0) && (errno == ENOENT)) { //check existing of export path file
            std::cout << "Note: export path is not existing." << std::endl; //status msg
            if (!forceCreateExpPath) { //no forcing creation of export path
                std::cout << "Should the file created? [y/n]" << std::endl;
                char tmpchar;
                std::cin >> tmpchar;
                std::cin.get(); //"eat" the return key
                if (tmpchar != 'y') {
                    std::cout << "User canceled." << std::endl; //status msg
                    std::cout << "CLOSED" << std::endl; //status msg
                    return 1;
                }
            }
            std::cout << "START: Creating export directory" << std::endl; //status msg
            if (!createDir(exportPath)) { //create directory
                std::cout << "ERROR: Creating export directory" << std::endl; //status msg
                std::cout << "CLOSED" << std::endl; //status msg
                return -1;
            }
            std::cout << "DONE: Creating export directory" << std::endl; //status msg
        } else { //is existing
            tmpmsg = checkFileAccess(exportPath, 2); //check exist and write access, returns an error reason if an error occurred
            if (tmpmsg != "") {
                std::cout << "ERROR: export path is " << tmpmsg << std::endl; //status msg
                std::cout << "CLOSED" << std::endl; //status msg
                return -1;
            }
        }
    }
    std::cout << "DONE: checking given paths" << std::endl; //status msg

    //import source
    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    std::cout << "START: import data" << std::endl; //status msg
    std::cout << "\r" << getProgressBar(-2); //show progress snail
    if (!readInputFile(sourcePath, std::ref(dataList))) { //read data from file and save it into dataList
        std::cout << "CLOSED" << std::endl; //status msg
        return 1;
    }
    std::cout << "\r" << getProgressBar(100); //show progress //progress is 100
    std::cout << std::endl;
    std::cout << "DONE: import data" << std::endl; //status msg

    //std::cout << "START: write source data to file" << std::endl; //status msg
    //std::cout << getProgressBar(-2) << std::endl; //show the snail; until theres now progressbar for input reading
    //save source data here if its needed
    //std::cout << "DONE: write source data to file" << std::endl; //status msg
    /*
        //test values
        dataList.push_back(6.300058841705322265625);
        dataList.push_back(11.27175426483154296875);
        dataList.push_back(9.9945583343505859375);
        dataList.push_back(6.81645107269287109375);
        dataList.push_back(10.869720458984375);
        dataList.push_back(9.98496723175048828125);
        dataList.push_back(6.258909702301025390625);
        dataList.push_back(11.07165622711181640625);
        dataList.push_back(9.96583461761474609375);
        dataList.push_back(6.63667011260986328125);
        dataList.push_back(11.12175464630126953125);
        dataList.push_back(9.89614772796630859375);

        /* results:
        (110.1884918212890625, 0)	(-16.076107025146484375, -3.8913784027099609375)	(-16.076107025146484375, 3.8913784027099609375)
        (0.2699718475341796875, -0.016567230224609375)	(0.2219267189502716064453125, -0.40980243682861328125)	(-0.3684501945972442626953125, -0.1129741668701171875)
        (-0.462940216064453125, -6.8629455417246187920454758568667e-015)	(-1.10975933074951171875, -0.2361278235912322998046875)	(-1.10975933074951171875, 0.2361278235912322998046875)
        (0.2699718475341796875, 0.016567230224609375)	(-0.3684501945972442626953125, 0.1129741668701171875)	(0.2219267189502716064453125, 0.40980243682861328125)
        */

    //set the max threads which can be used; inclusive the main thread
    unsigned int tmpMaxThreads = std::thread::hardware_concurrency();
    if (tmpMaxThreads != 0) { //test if it was possible to detect the max threads
        if (tmpMaxThreads > ((dataList.size() / 3) / 2))  { //prevent of using more threads as indices
            tmpMaxThreads = ((dataList.size() / 3) / 2); //use at least indices / 2 threads
        } else {
            tmpMaxThreads = std::thread::hardware_concurrency();
        }
    } else { //if it was not possible use at least one extra thread
        tmpMaxThreads = 2;
    }
    const unsigned int maxThreads = tmpMaxThreads; //get the max threads which will be supported
    std::cout << "Note: " << maxThreads << " threads will be used." << std::endl; //status msg

    //init the outputlist
    std::vector< std::vector<float> > outputList; //outputList is the list who "manage" all threadLists
    for (unsigned int i = 0; i < maxThreads; i++) { //init the threadLists, every thread will save into his own threadList
        outputList.push_back(std::vector<float>());
    }

    {
        //DFT
        std::vector<unsigned int> boundsList; //init the bounds vector
        calcPartSize(boundsList, (dataList.size() / 3), maxThreads); //save the bounds into boundsList

        std::thread *threads = new std::thread[maxThreads - 1]; //init the calc threads
        time_t start, end;
        time(&start);

        //configure threads
        for (unsigned int i = 1; i < maxThreads; ++i) {
            threads[i - 1] = std::thread(DFT, std::ref(dataList), boundsList[i], boundsList[i + 1], std::ref(outputList[i])); //std::ref forces the input as reference because thread doesnt allow this normally
        }
        std::thread progressT = std::thread(DFTprogress, std::ref(outputList), (dataList.size() / 3), 2, 3); //DFTprogress thread

        std::cout << "START: calculating DFT" << std::endl; //status msg
        //start threads
        DFT(dataList, boundsList[0], boundsList[1], outputList[0]); //use the main thread for calculating too
        for (unsigned int i = 0; i < maxThreads - 1; ++i) { //join maxThreads - 1 threads
            threads[i].join();
        }
        progressT.join(); //join DFTprogress thread

        time(&end);
        std::cout << "DONE: calculating DFT" << std::endl; //status msg
        std::cout << "Note: needed time: " << std::setprecision(1) << (float) (difftime(end, start) / 60) << " minutes" << std::endl; //status msg
    } //end - DFT

    std::cout << "START: saving DFT data" << std::endl; //status msg
    if (!saveToFile(std::ref(outputList), exportPath)) { //save DFT results to file
        std::cout << "CLOSED" << std::endl; //status msg
        return -1;
    }
    std::cout << "DONE: saving DFT data" << std::endl; //status msg
    //if (argc == 1) { ///DEV //does not need to close it manually if arguments were given, because automatic systems doesnt need those
        std::cout << "Press return to close the program.";
        std::cin.get();
    //}
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
    std::cout << "You can start this program even with arguments:" << std::endl;
    std::cout << "  <source file> <export file> <force creating>" << std::endl;
    std::cout << "Source File: (needed)" << std::endl;
    std::cout << "  Use as absolute path or relative path to the executable folder (use only \"/\" !)"<< std::endl;
    std::cout << "  Further if a directory has a space in it surround it with \" " << std::endl;
    std::cout << "Export File: (optional)" << std::endl;
    std::cout << "  The result will be exported to the given path or if no path is given to export/" << std::endl;
    std::cout << "  The file is for humans readable and has this structure:" << std::endl;
    std::cout << "  (<X real part>, <X imaginary part>) (<Y r. part>, <Y i. part>) (<Z r. part>, <Z i. part>)" << std::endl;
    std::cout << "Force creating: (optional)" << std::endl;
    std::cout << "  If this option is 1 it will not ask if a missing export path should be created." << std::endl;
    std::cout << "  It will just do it." << std::endl;
    std::cout << std::endl;
    std::cout << "Example: ./FourierTransformation \"my import\"/path/to/source.pos \"my export\"/path/ 1" << std::endl;
    std::cout << "----------------------------------------------------------------------------------------------" << std::endl;
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

void correctDirPath(std::string& path) {
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
    if (path[path.size() - 1] != '/') { //adds a / at the end if its not there
        path += '/';
    }
}

void correctFilePath(std::string& path) {
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
        if (mkdir(curPath.c_str(), S_IRWXU) != 0) {
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

bool readInputFile(const std::string path, std::vector<float>& dataList) { ///NOTE: changes dataList
    std::ifstream inputFile;
    uint32_t number;
    float floatnum;

    inputFile.open(path, std::ios::in | std::ios::binary);

    std::string tmpmsg = checkFileAccess(path, 1);
    if (tmpmsg != "") {
        std::cout << "ERROR: import data" << std::endl; //status msg
        std::cout << "\timport data is " << tmpmsg << std::endl; //status msg
        return false;
    }
    inputFile.seekg(0, inputFile.end);
    std::streamoff finishValue = (inputFile.tellg() / 4); //4 bytes are 32 bit
    inputFile.seekg(0, inputFile.beg);
    if (finishValue % 4 != 0) { //if there are no 4 * X bytes the file can be corrupted because the default input file has 4 number per each point
        std::cout << "WARNING: Source file may be corrupted!" << std::endl; //status msg
    }
    for (unsigned int i = 0; inputFile.read((char*)&number, sizeof(float)); i++) { //instead of i++ maybe (i % 4) (but slower)
        //convert big endian to little endian because the input is 32bit float big endian
        #if defined __GNUC__
            number = (__builtin_bswap32(number));
        #elif defined _MSC_VER
            number = (_byteswap_ulong(number));
        #endif
        char* pcUnsInt = (char*)&number;
        char* pcFloatNum = (char*)&floatnum;
        memcpy(pcFloatNum, pcUnsInt, sizeof(number));

        if ((i % 4) != 3) { //dont save every 4th input number (its the mass)
            dataList.push_back(floatnum);
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

void DFT(const std::vector<float>& dataList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList) {
    const unsigned int M = (dataList.size() / 3); //
    float tempRe = 0; //real part
    float tempIm = 0; //imaginary part
    const float* curValue = NULL;
    const unsigned int N = 3; // number of columns

    /*
    using a 2D matrix
                N = 0  N = 1  N = 2
    M = 0    ||  x0  |  y0  |  z0  ||
    M = 1    ||  x1  |  y1  |  z1  ||
    M = 2    ||  x2  |  y2  |  z2  ||
    M = :    ||  ::  |  ::  |  ::  ||
    M = M-1  || xM-1 | yM-1 | zM-1 ||
    */
    for (unsigned int m = start; m < ends; m++) { //row; calculate only from start to end, which will make that every thread calc nearly the same
        for (unsigned int n = 0; n < N; n++) { //column
            tempRe = 0;
            tempIm = 0;
            for (unsigned int k = 0; (k < M); k++) { //sum of all row
                for (unsigned int j = 0; (j < N); j++) { //sum of all column at the current row
                    curValue = &dataList[k * 3 + j];
                    tempRe += *curValue * ( std::cos( ((-2) * M_PI * m * k / M) + ((-2) * M_PI * n * j / N)) ); //DFT without complex numbers instead using the sin/cos version of it
                    tempIm += *curValue * ( std::sin( ((-2) * M_PI * m * k / M) + ((-2) * M_PI * n * j / N)) ); //DFT without complex numbers instead using the sin/cos version of it
                }
            }
            //std::cout << std::setprecision(32) << tempRe << "  --  " << tempIm << std::endl;
            threadOutputList.push_back(tempRe); //save the real part into the threadList vector
            threadOutputList.push_back(tempIm); //save the imaginary part into the threadList vector
        }
    }
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
            tmpCurRawProgress += (curVec.size() / 6);
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

bool saveToFile(const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath) {
    std::string DFToutputFile;
    DFToutputFile = DFToutputPath + "DFToutput_" + getCurrentTime() + ".txt"; //filename with current time

    std::ofstream outputfile;
    outputfile.open(DFToutputFile, std::ios::out | std::ios::trunc); //ios::trunc just for better viewing (is default)
    if (!outputfile.good()) {
        std::cout << "ERROR: saving DFT data" << std::endl; //status msg
        std::cout << "\tError while creating output file." << std::endl; //status msg
        return false;
    }

    std::cout << "Note: saving DFT results to " << DFToutputFile << std::endl;

    unsigned int finishValue = 0;
    for(auto threadList : outputList) { //loop through all thread lists
        finishValue += (threadList.size() / 6);
    }

    unsigned int curRawProgress = 0;
    std::vector<float> tempSave;
    for(auto threadList : outputList) { //loop through all thread lists
        for(auto curValue : threadList) { //loop through all numbers of the thread list
            tempSave.push_back(curValue); //save temporary all numbers for one point before writing into file
            if (tempSave.size() == 6) { //write into file
                outputfile << std::setprecision(32) << "(" << tempSave[0] << ", " << tempSave[1] << ") ";
                outputfile << std::setprecision(32) << "(" << tempSave[2] << ", " << tempSave[3] << ") ";
                outputfile << std::setprecision(32) << "(" << tempSave[4] << ", " << tempSave[5] << ")" << std::endl;
                tempSave.clear();
                curRawProgress++;
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress); //show progress
                if (!outputfile.good()) {
                    std::cout << "ERROR: saving DFT data" << std::endl; //status msg
                    std::cout << "\tError while writing into output file." << std::endl; //status msg
                    return false;
                }
            }
        }
    }
    std::cout << std::endl;
    outputfile.close();
    return true;
}
