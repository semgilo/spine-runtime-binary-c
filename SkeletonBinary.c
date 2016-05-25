//
// $id: SkeletonBinary.c semgilo $
//

#include "SkeletonBinary.h"
#include "spine/extension.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
typedef enum {
	SP_CURVE_LINEAR,
	SP_CURVE_STEPPED,
	SP_CURVE_BEZIER,
} spCurveType;

typedef struct {
	spSkeletonBinary super;
	int ownsLoader;
} _spSkeletonBinary;

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX(a, b) (((a)>(b))?(a):(b))
#define READ() (((int)*self->reader++) & 0xFF)

#define inline __inline

static inline int readBoolean(spSkeletonBinary *self)
{
    int ch = READ();
    return ch != 0;
}

static inline char readChar(spSkeletonBinary *self)
{
    int ch = READ();
    return (char)(ch);
}

static inline short readShort(spSkeletonBinary *self)
{
    int ch1 = READ();
    int ch2 = READ();
    return (short)((ch1 << 8) + (ch2 << 0));
}

static inline int readInt(spSkeletonBinary *self)
{
    int ch1 = READ();
    int ch2 = READ();
    int ch3 = READ();
    int ch4 = READ();
    return ((ch1 << 24) | (ch2 << 16) | (ch3 << 8) | (ch4 << 0));
}

static inline int readIntOptimize(spSkeletonBinary *self)
{
	int b = READ();
	int result = (b) & 0x7F;
	if ((b & 0x80) != 0) {
		b = READ();
		result = result | ((b & 0x7F) << 7);
		if ((b & 0x80) != 0) {
			b=READ();
			result=result | ((b & 0x7F) << 14);
			if ((b & 0x80) != 0) {
				b=READ();
				result=result | ((b & 0x7F) << 21);
				if ((b & 0x80) != 0) {
					b=READ();
					result=result | ((b & 0x7F) << 28);
				}
			}
		}
	}
	return  result;
}

static inline float readFloat(spSkeletonBinary *self)
{
    union {
        float f;
        int i;
    } u;

    u.i = readInt(self);
    
	if (u.f<=DBL_MAX&&u.f>=-DBL_MAX)
	{
		return u.f;
	}
//     if (isinf(u.f)) {
//         printf("%d to float is inf \n", u.i);
//         return 1.0;
//     }
//     
//     if (isnan(u.f)) {
//         printf("%d to float is nan \n", u.i);
//         return 1.0;
//     }
    
//     if (u.f > 1000 || u.f < -1000) {
//         printf("%d to float is warning %f \n", u.i, u.f);
//         return 1.0;
//     }
    
    return 1.0;
//    int bits = readInt(self);
//    
//    int s = ((bits >> 31) == 0) ? 1 : -1;
//    int e = ((bits >> 23) & 0xff);
//    int m = (e == 0) ? (bits & 0x7fffff) << 1 : (bits & 0x7fffff) | 0x800000;
//    float f = s*m*pow(2, 3-150);
//    if (isinf(f)) {
//        return 0.0;
//    }
//    
//    if (isnan(f)) {
//        return 0.0;
//    }
//    
//    return f;
}

static inline const char *readString(spSkeletonBinary *self)
{
    int length, i;
	char* pszContent;

	length = readIntOptimize(self);
	
	if ( length == 0 ) return NULL;
	pszContent = MALLOC(char, length);
    for (i = 0; i < length - 1; i ++) {
        pszContent[i] = readChar(self);
    }
    pszContent[length - 1] = '\0';
    
    if (self->cacheIndex >= 1024 * 5) {
        printf("cache is full !!!!!!!");
        return "";
    }
    self->cache[self->cacheIndex ++] = pszContent;
    return pszContent;
}

static inline float *readFloats(spSkeletonBinary *self, float scale, size_t *length)
{
    float *arr;
    int i;
    int n = readIntOptimize(self);
    *length = n;
    arr = MALLOC(float, n);
    for (i = 0; i < n; i++)
    {
        arr[i] = readFloat(self) * scale;
    }

    return arr;
}

static inline int *readShorts(spSkeletonBinary *self, size_t *length)
{
    int *arr;
    int i;
    int n = readIntOptimize(self);
    *length = n;
    arr = 0;
    arr = MALLOC(int, n);
    for (i = 0; i < n; i++)
    {
        arr[i] = readShort(self);
    }

    return arr;
}

static inline void readColor(spSkeletonBinary *self, float *r, float *g, float *b, float *a)
{
    *r = READ() / (float)255;
    *g = READ() / (float)255;
    *b = READ() / (float)255;
    *a = READ() / (float)255;
}

static spAttachment *readAttachment(spSkeletonBinary *self, spSkin *Skin, const char *attachmentName)
{
    int attachmentType;
    float scale = self->scale;
    const char *name = readString(self); 
    if (name == NULL) name = attachmentName;
    
    
    switch (readChar(self)) {
        case 0:
            attachmentType = SP_ATTACHMENT_REGION;
            break;
        case 1:
            attachmentType = SP_ATTACHMENT_BOUNDING_BOX;
            break;
        case 2:
            attachmentType = SP_ATTACHMENT_MESH;
            break;
        case 3:
            attachmentType = SP_ATTACHMENT_SKINNED_MESH;
            break;
            
        default:
            printf("unknow attachment type : -----------");
            break;
    }

    switch (attachmentType)
    {
        case SP_ATTACHMENT_REGION:
        {
            spAttachment *attachment;
            spRegionAttachment *region;
            const char *path = readString(self);
            if (path == NULL) path = name;

            attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, 
                Skin, SP_ATTACHMENT_REGION, attachmentName, path);
            region = SUB_CAST(spRegionAttachment, attachment);
            if (path) MALLOC_STR(region->path, path);

            region->x = readFloat(self) * scale;
            region->y = readFloat(self) * scale;
            region->scaleX = readFloat(self);
            region->scaleY = readFloat(self);
            region->rotation = readFloat(self);
            region->width = readFloat(self) * scale;
            region->height = readFloat(self) * scale;
            readColor(self, &region->r, &region->g, &region->b, &region->a);

            spRegionAttachment_updateOffset(region);

            return SUPER_CAST(spAttachment, region);
        }
        case SP_ATTACHMENT_BOUNDING_BOX: 
        {
            size_t length;
			spBoundingBoxAttachment *box;

            spAttachment *attachment = spAttachmentLoader_newAttachment(self->attachmentLoader,
                Skin, SP_ATTACHMENT_BOUNDING_BOX, attachmentName, NULL);
            if (attachment == NULL) {
                return NULL;
            }
            box = SUB_CAST(spBoundingBoxAttachment, attachment);

            box->vertices = readFloats(self, scale, &length);
            box->verticesCount = (int)length;

            return SUPER_CAST(spAttachment, box);
        }
        case SP_ATTACHMENT_MESH: 
        {
            size_t length;
            spAttachment *attachment;
            spMeshAttachment *mesh;
            const char *path = readString(self);
            if (path == NULL) path = name;

            attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, 
                Skin, SP_ATTACHMENT_MESH, attachmentName, path);
            if (attachment == NULL) {
                return NULL;
            }
            mesh = SUB_CAST(spMeshAttachment, attachment);
            if (path) MALLOC_STR(mesh->path, path);

            mesh->regionUVs = readFloats(self, 1, &length);
            mesh->triangles = readShorts(self, &length);
            mesh->trianglesCount = (int)length;
            mesh->vertices = readFloats(self, scale, &length);
            mesh->verticesCount = (int)length;
            readColor(self, &mesh->r, &mesh->g, &mesh->b, &mesh->a);
            mesh->hullLength = readIntOptimize(self) * 2;

            spMeshAttachment_updateUVs(mesh);
			 
            return SUPER_CAST(spAttachment, mesh);
        }
        case SP_ATTACHMENT_SKINNED_MESH: 
        {
            int verticesCount, b, w, nn, i;
            size_t length;
            spAttachment *attachment;
            spSkinnedMeshAttachment *mesh;
            float* vertices;
            const char *path = readString(self);
            if (path == NULL) path = name;

            attachment = spAttachmentLoader_newAttachment(self->attachmentLoader, 
                Skin, SP_ATTACHMENT_SKINNED_MESH, attachmentName, path);
            if (attachment == NULL) {
                return NULL;
            }
            mesh = SUB_CAST(spSkinnedMeshAttachment, attachment);
            if (path) MALLOC_STR(mesh->path, path);
            
            mesh->regionUVs = readFloats(self, 1, &length);
            mesh->uvsCount = (int)length;
            mesh->triangles = readShorts(self, &length);
            mesh->trianglesCount = (int)length;

            verticesCount = readIntOptimize(self);
            vertices = CALLOC(float, verticesCount);
            for (i = 0; i < verticesCount;) {
                int bonesCount;
                vertices[i] = readFloat(self);
                bonesCount = (int)vertices[i++];
                mesh->bonesCount += bonesCount + 1;
                mesh->weightsCount += bonesCount * 3;
                for (nn = i + bonesCount * 4; i < nn; i += 4)
                {
                    vertices[i] = readFloat(self);
                    vertices[i + 1] = readFloat(self);
                    vertices[i + 2] = readFloat(self);
                    vertices[i + 3] = readFloat(self);
                }
            }


            mesh->weights = CALLOC(float, mesh->weightsCount);
            mesh->bones = CALLOC(int, mesh->bonesCount);
            for (i = 0, b = 0, w = 0; i < verticesCount;)
            {
                int bonesCount = (int)vertices[i++];
                mesh->bones[b++] = bonesCount;
                for (nn = i + bonesCount * 4; i < nn; i += 4, ++b, w += 3)
                {
                    mesh->bones[b] = (int)vertices[i];
                    mesh->weights[w] = vertices[i + 1] * scale;
                    mesh->weights[w + 1] = vertices[i + 2] * scale;
                    mesh->weights[w + 2] = vertices[i + 3];
                }
            }
            
            FREE(vertices);
            readColor(self, &mesh->r, &mesh->g, &mesh->b, &mesh->a);
            mesh->hullLength = readIntOptimize(self) * 2;

            spSkinnedMeshAttachment_updateUVs(mesh);

            return SUPER_CAST(spAttachment, mesh);
        }
    }

    return NULL;
}

static spSkin *readSkin(spSkeletonBinary *self, const char *skinName, int *nonessential)
{
    spSkin *skin;
    int i;

    int slotCount = readIntOptimize(self);

    skin = spSkin_create(skinName);
    for (i = 0; i < slotCount; i++)
    {
        int ii, nn;
        int slotIndex = readIntOptimize(self);
        for (ii = 0, nn = readIntOptimize(self); ii < nn; ii++)
        {
            const char *name = readString(self);
            spSkin_addAttachment(skin, slotIndex, name, readAttachment(self, skin, name));
        }
    }

    return skin;
}

static void readCurve(spSkeletonBinary *self, spCurveTimeline *timeline, int frameIndex)
{
    spCurveType type = (spCurveType)readChar(self);
    if (type == SP_CURVE_STEPPED)
    {
        spCurveTimeline_setStepped(timeline, frameIndex);
    }
    else if (type == SP_CURVE_BEZIER)
    {
        float v1 = readFloat(self);
        float v2 = readFloat(self);
        float v3 = readFloat(self);
        float v4 = readFloat(self);
        spCurveTimeline_setCurve(timeline, frameIndex, v1, v2, v3, v4);
    }
} 

static void readAnimation(spSkeletonBinary *self, spSkeletonData *skeletonData, const char *name)
{ 
    int i, ii, n, nn;
    float scale = self->scale;
    spAnimation *animation = spAnimation_create(name, 1024 * 5);
    
    animation->timelinesCount = 0;

    // Slot timelines
    n = readIntOptimize(self);
    for (i = 0; i < n; i++)
    {
        int slotIndex = readIntOptimize(self);
        nn = readIntOptimize(self);
        for (ii = 0; ii < nn; ii++)
        {
            int frameIndex;
            int timelineType = readChar(self);
            int framesCount = readIntOptimize(self);
            if (timelineType == 3) {
                timelineType = 4;
            } else if (timelineType == 4) {
                timelineType = 3;
            }
            switch (timelineType)
            {
                case SP_TIMELINE_COLOR:
                {
                    spColorTimeline *timeline = spColorTimeline_create(framesCount);
                    timeline->slotIndex = slotIndex;
                    for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
                    {
                        float time = readFloat(self);
                        
                        float r = READ() / (float)255;
                        float g = READ() / (float)255;
                        float b = READ() / (float)255;
                        float a = READ() / (float)255;
                        spColorTimeline_setFrame(timeline, frameIndex, time, r, g, b, a);
                        if (frameIndex < framesCount - 1)
                            readCurve(self, SUPER(timeline), frameIndex);
                    }
                    animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
                    animation->duration = MAX(animation->duration, timeline->frames[framesCount * 5 - 5]);
                    break;
                }
                case SP_TIMELINE_ATTACHMENT:
                {
                    spAttachmentTimeline *timeline = spAttachmentTimeline_create(framesCount);
                    timeline->slotIndex = slotIndex;
                    for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
                    {
                        float time = readFloat(self);
                        spAttachmentTimeline_setFrame(timeline, frameIndex, time, readString(self));
                    }
                    animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
                    animation->duration = MAX(animation->duration, timeline->frames[framesCount - 1]);
                    break;
                }
            }
        }
    }

    // Bone timelines.
    n = readIntOptimize(self);
    for (i = 0; i < n; i++)
    {
        int boneIndex = readIntOptimize(self);
        nn = readIntOptimize(self);
        for (ii = 0; ii < nn; ii++)
        {
            int frameIndex;
            spTimelineType timelineType = (spTimelineType)readChar(self);
            int framesCount = readIntOptimize(self);
            if (timelineType == SP_TIMELINE_EVENT) {
                timelineType = SP_TIMELINE_FLIPX;
            } else if (timelineType == SP_TIMELINE_DRAWORDER) {
                timelineType = SP_TIMELINE_FLIPY;
            }
            
            switch (timelineType)
            {
                case SP_TIMELINE_ROTATE:
                {
                    spRotateTimeline *timeline = spRotateTimeline_create(framesCount);
                    timeline->boneIndex = boneIndex;
                    for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
                    {
                        float time = readFloat(self);
                        float angle = readFloat(self);
                        spRotateTimeline_setFrame(timeline, frameIndex, time, angle);
                        if (frameIndex < framesCount - 1)
                            readCurve(self, SUPER(timeline), frameIndex);
                    }
                    animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
                    animation->duration = MAX(animation->duration, timeline->frames[framesCount * 2 - 2]);
                    break;
                }
                case SP_TIMELINE_TRANSLATE:
                case SP_TIMELINE_SCALE:
                {
                    spTranslateTimeline *timeline;
                    float timelineScale = 1;
                    if (timelineType == SP_TIMELINE_SCALE)
                    {
                        timeline = spScaleTimeline_create(framesCount);
                    }
                    else
                    {
                        timeline = spTranslateTimeline_create(framesCount);
                        timelineScale = scale;
                    }
                    timeline->boneIndex = boneIndex;
                    for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
                    {
                        float time = readFloat(self);
                        float x = readFloat(self) * timelineScale;
                        float y = readFloat(self) * timelineScale;
                        spTranslateTimeline_setFrame(timeline, frameIndex, time, x, y);
                        if (frameIndex < framesCount - 1)
                            readCurve(self, SUPER(timeline), frameIndex);
                    }
                    animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
                    animation->duration = MAX(animation->duration, timeline->frames[framesCount * 3 - 3]);
                    break;
                }
                case SP_TIMELINE_FLIPX:
                case SP_TIMELINE_FLIPY:
                {
                    spFlipTimeline *timeline = spFlipTimeline_create(framesCount, timelineType == SP_TIMELINE_FLIPX);
                    timeline->boneIndex = boneIndex;
                    for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
                    {
                        float time = readFloat(self);
                        spFlipTimeline_setFrame(timeline, frameIndex, time, readBoolean(self));
                    }
                    animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
                    animation->duration = MAX(animation->duration, timeline->frames[framesCount * 2 - 2]);
                    break;
                }
                default:
                {
                    printf("unknow timelineType :%d", timelineType);
                }
                    break;
            }
        }
    }
    
    //ik timelines
    n = readIntOptimize(self);
    for (i = 0; i < n; i++)
    {
        int frameIndex;
        int index = readIntOptimize(self);
        int framesCount = readIntOptimize(self);
        spIkConstraintTimeline *timeline = spIkConstraintTimeline_create(framesCount);
        timeline->ikConstraintIndex = index;
        for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
        {
            float time = readFloat(self);
            float mix = readFloat(self);
            int bendPositive = READ() == 1 ? 1 : -1;
            spIkConstraintTimeline_setFrame(timeline, frameIndex, time, mix, bendPositive);
            if (frameIndex < framesCount - 1)
                readCurve(self, SUPER(timeline), frameIndex);
        }
        animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
        animation->duration = MAX(animation->duration, timeline->frames[framesCount * 3 - 3]);
    }

    // FFD timelines
    n = readIntOptimize(self);
    for (i = 0; i < n; i++)
    {
        spSkin *skin = skeletonData->skins[readIntOptimize(self)];
        nn = readIntOptimize(self);
        for (ii = 0; ii < nn; ii++)
        {
            int slotIndex = readIntOptimize(self);
            int iii, nnn;
            nnn = readIntOptimize(self);
            for (iii = 0; iii < nnn; iii++)
            {
                int frameIndex = 0;
                int framesCount = 0;
                int verticesCount = 0;         
                float *tempVertices;
    
                spFFDTimeline *timeline;

                spAttachment *attachment = spSkin_getAttachment(skin, slotIndex, readString(self));
                if (attachment->type == SP_ATTACHMENT_MESH)
                    verticesCount = SUB_CAST(spMeshAttachment, attachment)->verticesCount;
                else if (attachment->type == SP_ATTACHMENT_SKINNED_MESH)
                    verticesCount = SUB_CAST(spSkinnedMeshAttachment, attachment)->weightsCount / 3 * 2;
                else
                    printf("%d type \n", attachment->type);

                framesCount = readIntOptimize(self);
                timeline = spFFDTimeline_create(framesCount, verticesCount);
                timeline->slotIndex = slotIndex;
                timeline->attachment = attachment;

                tempVertices = MALLOC(float, verticesCount);
                for (frameIndex = 0; frameIndex < framesCount; frameIndex++)
                {
                    float *frameVertices;
                    float time = readFloat(self);
//                    int start = readIntOptimize(self);
                    int end = readIntOptimize(self);

                    if (end == 0)
                    {
                        if (attachment->type == SP_ATTACHMENT_MESH)
                            frameVertices = SUB_CAST(spMeshAttachment, attachment)->vertices;
                        else {
                            frameVertices = tempVertices;
                            memset(frameVertices, 0, sizeof(float) * verticesCount);
                        }
                    }
                    else
                    {
                        int v,start;
                        frameVertices = tempVertices;
                        start = readIntOptimize(self);
						memset(frameVertices, 0, sizeof(float) * start);
                        end += start;
                        if (scale == 1)
                        {
                            for (v = start; v < end; v++)
                                frameVertices[v] = readFloat(self);
                        }
                        else
                        {
                            for (v = start; v < end; v++)
                                frameVertices[v] = readFloat(self) * scale;
                        }
                        memset(frameVertices + v, 0, sizeof(float) * (verticesCount - v));
                        if (attachment->type == SP_ATTACHMENT_MESH) 
                        {
                            float *meshVertices = SUB_CAST(spMeshAttachment, attachment)->vertices;
                            for (v = 0; v < verticesCount; ++v)
                            {
                                frameVertices[v] += meshVertices[v];
                                if (frameVertices[v] > 1000 || frameVertices[v] < - 1000) {
                                    printf("note vertice %f \n", frameVertices[v]);
                                    frameVertices[v] = meshVertices[v];
                                }
                            }
                        }
                    }

                    spFFDTimeline_setFrame(timeline, frameIndex, time, frameVertices);
                    if (frameIndex < framesCount - 1)
                        readCurve(self, SUPER(timeline), frameIndex);
                }
                FREE(tempVertices);
                animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
                animation->duration = MAX(animation->duration, timeline->frames[framesCount - 1]);
            }
        }
    }

    // Draw order timeline
    n = readIntOptimize(self);
    if (n > 0)
    {
        int* drawOrder = 0;
        spDrawOrderTimeline *timeline = spDrawOrderTimeline_create(n, skeletonData->slotsCount);
        int slotCount = skeletonData->slotsCount;
        int frameIndex;
        for (frameIndex = 0; frameIndex < n; frameIndex++)
        {
            int originalIndex = 0, unchangedIndex = 0;
            int offsetCount = readIntOptimize(self);
            int *unchanged = MALLOC(int, skeletonData->slotsCount - offsetCount);
            drawOrder = MALLOC(int, skeletonData->slotsCount);
            for (ii = slotCount - 1; ii >= 0; ii--)
                drawOrder[ii] = -1;
            for (ii = 0; ii < offsetCount; ii++)
            {
                int slotIndex = readIntOptimize(self);
                while (originalIndex != slotIndex)
                    unchanged[unchangedIndex++] = originalIndex++;
                drawOrder[originalIndex + readIntOptimize(self)] = originalIndex;
                originalIndex++;
            }
            while (originalIndex < slotCount)
                unchanged[unchangedIndex++] = originalIndex++;
            for (ii = slotCount - 1; ii >= 0; ii--)
                if (drawOrder[ii] == -1) drawOrder[ii] = unchanged[--unchangedIndex];
            FREE(unchanged);
            spDrawOrderTimeline_setFrame(timeline, frameIndex, readFloat(self), drawOrder);
            FREE(drawOrder);
        }
        animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
        animation->duration = MAX(animation->duration, timeline->frames[n - 1]);
    }

    // Event timeline.
    n = readIntOptimize(self);
    if (n > 0)
    {
        spEventTimeline *timeline = spEventTimeline_create(n);
        int frameIndex;
        for (frameIndex = 0; frameIndex < n; frameIndex++)
        {
            spEvent *event;
            const char *stringValue;
            float time = readFloat(self);
            spEventData *eventData = skeletonData->events[readIntOptimize(self)];
            event = spEvent_create(eventData);
            event->intValue = readIntOptimize(self);
            event->floatValue = readFloat(self);
            stringValue = readBoolean(self) ? readString(self) : eventData->stringValue;
            if (stringValue) MALLOC_STR(event->stringValue, stringValue);
            spEventTimeline_setFrame(timeline, frameIndex, time, event);
        }
        animation->timelines[animation->timelinesCount++] = SUPER_CAST(spTimeline, timeline);
        animation->duration = MAX(animation->duration, timeline->frames[n - 1]);
    }

    skeletonData->animations[skeletonData->animationsCount++] = animation;
}

spSkeletonBinary* spSkeletonBinary_createWithLoader (spAttachmentLoader* attachmentLoader)
{
	spSkeletonBinary* self = SUPER(NEW(_spSkeletonBinary));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

spSkeletonBinary* spSkeletonBinary_create(spAtlas* atlas)
{
	spAtlasAttachmentLoader* attachmentLoader = spAtlasAttachmentLoader_create(atlas);
	spSkeletonBinary* self = spSkeletonBinary_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_spSkeletonBinary, self)->ownsLoader = 1;
	return self;
}

spSkeletonData* spSkeletonBinary_readSkeletonData( spSkeletonBinary* self )
{
	int size, i, nonessential;
	const char* buff;
	spSkeletonData *skeletonData;
	float scale = self->scale;
	spSkin *defaultSkin;

	skeletonData = spSkeletonData_create();

	// Header
	if ((buff = readString(self)) != NULL) 
		MALLOC_STR(skeletonData->hash, buff);
	if ((buff = readString(self)) != NULL) 
		MALLOC_STR(skeletonData->version, buff);
	skeletonData->width = readFloat(self);
	skeletonData->height = readFloat(self);
	
	nonessential = readBoolean(self);
	if (nonessential)
	{

	}
	// Bones
	size = readIntOptimize(self);
	skeletonData->bones = MALLOC(spBoneData *, size);
	for (i = 0; i < size; i++)
	{
		spBoneData *parent = NULL;
		spBoneData *boneData;
		int parentIndex;
		const char *name; 
		
		name = readString(self);
//		printf("bone name : %s\n", name);
		parentIndex = readIntOptimize(self) - 1;
		if (parentIndex != -1) parent = skeletonData->bones[parentIndex];
		boneData = spBoneData_create(name, parent);
		boneData->x = readFloat(self) * scale;
		boneData->y = readFloat(self) * scale;
		boneData->scaleX = readFloat(self);
		boneData->scaleY = readFloat(self);
		boneData->rotation = readFloat(self);
		boneData->length = readFloat(self) * scale;
		boneData->flipX = readBoolean(self);
		boneData->flipY = readBoolean(self);
		boneData->inheritScale = readBoolean(self);
		boneData->inheritRotation = readBoolean(self);
		skeletonData->bones[i] = boneData;
		if (nonessential)
		{
			//Color.rgba8888ToColor(boneData.color, input.readInt());
		}
		++skeletonData->bonesCount;
	}

	//ik
	size = readIntOptimize(self);
	skeletonData->ikConstraints = MALLOC(spIkConstraintData *, size);
	for (i = 0; i < size; i++)
	{
		int n;
		spIkConstraintData *ik = spIkConstraintData_create(readString(self));
		int boneCount = readIntOptimize(self);
//        printf("ik : %d\n", boneCount);
		ik->bones = MALLOC(spBoneData *, boneCount);
		for (n = 0; n < boneCount; n++)
		{
			ik->bones[ik->bonesCount ++] = skeletonData->bones[readIntOptimize(self)];
//            printf("ik bone name : %s\n", ik->bones[n]->name);
		}
		ik->target = skeletonData->bones[readIntOptimize(self)];
		ik->mix = readFloat(self);
		ik->bendDirection =  READ() == 1 ? 1 : -1;
        skeletonData->ikConstraints[skeletonData->ikConstraintsCount ++] = ik;
	}

	// Slots
	size = readIntOptimize(self);
	if (size > 0)
	{
		skeletonData->slots = MALLOC(spSlotData *, size);
		for (i = 0; i < size; i++) {
			const char *attachment;
			spBoneData *boneData;
			spSlotData *slotData;

			const char *name = readString(self);
			boneData = skeletonData->bones[readIntOptimize(self)];
			slotData = spSlotData_create(name, boneData);
			readColor(self, &slotData->r, &slotData->g, &slotData->b, &slotData->a);
			attachment = readString(self);
			if (attachment) spSlotData_setAttachmentName(slotData, attachment);

			slotData->blendMode = (spBlendMode)READ();

			skeletonData->slots[i] = slotData;
			++skeletonData->slotsCount;
		}
	}
    
    // Default skin
    defaultSkin = readSkin(self, "default", &nonessential);
    if (defaultSkin != NULL)
    {
        skeletonData->defaultSkin = defaultSkin;
        skeletonData->skinsCount ++;
    }
    
	// user skin
	size = readIntOptimize(self);
    
	// Skins
	if (size > 0)
	{
		skeletonData->skins = MALLOC(spSkin *, size + skeletonData->skinsCount);
        if (defaultSkin != NULL)
        {
            skeletonData->skins[0] = defaultSkin;
        }

        for (i = skeletonData->skinsCount; i < size + skeletonData->skinsCount; i++)
		{
			const char *name = readString(self);
			spSkin *skin = readSkin(self, name, &nonessential);
			skeletonData->skins[skeletonData->skinsCount] = skin;
			++skeletonData->skinsCount;
		}
    }
    else
    {
        if (defaultSkin != NULL)
        {
            skeletonData->skins = MALLOC(spSkin *, 1);
            skeletonData->skins[0] = defaultSkin;
        }
    }

	// Events
	size = readIntOptimize(self);
	if (size > 0)
	{
		const char *stringValue;
		skeletonData->events = MALLOC(spEventData *, size);
		for (i = 0; i < size; i++)
		{
			spEventData *eventData = spEventData_create(readString(self));
			eventData->intValue = readIntOptimize(self);
			eventData->floatValue = readFloat(self);
			stringValue = readString(self);
			if (stringValue) MALLOC_STR(eventData->stringValue, stringValue);
			skeletonData->events[skeletonData->eventsCount++] = eventData;
		}
	}

	// Animations
	size = readIntOptimize(self);
	if (size > 0)
	{
		skeletonData->animations = MALLOC(spAnimation *, size);
		for (i = 0; i < size; i++)
		{
			const char *name = readString(self);
			readAnimation(self, skeletonData, name);
		}
	}

	return skeletonData;
}

spSkeletonData *spSkeletonBinary_readSkeletonDataFile(spSkeletonBinary* self, const char * path)
{
	int length;
	spSkeletonData* skeletonData;

	self->rawdata = _spUtil_readFile(path, &length);
	self->reader = self->rawdata;
    self->cache = (char**)malloc(sizeof(char*) * 1024 * 5);
    self->cacheIndex = 0;
	if (!self->reader) {
		return 0;
	}

	skeletonData = spSkeletonBinary_readSkeletonData(self);
	
    FREE(self->cache);
	FREE(self->rawdata);
	return skeletonData;
}

void spSkeletonBinary_dispose (spSkeletonBinary* self)
{
	if (SUB_CAST(_spSkeletonBinary, self)->ownsLoader) spAttachmentLoader_dispose(self->attachmentLoader);
	FREE(self);
}