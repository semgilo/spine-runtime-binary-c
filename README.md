# spine-runtime-binary-c
spine runtime binary for c 
# enviroment
* cocos2dx 3.9
* spine 2.1.27

# use stage 
### copy them to spine dir
### import them to spine.h
`#include <spine/SkeletonBinary.h>`
### modify entrance
```c++
std::string str_ext = skeletonDataFile.substr(skeletonDataFile.length() - 5, 5);
if (str_ext.compare(".skel") == 0)
{
	spSkeletonBinary* binary = spSkeletonBinary_create(_atlas);
	binary->scale = scale;
	spSkeletonData* skeletonData = spSkeletonBinary_readSkeletonDataFile(binary, skeletonDataFile.c_str());
	CCASSERT(skeletonData, ("Error reading skeleton data file." +  atlasFile).c_str());
    spSkeletonBinary_dispose(binary);
	setSkeletonData(skeletonData, true);
}
else 
{
	spSkeletonJson* json = spSkeletonJson_create(_atlas);
	json->scale = scale; 
	spSkeletonData* skeletonData = spSkeletonJson_readSkeletonDataFile(json, skeletonDataFile.c_str());
	CCASSERT(skeletonData, ("Error reading skeleton data file." +  atlasFile).c_str());
	spSkeletonJson_dispose(json);
	setSkeletonData(skeletonData, true);
}
```
