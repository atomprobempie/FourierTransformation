/*
    Developer:
        Iceflower S
    License:
        GPLv3 (http://www.gnu.org/licenses/gpl.html)
*/
#ifndef FILESTATSSTRUCT_H
#define FILESTATSSTRUCT_H

    #include <string>

    struct FileStatsStruct {
        int existing;
        int existingErrno;
        std::string existingStr = ""; //existing error message
        int readable;
        std::string readableStr = ""; //readable error message
        int writeable;
        std::string writeableStr = ""; //writable error message
        unsigned short fileType;

        FileStatsStruct() {};
        FileStatsStruct(std::string path);
    };

#endif // FILESTATSSTRUCT_H
