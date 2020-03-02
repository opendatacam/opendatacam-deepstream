#ifndef HTTP_STREAM_H
#define HTTP_STREAM_H

// to use the type ==> "NvDsInferSegmentationMeta"
#include "deepstream_primary_gie.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "detection_func.h"


void delete_json_sender();
void send_json_custom(char const* send_buf, int port, int timeout);

void send_json(NvDsFrameMeta *frame_meta, int port, int timeout);


#ifdef __cplusplus
}
#endif

#endif // HTTP_STREAM_H
