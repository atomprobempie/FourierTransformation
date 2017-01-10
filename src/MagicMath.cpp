#include "MagicMath.h"

#include <cmath> // cos, sin, sqrt
#include <fstream> // ofstream

namespace MagicMath {
    const float M_PI = 3.141592653589793238462;

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

    void calcPartSize(std::vector<size_t>& boundsList, const size_t numbers, const size_t maxThreads) {
        size_t partsize = numbers / maxThreads;

        size_t curPart = 0;
        for (size_t i = 0; i < maxThreads; i++) {
            boundsList.push_back(curPart); //start value
            curPart = curPart + partsize + ((i < numbers % maxThreads) ? 1 : 0); //+ ((i < numbers % maxThreads) ? 1 : 0) : adds one extra calculation part if the threads cannot allocate overall the same part size; e.g. 4 threads, 5 calc.parts then the first thread will be calc 2
            boundsList.push_back(curPart); //end value
        }
    }

    void DFT(const std::vector<float>& dataList, const std::vector<float>& reciList, const size_t start, const size_t ends, std::vector<float>& threadOutputList) {
        const size_t srcSize = (dataList.size() / 3); //
        float tempRe = 0; //real part
        float tempIm = 0; //imaginary part

        for (size_t s = start; s < ends; s++) {
            tempRe = 0;
            tempIm = 0;
            for (size_t k = 0; k < srcSize; k++) {
                tempRe += std::cos( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
                tempIm += std::sin( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
            }
            threadOutputList.push_back(std::sqrt(tempIm * tempIm + tempRe * tempRe));  //norm of real and imaginary
        }
    }

    void DFTwithBackup(const std::vector<float>& dataList, const std::vector<float>& reciList, const size_t start, const size_t ends, std::vector<float>& threadOutputList, const std::string backupPath) {
        std::ofstream backupFile;
        backupFile.open(backupPath, std::ofstream::out | std::ofstream::binary | std::ofstream::app);

        const size_t srcSize = (dataList.size() / 3); //
        float tempRe = 0; //real part
        float tempIm = 0; //imaginary part
        float result = 0;

        for (size_t s = start; s < ends; s++) {
            tempRe = 0;
            tempIm = 0;
            for (size_t k = 0; k < srcSize; k++) {
                tempRe += std::cos( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
                tempIm += std::sin( 2 * M_PI * (dataList[k * 3] * reciList[s * 3] + dataList[k * 3 + 1] * reciList[s * 3 + 1] + dataList[k * 3 + 2] * reciList[s * 3 + 2]));
            }
            result = std::sqrt(tempIm * tempIm + tempRe * tempRe); //norm of real and imaginary
            backupFile.write((char*)& result, sizeof(float));
            threadOutputList.push_back(result);
        }
        backupFile.close();
    }

};
