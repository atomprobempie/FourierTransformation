/*
TODO:

programm should run

save results to file

maybe easier structur for saving the results, this is not nice:
    std::vector< std::vector<float> > outputListRe;
    std::vector< std::vector<float> > outputListIm;
    DFT(dataList, bounds[i], bounds[i+1], outputListRe[i], outputListIm[i]);

*/


#include <iostream>
#include <fstream>
#include <string.h>
#include <iomanip>
#include <stdint.h>
#include <thread>
#include <time.h>

#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>

int main() {
    //functions
    void readInputFile(std::string path, std::string outputPath, std::vector<float>& dataList);
    void calcPartSize(std::vector<unsigned int>& bounds, const unsigned int numbers, const unsigned int& cores);
    void DFT(std::vector<float>& dataList, unsigned int start, unsigned int M, std::vector<float>& outputListRe, std::vector<float>& outputListIm);

    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    //readInputFile("data/test.pos", "data/output.txt", dataList);
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

    { //DFT
        const unsigned int maxThreads = std::thread::hardware_concurrency(); //get the max threads which will be supported
        std::vector<unsigned int> bounds;
        calcPartSize(bounds, (dataList.size() / 3), maxThreads); //change the vector

        std::vector< std::vector<float> > outputListRe;
        std::vector< std::vector<float> > outputListIm;
        for (unsigned int i = 0; i < maxThreads; i++) {
            outputListRe.push_back(std::vector<float>());
            outputListIm.push_back(std::vector<float>());
        }

        std::thread *threads = new std::thread[maxThreads - 1];
        time_t start, end; ///DEV
        time(&start); ///DEV
        //Lauch maxThreads-1 threads
        for (unsigned int i = 1; i < maxThreads; ++i) {
            threads[i - 1] = std::thread(DFT, dataList, bounds[i], bounds[i + 1], outputListRe[i], outputListIm[i]);
        }

        //Use the main thread to do part of the work !!!
        DFT(dataList, bounds[0], bounds[1], outputListRe[0], outputListIm[0]);

        //Join maxThreads-1 threads
        for (unsigned int i = 0; i < maxThreads - 1; ++i) {
            threads[i].join();
        }
        time(&end); ///DEV
        std::cout << difftime(end, start) << std::endl; ///DEV
        std::cout << "DONE!" << std::endl; ///DEV
    } //end - DFT
/*
    { //save DFT to file
        std::ofstream outputfile; ///DEV
        const unsigned int listSize = (dataList.size() / 3); // Define the Size of the read in vector
        outputfile.open("data/DFToutput.txt", std::ios::out | std::ios::trunc); ///DEV //ios::trunc just for better viewing (is default)
        for (unsigned int i = 0; i < listSize; i++) {
            outputfile << std::setprecision(32) << "(" << OutputListRe[i * 3] << ", " << OutputListIm[i * 3] << ")\t"; ///DEV
            outputfile << std::setprecision(32) << "(" << OutputListRe[i * 3 + 1] << ", " << OutputListIm[i * 3 + 1] << ")\t"; ///DEV
            outputfile << std::setprecision(32) << "(" << OutputListRe[i * 3 + 2] << ", " << OutputListIm[i * 3 + 2] << ")" << std::endl; ///DEV
        }
    }*/
    return 0;
}

void DFT(std::vector<float>& dataList, unsigned int start, unsigned int ends, std::vector<float>& outputListRe, std::vector<float>& outputListIm) {
    const unsigned int M = (dataList.size() / 3);
    std::cout << M << std::endl; ///DEV
    float tempRe = 0;
    float tempIm = 0;
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
    for (unsigned int m = start; m < ends; m++) { //row
        if (m % 1000 == 0) //show status
        std::cout << m << "/" << M << std::endl;
        for (unsigned int n = 0; n < N; n++) { //column
            tempRe = 0;
            tempIm = 0;
            for (unsigned int k = 0; (k < M); k++) { //sum of all row
                for (unsigned int j = 0; (j < N); j++) { //sum of all column at the current row
                    curValue = &dataList[k * 3 + j];
                    tempRe += *curValue * ( std::cos( ((-2) * M_PI * m * k / M) + ((-2) * M_PI * n * j / N)) );
                    tempIm += *curValue * ( std::sin( ((-2) * M_PI * m * k / M) + ((-2) * M_PI * n * j / N)) );
                }
            }
            //std::cout << std::setprecision(32) << tempRe << "  --  " << tempIm << std::endl;
            outputListRe.push_back(tempRe);
            outputListIm.push_back(tempIm);
        }
    }
}

void readInputFile(std::string path, std::string outputPath, std::vector<float>& dataList) { ///NOTE: changes dataList
    std::ifstream file;
    std::ofstream outputfile; ///DEV
    uint32_t number;
    float floatnum;
    file.open(path, std::ios::in | std::ios::binary);
    outputfile.open(outputPath, std::ios::out | std::ios::trunc); ///DEV //ios::trunc just for better viewing (is default)
    for (unsigned int i = 0; file.read((char*)&number, sizeof(float)); i++) { //instead of i++ maybe (i % 4) (but slower)
        //convert little endian to big endian because the input is 32bit float big endian
        number = (__builtin_bswap32(number));
        char* pcUnsInt = (char*)&number;
        char* pcFloatNum = (char*)&floatnum;
        memcpy(pcFloatNum, pcUnsInt, sizeof(number));

        if ((i % 4) != 3) { //dont save every 4th input number (its the mass)
            dataList.push_back(floatnum);
        }
        outputfile << std::setprecision(32) << floatnum << std::endl; ///DEV
    }
    file.close();
    outputfile.close();
}

void calcPartSize(std::vector<unsigned int>& bounds, const unsigned int numbers, const unsigned int& cores) {
    unsigned int partsize = numbers / cores;

    bounds.push_back(0);
    std::cout << "0" << std::endl;
    for (unsigned int i = 1; i <= cores; i++) {
        if (i <= (numbers % cores)) {
            bounds.push_back(bounds[i - 1] + partsize + 1);
        } else {
             bounds.push_back(bounds[i - 1] + partsize);
        }
    }
}
