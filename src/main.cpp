/* TODO:
    progressbar for import reading
    handle different partitions under windows with absolut paths
    access vor MVS support
    documentation
    -help support
*/

#if defined __GNUC__
    #include <sys/stat.h>
    #include <unistd.h>
#elif defined _MSC_VER
    #include <direct.h>
    #include <Windows.h>
    #include <intrin.h>
    #include <io.h>
#endif

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
    std::cout << "Direct Fourier Transformation of 3D points. - under development" << std::endl << std::endl;
    std::cout << "Start the program with the argument -help for more information about starting with arguments." << std::endl;
    std::cout << "WARNING: This program will try to use all cores! Your system should run stable though." << std::endl;
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
            if (argc > 2) { //export path is second argument
                exportPath = argv[2];
            } else {
                exportPath = "export/";
            }
            if (argc > 3) { //force creating export path is third argument
                if (strcmp(argv[3], "0") != 0 && strcmp(argv[3], "1") != 0) { //check if the argument is correct
                    std::cout << "ERROR: wrong third argument" << std::endl; //status msg
                    std::cout << "CLOSED" << std::endl;
                    return -1;
                }
                forceCreateExpPath = (strcmp(argv[3], "1") == 0);
            }
        }

        //correct some mistakes in the paths
        correctFilePath(std::ref(sourcePath));
        correctDirPath(std::ref(exportPath));

        //check paths
        std::string tmpmsg = checkFileAccess(sourcePath, 1);
        if (tmpmsg.compare("") != 0) { //check source file path
            std::cout << "ERROR: source file is " << tmpmsg << std::endl; //status msg
            std::cout << "CLOSED" << std::endl;
            return -1;
        }

        if ((access(exportPath.c_str(), F_OK) != 0) && (errno == ENOENT)) { //check existing of export path file
            std::cout << "NOTE: export path is not existing." << std::endl;
            if (!forceCreateExpPath) { //no forcing creation of export path
                std::cout << "Should file created? [y/n]" << std::endl;
                char tmpchar;
                std::cin >> tmpchar;
                std::cin.get();
                if (tmpchar != 'y') {
                    std::cout << "User canceled." << std::endl;
                    std::cout << "CLOSED" << std::endl;
                    return 1;
                }
            }
            std::cout << "START: Creating export directory" << std::endl; //status msg
            if (!createDir(exportPath)) { //create directory
                std::cout << "ERROR: Creating export directory" << std::endl; //status msg
                std::cout << "CLOSED" << std::endl;
                return -1;
            }
            std::cout << "DONE: Creating export directory" << std::endl; //status msg
        } else { //is existing but has another error
            tmpmsg = checkFileAccess(exportPath, 2);
            if (tmpmsg.compare("") != 0) {
                std::cout << "ERROR: export path is " << tmpmsg << std::endl; //status msg
                std::cout << "CLOSED" << std::endl;
                return -1;
            }
        }
    }

    //import source
    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    std::cout << "START: import data" << std::endl; //status msg
    std::cout << getProgressBar(-2) << std::endl; //show the snail; until theres now progressbar for input reading
    if (!readInputFile(sourcePath, std::ref(dataList))) { //read data from file and save it into dataList
        std::cout << "CLOSED" << std::endl; //status msg
        return 1;
    }
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
    std::cout << "NOTE: " << maxThreads << " threads will be used." << std::endl; //status msg

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
        std::cout << "NOTE: needed time: " << std::setprecision(1) << (float) (difftime(end, start) / 60) << " minutes" << std::endl; //status msg
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

void getPaths(std::string &sourcePath, std::string &exportPath) {
    std::cout << "NOTE: no arguments given" << std::endl; //status msg
    std::cout << "\tYou can use absolute paths or a relative path based on the execution file" << std::endl;
    std::cout << "\tUse /" << std::endl;
    std::cout << "Source file: ";
    std::getline(std::cin, sourcePath);
    std::cout << std::endl;

    std::cout << "\tIf you dont set a export path the export folder in your execution file will be used." << std::endl;
    std::cout << "Export path: ";
    std::getline(std::cin, exportPath);
    if (exportPath.compare("") == 0) {
        exportPath = "export/";
    }
    std::cout << std::endl;
}

void correctDirPath(std::string& path) {
    size_t pos = path.find("\\");
    while(pos != std::string::npos) {
        path.replace(pos, 1, "/");
        pos = path.find("\\");
    }
    pos = path.find("//");
    while(pos != std::string::npos) {
        path.replace(pos, 2, "/");
        pos = path.find("//");
    }
    if (path[0] == '/') {
        path.erase(0,1);
    }
    if (path[path.size() - 1] != '/') {
        path += '/';
    }
}

void correctFilePath(std::string& path) {
    size_t pos = path.find("\\");
    while(pos != std::string::npos) {
        path.replace(pos, 1, "/");
        pos = path.find("\\");
    }
    pos = path.find("//");
    while(pos != std::string::npos) {
        path.replace(pos, 2, "/");
        pos = path.find("//");
    }
    if (path[0] == '/') {
        path.erase(0,1);
    }
}

/*
    #if defined __GNUC__
    if(access(DFToutputPath.c_str(), F_OK | W_OK ) == -1 ) { //if file is not existing
    #elif defined _MSC_VER
    if(access(DFToutputPath.c_str(), 0) == -1) { //if file is not existing
    #endif
*/

const std::string checkFileAccess(std::string path, int arg) { //arg = 1: read access; arg = 2: write access; arg = 3: read & write access
    int accres;

    //check exist
    accres = access(path.c_str(), F_OK);
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
        accres = access(path.c_str(), R_OK);
        if (accres != 0) {
            return "not readable (access denied).";
        }
    }

    //check write
    if (arg & 1) {
        accres = access(path.c_str(), W_OK);
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
    return "";
}

const bool createDir(std::string path) {
    std::string curPath = path;
    size_t pos = path.find("/");;
    curPath = path.substr(0, pos + 1);
    while (pos != std::string::npos) {
        #if defined _linux
        if (mkdir(curPath.c_str(), 0) != 0) {
        #elif defined _WIN32 || _WIN64
        if (mkdir(curPath.c_str()) != 0) {
        #endif
            if (errno != EEXIST) {
                return false;
            }
        }
        pos = path.find("/", pos + 1);
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
    if (tmpmsg.compare("") != 0) {
        std::cout << "ERROR: import data" << std::endl; //status msg
        std::cout << "\timport data is " << tmpmsg << std::endl; //status msg
        return false;
    }
    for (unsigned int i = 0; inputFile.read((char*)&number, sizeof(float)); i++) { //instead of i++ maybe (i % 4) (but slower)
        //convert little endian to big endian because the input is 32bit float big endian
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
            case 0:
                std::cout << "\r" << curRawProgress << " / " << finishValue;
                break;
            case 1:
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress);
                break;
            case 2:
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress) << " | " << curRawProgress << " / " << finishValue;
                break;
            default:
                time(&ends);
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress) << " | " << curRawProgress << " / " << finishValue;
                std::cout << std::fixed << " | " << (int) ((difftime(ends, start) / curRawProgress) * (finishValue - curRawProgress) / 60) << "m";
                break;
            }
        }
    }
    std::cout << std::endl;
}

const std::string getCurrentTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[25];
    tstruct = *localtime(&now);
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

    std::cout << "NOTE: saving DFT results to .../" << DFToutputFile << std::endl;

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
