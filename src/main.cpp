/* TODO:
    catch file exceptions
    use "work" as import file
    use "export" as export file

    Future:
        custom files
*/

#include <iostream>
#include <fstream>
#include <string.h>

#include <iomanip>
#include <stdint.h>
#include <thread>

#include <time.h>
#include <unistd.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>

int main() {
    //functions
    void readInputFile(const std::string path, std::vector<float>& dataList);
    void DFT(const std::vector<float>& dataList, const unsigned int start, const unsigned int ends, std::vector<float>& threadOutputList);
    void calcPartSize(std::vector<unsigned int>& boundsList, const unsigned int numbers, const unsigned int& cores);
    const void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme);
    const void saveToFile(const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath);
    std::string getProgressBar(int percent);


    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    std::cout << "START: import data" << std::endl; //status msg
    std::cout << getProgressBar(-2) << std::endl; //show the snail; until theres now progressbar for input reading
    readInputFile("data/test.pos", std::ref(dataList)); //read data from file and save it into dataList
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

    { //DFT
        std::vector<unsigned int> boundsList; //init the bounds vector
        calcPartSize(boundsList, (dataList.size() / 3), maxThreads); //save the bounds into boundsList

        std::thread *threads = new std::thread[maxThreads - 1]; //init the calc threads
        time_t start, end; ///DEV
        time(&start); ///DEV

        //configure threads
        for (unsigned int i = 1; i < maxThreads; ++i) {
            threads[i - 1] = std::thread(DFT, std::ref(dataList), boundsList[i], boundsList[i + 1], std::ref(outputList[i])); //std::ref forces the input as reference because thread doesnt allow this normally
        }
        std::thread progressT = std::thread(DFTprogress, std::ref(outputList), (dataList.size() / 3), 2, 3); //DFTprogress thread

        std::cout << "Start: calculating DFT" << std::endl; //status msg
        //start threads
        DFT(dataList, boundsList[0], boundsList[1], outputList[0]); //use the main thread for calculating too
        for (unsigned int i = 0; i < maxThreads - 1; ++i) { //join maxThreads - 1 threads
            threads[i].join();
        }
        progressT.join(); //join DFTprogress thread

        time(&end); ///DEV
        std::cout << "DONE: calculating DFT" << std::endl; //status msg
        std::cout << "NOTE: needed time: " << std::setprecision(1) << (float) (difftime(end, start) / 60) << " minutes" << std::endl; //status msg
    } //end - DFT

    saveToFile(std::ref(outputList), "data/DFToutput.txt"); //save DFT results to file
    return 0;
}

void readInputFile(const std::string path, std::vector<float>& dataList) { ///NOTE: changes dataList
    std::ifstream inputFile;
    uint32_t number;
    float floatnum;
    inputFile.open(path, std::ios::in | std::ios::binary);
    for (unsigned int i = 0; inputFile.read((char*)&number, sizeof(float)); i++) { //instead of i++ maybe (i % 4) (but slower)
        //convert little endian to big endian because the input is 32bit float big endian
        number = (__builtin_bswap32(number));
        char* pcUnsInt = (char*)&number;
        char* pcFloatNum = (char*)&floatnum;
        memcpy(pcFloatNum, pcUnsInt, sizeof(number));

        if ((i % 4) != 3) { //dont save every 4th input number (its the mass)
            dataList.push_back(floatnum);
        }
    }
    inputFile.close();
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

const void DFTprogress(const std::vector< std::vector<float> >& outputList, const unsigned int finishValue, const int updateTime, const int showBarTheme) { //showbarTheme: 0 = absolut; 1 = percentage; 2 = percentage + absolut; 3 = percentage + absolut + remaining time
    //function
    std::string getProgressBar(const float percent);

    time_t start, end; ///DEV
    time(&start); ///DEV

    unsigned int curRawProgress = 0;
    unsigned int tmpCurRawProgress = 0;
    while(curRawProgress < finishValue) { //work until the finishValue (100%) is reached
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
            case 3: ///DEV
                time(&end); ///DEV
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress) << " | " << curRawProgress << " / " << finishValue;
                std::cout << std::fixed << " | ~" << (int) ((difftime(end, start) / curRawProgress) * (finishValue - curRawProgress) / 60) << "m"; ///DEV
                break; ///DEV
            default:
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress) << " | " << curRawProgress << " / " << finishValue;
                break;
            }
        }
        sleep(updateTime); //in seconds
    }
    std::cout << std::endl;
}

std::string getProgressBar(const int percent) { //progressbar without numbers after the decimal point
    std::stringstream progressBar;
    if ((percent < 0) || (percent > 100)) { //if illegal percent value show a snail ;)
        return "                  ...  _o**  ...                   ...";
    }
    //create the "empty" part of the bar + percent view
    progressBar.width(53 - (percent + 3) / 2);
    progressBar.fill(' ');
    progressBar << percent << "%";
    std::string secondPart = progressBar.str();
    //create the "full" part of the bar
    progressBar.str(std::string());
    progressBar.width((percent + 3) / 2);
    progressBar.fill('=');
    progressBar << " ";
    progressBar << secondPart; //smelt both parts to one
    return progressBar.str();
}
std::string getProgressBar(const float percent) { //progressbar with numbers after the decimal point
    std::stringstream progressBar;
    if ((percent < 0) || (percent > 100)) {
        return "                  ...  _o**  ...                   ..."; //if illegal percent value show a snail ;)
    }
    //create the "empty" part of the bar + percent view
    progressBar.width(54 - (percent) / 2 - ((percent == 100)? 1 : 0));
    progressBar.fill(' ');
    progressBar << std::fixed << std::setprecision(((percent != 100) ? 1 : 0)) << percent << "%";
    std::string secondPart = progressBar.str();
    progressBar.str(std::string());
    //create the "full" part of the bar
    progressBar.width((percent + 3) / 2);
    progressBar.fill('=');
    progressBar << " ";
    progressBar << secondPart; //smelt both parts to one
    return progressBar.str();
}

const void saveToFile(const std::vector< std::vector<float> >& outputList, const std::string DFToutputPath) {
    std::string getProgressBar(const float percent);

    std::ofstream outputfile; ///DEV
    outputfile.open(DFToutputPath, std::ios::out | std::ios::trunc); //ios::trunc just for better viewing (is default)

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
                outputfile << std::setprecision(32) << "(" << tempSave[0] << ", " << tempSave[1] << ")\t"; ///DEV
                outputfile << std::setprecision(32) << "(" << tempSave[2] << ", " << tempSave[3] << ")\t"; ///DEV
                outputfile << std::setprecision(32) << "(" << tempSave[4] << ", " << tempSave[5] << ")" << std::endl; ///DEV
                tempSave.clear();
                curRawProgress++;
                std::cout << "\r" << getProgressBar(100. / finishValue * curRawProgress); //show progress
            }
        }
    }
    outputfile.close();
}
