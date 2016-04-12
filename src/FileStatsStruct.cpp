#include "FileStatsStruct.h"
#if defined _MSC_VER
    int F_OK = 00;
    int W_OK = 02;
    int R_OK = 04;
#endif
#include <string>
#include <unistd.h>
#include <sys/stat.h>

FileStatsStruct::FileStatsStruct(std::string path) {
    errno = 0;
    FileStatsStruct::existing = _access_s(path.c_str(), F_OK); //check exist
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

    FileStatsStruct::readable = _access_s(path.c_str(), R_OK); //check read
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
    FileStatsStruct::writeable = _access_s(path.c_str(), W_OK); //check write
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
