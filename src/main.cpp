#include <iostream>
#include <string.h>
#include <fstream>
#include <iomanip>
#include <stdint.h>
#include <vector>
#define _USE_MATH_DEFINES
#include <cmath>

#include <complex>

int main() {
    std::vector<float> xList, yList, zList, mList;
    std::vector<float> xListR, yListR, zListR; //Re part
    std::vector<float> xListI, yListI, zListI; //Im part
    { //reading file section
        std::ifstream file;
        std::ofstream outputfile; ///DEV
        uint32_t number;
        float floatnum;
        file.open("data/test.pos", std::ios::in | std::ios::binary);
        outputfile.open("data/output.txt", std::ios::out | std::ios::trunc); ///DEV //ios::trunc just for better viewing (is default)
        for (uint32_t i = 0; file.read((char*)&number, sizeof(float)); i++) { //instead of i++ maybe (i % 4) (but slower)
            //convert little endian to big endian because the input is 32bit float big endian
            number = (__builtin_bswap32(number));
            char* pcUnsInt = (char*)&number;
            char* pcFloatNum = (char*)&floatnum;
            memcpy(pcFloatNum, pcUnsInt, sizeof(number));

            switch (i % 4) {
            case 0:
                xList.push_back(floatnum);
                break;
            case 1:
                yList.push_back(floatnum);
                break;
            case 2:
                zList.push_back(floatnum);
                break;
            case 3:
                mList.push_back(floatnum);
                break;
            }
            outputfile << std::setprecision(32) << floatnum << std::endl; ///DEV
        }
        file.close();
        outputfile.close();
    } //end - reading file section

    { //DFT
        const uint32_t M = xList.size();
        float tempRe = 0;
        float tempIm = 0;
        const float* curValue = NULL;
        const uint32_t N = 3;

        for (uint32_t m = 0; m < M; m++) { //row
            if (m % 1000 == 0)
            std::cout << m << "/" << M << std::endl;
            for (uint32_t n = 0; n < N; n++) { //column
                tempRe = 0;
                tempIm = 0;
                for (uint32_t k = 0; (k < M); k++) { //adding all row
                    for (uint32_t j = 0; (j < N); j++) { //adding all column
                        switch (j) {
                            case 0:
                                curValue = &xList[k];
                                tempRe += *curValue * ( std::cos( ((-2) * M_PI * m * k / M) - (2 * M_PI * n * j / N)) );
                                tempIm += *curValue * ( std::sin( ((-2) * M_PI * m * k / M) - (2 * M_PI * n * j / N)) );
                                xListR.push_back(tempRe);
                                xListI.push_back(tempRe);
                                break;
                            case 1:
                                curValue = &yList[k];
                                tempRe += *curValue * ( std::cos( ((-2) * M_PI * m * k / M) - (2 * M_PI * n * j / N)) );
                                tempIm += *curValue * ( std::sin( ((-2) * M_PI * m * k / M) - (2 * M_PI * n * j / N)) );
                                yListR.push_back(tempRe);
                                yListI.push_back(tempRe);
                                break;
                            case 2:
                                curValue = &zList[k];
                                tempRe += *curValue * ( std::cos( ((-2) * M_PI * m * k / M) - (2 * M_PI * n * j / N)) );
                                tempIm += *curValue * ( std::sin( ((-2) * M_PI * m * k / M) - (2 * M_PI * n * j / N)) );
                                zListR.push_back(tempRe);
                                zListI.push_back(tempRe);
                                break;
                        }
                    }
                }
                //std::cout << tempRe << "  --  " << tempIm << "   " << (m % 3) << std::endl;
                switch (n % 3) {
                    case 0:
                        xListR.push_back(tempRe);
                        xListI.push_back(tempRe);
                        break;
                    case 1:
                        yListR.push_back(tempRe);
                        yListI.push_back(tempRe);
                        break;
                    case 2:
                        zListR.push_back(tempRe);
                        zListI.push_back(tempRe);
                        break;
                }
            }
        }
    } //end - DFT

    {//save to file
        std::ofstream outputfile; ///DEV
        const uint32_t listSize = xListR.size(); // Define the Size of the read in vector
        outputfile.open("data/DFToutput.txt", std::ios::out | std::ios::trunc); ///DEV //ios::trunc just for better viewing (is default)
        for (uint32_t i = 0; i < listSize; i++) {
            outputfile << std::setprecision(32) << "(" << xListR[i] << ", " << xListI[i] << ")\t"; ///DEV
            outputfile << std::setprecision(32) << "(" << yListR[i] << ", " << yListI[i] << ")\t"; ///DEV
            outputfile << std::setprecision(32) << "(" << zListR[i] << ", " << zListI[i] << ")" << std::endl; ///DEV
        }
    }
    return 0;
}
