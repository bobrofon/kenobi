#define INTEL_RET_INSTRUCTION 0xc3
#define INTEL_INT3_INSTRUCTION ((char)0xcc)

long freespaceaddr(pid_t pid);
long getlibcaddr(pid_t pid);
int checkloaded(pid_t pid, const char* libname);
long getFunctionAddress(const char* funcName);
const unsigned char* findRet(const void* endAddr);
