//
// $id: SkeletonBinary.h 2014-08-06 zhongfengqu $
//

#ifndef __SKELETONBINARY_H__
#define __SKELETONBINARY_H__

#include "spine/spine.h"

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct {
	float scale;
	char *rawdata;
	char *reader;
    char **cache;
    int cacheIndex;
	spAttachmentLoader* attachmentLoader;
} spSkeletonBinary;

spSkeletonData *spSkeletonBinary_readSkeletonData(spSkeletonBinary* self);
spSkeletonData *spSkeletonBinary_readSkeletonDataFile(spSkeletonBinary* self, const char *skeketonPath);

spSkeletonBinary* spSkeletonBinary_createWithLoader (spAttachmentLoader* attachmentLoader);
spSkeletonBinary* spSkeletonBinary_create (spAtlas* atlas);

void spSkeletonBinary_dispose (spSkeletonBinary* self);

#ifdef __cplusplus
}
#endif

#endif
