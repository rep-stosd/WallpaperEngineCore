#include <string>
#import <Foundation/Foundation.h>


std::string GetBundleFilePath(const std::string& filename) {
#if TARGET_OS_IPHONE
    NSString *full = [NSString stringWithUTF8String:filename.data()];
    NSString *name = [full stringByDeletingPathExtension];
    NSString *ext  = [full pathExtension];

    NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:ext];
    if (!path) {
        std::string cpath = GetBundleFilePath("tahoe.tex");
        size_t r = cpath.rfind("/", -1, 1);
        if (r != std::string::npos) {
            cpath  = cpath.substr(0, r + 1);
            cpath = cpath + filename;
            return cpath;
        }
    }
    return std::string([path UTF8String]);
#else
    return filename;
#endif
}
