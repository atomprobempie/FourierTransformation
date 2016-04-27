/*
    License:
        GPLv3 (http://www.gnu.org/licenses/gpl.html)
*/
#include "FileStatsStruct.h"
#if _MSC_VER
    #include <io.h>
    int F_OK = 00;
    int W_OK = 02;
    int R_OK = 04;
#elif __GNUC__
    #include <unistd.h>
#endif
#include <string>
#include <sys/stat.h>

FileStatsStruct::FileStatsStruct(std::string path) {
    errno = 0;
    #if __MINGW__ || _MSC_VER
        FileStatsStruct::existing = _access_s(path.c_str(), F_OK); //check exist
    #elif __GNUC__
        FileStatsStruct::existing = access(path.c_str(), F_OK); //check exist
    #endif
    FileStatsStruct::existingErrno = errno;
    if (FileStatsStruct::existing != 0) {
        switch (FileStatsStruct::existingErrno) {
            case (ENOENT) : //not existing
                FileStatsStruct::existingStr = "not existing";
                break;
            case (EACCES) : //not accessible
                FileStatsStruct::existingStr = "not accessible";
                break;
            default :
                FileStatsStruct::existingStr = "not accessible";
                break;
        }
    }

    #if __MINGW__ || _MSC_VER
        FileStatsStruct::readable = _access_s(path.c_str(), R_OK); //check exist
    #elif __GNUC__
        FileStatsStruct::readable = access(path.c_str(), R_OK); //check exist
    #endif
    if (FileStatsStruct::readable != 0) {
        switch (FileStatsStruct::readable) {
            case (0) :
                FileStatsStruct::readableStr = "readable";
                break;
            default :
                FileStatsStruct::readableStr = "not readable (access denied)";
                break;
        }
    }

    errno = 0;
    #if __MINGW__ || _MSC_VER
        FileStatsStruct::writeable = _access_s(path.c_str(), W_OK); //check exist
    #elif __GNUC__
        FileStatsStruct::writeable = access(path.c_str(), W_OK); //check exist
    #endif
    int curErrno = errno;
    if (FileStatsStruct::writeable != 0) {
        switch (curErrno) {
            case (EACCES) : //access denied
                FileStatsStruct::writeableStr = "not writable (access denied)";
                break;
            case (EROFS) : //read only filesystem
                FileStatsStruct::writeableStr = "not writable (read-only filesystem)";
                break;
            default :
                FileStatsStruct::writeableStr = "not writable";
                break;
        }
    }

    struct stat sb;
    stat((path).c_str(), &sb);
    FileStatsStruct::fileType = sb.st_mode;
}
