//
//  platform_mac.cpp
//  pixelsort
//
//  Created by Kevin Watters on 1/22/15.
//  Copyright (c) 2015 tinygayvoxels. All rights reserved.
//

#include "platform.h"

#include <algorithm>
#include <dirent.h>

static const string savePath = "~/Desktop";

static vector<string> ignore = {
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
            if (std::find(ignore.begin(), ignore.end(), filename) == ignore.end()) {
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
