#include "detection_func.h"

// just for test
#include <iostream>

/// convert to YOLO format :

float* cvt2yolo(unsigned int f_width, unsigned int f_height, int left, int top, int right, int bottom){

	float dw = 1.0 / f_width;
	float dh = 1.0 / f_height;

	// (xmin + xmax / 2)
	float x = (left + right) / 2.0;
	// (ymin + ymax / 2)
	float y = (top + bottom) / 2.0;

	// (xmax - xmin) = w
	float w = abs(left-right);
	float h = abs(top-bottom);
	
	float *r = (float *)calloc(4, sizeof(float));
	r[0] = x * dw;
	r[2] = w * dw;
	r[1] = y * dh;
	r[3] = h * dh;

	
	return r;
}

// JSON format:
//{
// "frame_id":8990,
// "objects":[
//  {"class_id":4, "name":"aeroplane", "relative coordinates":{"center_x":0.398831, "center_y":0.630203, "width":0.057455, "height":0.020396}, "confidence":0.793070},
//  {"class_id":14, "name":"bird", "relative coordinates":{"center_x":0.398831, "center_y":0.630203, "width":0.057455, "height":0.020396}, "confidence":0.265497}
// ]
//},

char *detection_to_json(NvDsFrameMeta *frame_meta)
{

    char *send_buf = (char *)calloc(1024, sizeof(char));

    sprintf(send_buf, "{\n \"frame_id\":%lld, \n \"objects\": [ \n", frame_meta->frame_num);

		bool entred_for = false;
    for (NvDsMetaList * l_obj = frame_meta->obj_meta_list; l_obj != NULL;
        l_obj = l_obj->next) {
			entred_for = true;      

			NvDsObjectMeta *obj = (NvDsObjectMeta *) l_obj->data;
      
      int left = obj->rect_params.left;
      int top = obj->rect_params.top;
      int right = left + obj->rect_params.width;
      int bottom = top + obj->rect_params.height;
      //guint64 id = obj->object_id;
      
      // convert bbox to darknet format :
      float* bbox =  cvt2yolo(frame_meta->source_frame_width, frame_meta->source_frame_height, left, top, right, bottom);
      
      /*
      // in case of an object with multiple labels  ---> SOLVE confidence = 0  ????????????
      gfloat best_prob = 0.0;
      gchar* best_label;
      unsigned int best_class_id = 0;
      // in case of an object with multiple labels  ---> SOLVE confidence = 0  ????????????
      for (NvDsLabelInfoList *l_obj_class_meta = obj->classifier_meta_list; l_obj_class_meta != NULL;
          l_obj_class_meta = l_obj_class_meta->next){
	NvDsClassifierMeta *obj_class_meta = (NvDsClassifierMeta *)l_obj_class_meta->data;
	for (NvDsLabelInfoList *obj_label = obj_class_meta->label_info_list; obj_label != NULL;
            obj_label = obj_label->next){
  	  NvDsLabelInfo *lab_info = (NvDsLabelInfo *)obj_label->data;
	  std::cout <<"prob = " << lab_info->result_prob << std::endl;
	  if (lab_info->result_prob > best_prob){
	    best_prob = lab_info->result_prob;
	    best_label = lab_info->result_label;
	    best_class_id = lab_info->result_class_id;
	  }
	}
      }
      */
      //darknet Format :

      char *buf = (char *)calloc(2048, sizeof(char));
      sprintf(buf, "  {\"class_id\":%d, \"name\":\"%s\", \"relative_coordinates\":{\"center_x\":%f, \"center_y\":%f, \"width\":%f, \"height\":%f}, \"confidence\":%f} ,",
          obj->class_id, obj->obj_label, bbox[0], bbox[1], bbox[2], bbox[3], obj->confidence);

      int send_buf_len = strlen(send_buf);
      int buf_len = strlen(buf);
      int total_len = send_buf_len + buf_len + 100;
      send_buf = (char *)realloc(send_buf, total_len * sizeof(char));
      if (!send_buf) return 0;// exit(-1);
      strcat(send_buf, buf);
      free(buf);
    
      free(bbox);
    }
		// delete the last ',' 
		if (entred_for==true){
			int send_buf_len = strlen(send_buf);
			send_buf[send_buf_len-2]='\0';
		}

    strcat(send_buf, "\n ] \n}");
    return send_buf;
}



