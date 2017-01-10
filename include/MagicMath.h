#ifndef MAGICMATH_H
#define MAGICMATH_H

#include <vector> // vector
#include <stdlib.h> // size_t
#include <string> // string

namespace MagicMath {
    void createReciLattice(std::vector<float>& reciList, const float start, const float ends, const float distance);
    void calcPartSize(std::vector<size_t>& boundsList, const size_t numbers, const size_t maxThreads);
    void DFT(const std::vector<float>& dataList, const std::vector<float>& reciList, const size_t start, const size_t ends, std::vector<float>& threadOutputList);
    void DFTwithBackup(const std::vector<float>& dataList, const std::vector<float>& reciList, const size_t start, const size_t ends, std::vector<float>& threadOutputList, const std::string backupPath);
};

#endif // MAGICMATH_H
