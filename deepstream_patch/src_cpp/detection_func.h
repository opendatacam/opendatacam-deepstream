#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "nvdsmeta.h"

float* cvt2yolo(unsigned int f_width, unsigned int f_height, int left, int top, int right, int bottom);

char *detection_to_json(NvDsFrameMeta *frame_meta);

#ifdef __cplusplus
}
#endif
