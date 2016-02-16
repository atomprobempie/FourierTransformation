#include <iostream>
#include <string.h>
#include <fstream>
#include <iomanip>
#include <stdint.h>
#include <vector>
#define _USE_MATH_DEFINES
#include <cmath>

int main() {
    std::vector<float> dataList; //0: x Coord.; 1: y Coord.; 2: z Coord.
    std::vector<float> OutputListRe; //Re part
    std::vector<float> OutputListIm; //Im part
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

            if ((i % 4) != 3) { //dont save every 4th input number (its the mass)
                dataList.push_back(floatnum);
            }
            outputfile << std::setprecision(32) << floatnum << std::endl; ///DEV
        }
        file.close();
        outputfile.close();
    } //end - reading file section

/* //test values
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
        const uint32_t M = (dataList.size() / 3);
        std::cout << M << std::endl;
        float tempRe = 0;
        float tempIm = 0;
        const float* curValue = NULL;
        const uint32_t N = 3; // number of columns

        /*
        using a 2D matrix
                    N = 0  N = 1  N = 2
        M = 0    ||  x0  |  y0  |  z0  ||
        M = 1    ||  x1  |  y1  |  z1  ||
        M = 2    ||  x2  |  y2  |  z2  ||
        M = :    ||  ::  |  ::  |  ::  ||
        M = M-1  || xM-1 | yM-1 | zM-1 ||
        */
        for (uint32_t m = 0; m < M; m++) { //row
            if (m % 1000 == 0) //show status
            std::cout << m << "/" << M << std::endl;
            for (uint32_t n = 0; n < N; n++) { //column
                tempRe = 0;
                tempIm = 0;
                for (uint32_t k = 0; (k < M); k++) { //sum of all row
                    for (uint32_t j = 0; (j < N); j++) { //sum of all column at the current row
                        curValue = &dataList[k * 3 + j];
                        tempRe += *curValue * ( std::cos( ((-2) * M_PI * m * k / M) + ((-2) * M_PI * n * j / N)) );
                        tempIm += *curValue * ( std::sin( ((-2) * M_PI * m * k / M) + ((-2) * M_PI * n * j / N)) );
                    }
                }
                //std::cout << std::setprecision(32) << tempRe << "  --  " << tempIm << std::endl;
                OutputListRe.push_back(tempRe);
                OutputListIm.push_back(tempIm);
            }
        }
    } //end - DFT

    { //save DFT to file
        std::ofstream outputfile; ///DEV
        const uint32_t listSize = (dataList.size() / 3); // Define the Size of the read in vector
        outputfile.open("data/DFToutput.txt", std::ios::out | std::ios::trunc); ///DEV //ios::trunc just for better viewing (is default)
        for (uint32_t i = 0; i < listSize; i++) {
            outputfile << std::setprecision(32) << "(" << OutputListRe[i * 3] << ", " << OutputListIm[i * 3] << ")\t"; ///DEV
            outputfile << std::setprecision(32) << "(" << OutputListRe[i * 3 + 1] << ", " << OutputListIm[i * 3 + 1] << ")\t"; ///DEV
            outputfile << std::setprecision(32) << "(" << OutputListRe[i * 3 + 2] << ", " << OutputListIm[i * 3 + 2] << ")" << std::endl; ///DEV
        }
    }
    return 0;
}
