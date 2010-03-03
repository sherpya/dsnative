#define DSN_DLL_VERSION 0,0,0,4
#define DSN_API_VERSION 4

#define DSN_STRINGIFY(s) DSN_TOSTR(s)
#define DSN_TOSTR(s) #s

//#define PTS2RT(x) ((REFERENCE_TIME) llrintf(x * 1E7))
#define PTS2RT(x) ((REFERENCE_TIME) ((x) * 1E7))
#define RT2PTS(x) (((double) x) / 1E7)
