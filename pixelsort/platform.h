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

#endif
