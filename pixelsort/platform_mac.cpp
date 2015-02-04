//
//  platform_mac.cpp
//  pixelsort
//
//  Created by Kevin Watters on 1/22/15.
//  Copyright (c) 2015 tinygayvoxels. All rights reserved.
//

#include "platform.h"

#include <iostream>
#include <algorithm>
#include <dirent.h>

using namespace std;

static const string savePath = "~/Desktop";

static vector<string> ignorePaths = {
    ".DS_Store"
};

vector<string> listDir(const string& dirName)
{
    vector<string> filenames;
    auto dirp = opendir(dirName.c_str());
    dirent* dp;
    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_type == DT_REG) {
            string filename(dp->d_name);
            if (std::find(begin(ignorePaths), end(ignorePaths), filename) == end(ignorePaths)) {
                string absolutePath = dirName;
                absolutePath += "/";
                absolutePath += dp->d_name;
                filenames.push_back(absolutePath);
            }
        }
    }
    closedir(dirp);
    return filenames;
}

vector<string> findMovies()
{
    return listDir("/Users/kevin/fakeartist/movies");
}

string nextFilename()
{
    return "TODO.png";
    //savePath += "/" filename;
}

OSXWatcher::OSXWatcher(string filename, watchFileCallback callback_)
    : dirToWatch(filename)
    , callback(callback_)
{
}

bool OSXWatcher::start()
{
    cout << "watching " << this->dirToWatch << endl;
    CFStringRef pathToWatchCF = CFStringCreateWithCString(NULL, this->dirToWatch.c_str(), kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&pathToWatchCF, 1, NULL);
    
    FSEventStreamContext context;
    context.version = 0;
    context.info = this;
    context.retain = NULL;
    context.release = NULL;
    context.copyDescription = NULL;
    
    stream = FSEventStreamCreate(NULL, &OSXWatcher::fileSystemEventCallback, &context, pathsToWatch, kFSEventStreamEventIdSinceNow, 3.0, kFSEventStreamCreateFlagFileEvents);
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);
    
    CFRelease(pathToWatchCF);
    
    // Read the folder content to protect any unprotected or pending file
//    ReadFolderContent();
}

bool OSXWatcher::stop()
{
    FSEventStreamStop(stream);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
}

void OSXWatcher::fileSystemEventCallback(ConstFSEventStreamRef /*streamRef*/, void *clientCallBackInfo, size_t numEvents, void *eventPaths, const FSEventStreamEventFlags eventFlags[], const FSEventStreamEventId eventIds[])
{
    reinterpret_cast<OSXWatcher*>(clientCallBackInfo)->callback();
    cout << "filesystem callback" << endl;
    
    char **paths = (char **)eventPaths;
    
    for (size_t i=0; i<numEvents; i++) {
        // When a file is created we receive first a kFSEventStreamEventFlagItemCreated and second a (kFSEventStreamEventFlagItemCreated & kFSEventStreamEventFlagItemModified)
        // when the file is finally copied. Catch this second event.
        
        FSEventStreamEventFlags flags = eventFlags[i];
        
//        if (flags & kFSEventStreamEventFlagItemModified) {
            std::cout << "updated!" << std::endl;
            
            OSXWatcher *watcher = (OSXWatcher *)clientCallBackInfo;
//            if (watcher->FileValidator(paths[i]))
//                emit watcher->yourSignalHere();
//        }
    }
}