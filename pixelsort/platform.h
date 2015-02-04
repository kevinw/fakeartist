//
//  platform.h
//  pixelsort
//
//  Created by Kevin Watters on 1/22/15.
//  Copyright (c) 2015 tinygayvoxels. All rights reserved.
//

#ifndef pixelsort_platform_h
#define pixelsort_platform_h

#include <vector>
#include <string>



using std::string;
using std::vector;

vector<string> listDir(const string& dirName);
vector<string> findMovies();
string nextFilename();

typedef void (*watchFileCallback)();

#ifdef __APPLE__


#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>


class OSXWatcher
{
public:
    OSXWatcher(string strDirectory, watchFileCallback callback);
    
    bool start();
    bool stop();
    
private:
    static void fileSystemEventCallback(ConstFSEventStreamRef streamRef, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[]);
    
    FSEventStreamRef stream;
    string dirToWatch;
    watchFileCallback callback;
};

#endif

#endif
