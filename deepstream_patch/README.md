

●	Install ffmpeg

    sudo apt-get install ffmpeg

●	Patch ffserver

    sudo cp -r <PATH_TO_OPENDATACAM>/deepstream_patch/ffserver.conf /etc/ffserver.conf
    
●	Download Deepstream from this [link](https://developer.nvidia.com/deepstream-download) and make sure to get debian file for jetson

●	Install deepstream

    sudo apt-get update

    sudo apt-get install --fix-broken

    sudo apt-get install \
       	libssl1.0.0 \
    	libgstreamer1.0-0 \
    	gstreamer1.0-tools \
    	gstreamer1.0-plugins-good \
   	gstreamer1.0-plugins-bad \
    	gstreamer1.0-plugins-ugly \
    	gstreamer1.0-libav \
    	libgstrtspserver-1.0-0 \
    	libjansson4=2.11-1

    sudo dpkg -i <deepstream debian file>

●	Patch DeepStream : add confidence value to Deepstream inference objects (for more details refer to [link](https://devtalk.nvidia.com/default/topic/1058661/deepstream-sdk/nvinfer-is-not-populating-confidence-field-in-nvdsobjectmeta-ds-4-0-/post/5373361/#5373361) )

Open file: /opt/nvidia/deepstream/deepstream-4.0/sources/includes/nvdsinfer_context.h

*Replace "NvDsInferObject" structure with the following :*

	typedef struct
	{
		/** Offset from the left boundary of the frame. */
		unsigned int left;
		/** Offset from the top boundary of the frame. */
		unsigned int top;
		/** Object width. */
		unsigned int width;
		/** Object height. */
		unsigned int height;
		/* Index for the object class. */
		int classIndex;
		/* String label for the detected object. */
		char *label;
		/* Object confidence */
		float confidence;
	} NvDsInferObject;

Open file: /opt/nvidia/deepstream/deepstream-4.0/sources/libs/nvdsinfer/nvdsinfer_context_impl_output_parsing.cpp

*Replace "clusterAndFillDetectionOutputCV" method with the following:*

	void
	NvDsInferContextImpl::clusterAndFillDetectionOutputCV(NvDsInferDetectionOutput &output)
	{
		output.objects = new NvDsInferObject[m_ObjectList.size()];
		output.numObjects = 0;

		for (unsigned int i = 0; i < m_ObjectList.size(); i++)
		{

			NvDsInferObject &object = output.objects[i];
			object.left = m_ObjectList[i].left;
			object.top = m_ObjectList[i].top;
			object.width = m_ObjectList[i].width;
			object.height = m_ObjectList[i].height;
			object.classIndex = m_ObjectList[i].classId;
			object.confidence = m_ObjectList[i].detectionConfidence;
			int c = object.classIndex;
			object.label = nullptr;
			if (c < m_Labels.size() && m_Labels[c].size() > 0)
						object.label = strdup(m_Labels[c][0].c_str());
			output.numObjects++;
		}
			
	}

Follow README file in /opt/nvidia/deepstream/deepstream-4.0/sources/libs/nvdsinfer/ to generate the new libraries: 
	
	make && sudo CUDA_VER=10.0 make install

Open file: /opt/nvidia/deepstream/deepstream-4.0/sources/gst-plugins/gst-nvinfer/gstnvinfer_meta_utils.cpp

*Replace Line 83 ("obj_meta->confidence = 0.0;")  with :*

	obj_meta->confidence = obj.confidence;

	
Follow README file in /opt/nvidia/deepstream/deepstream-4.0/sources/gst-plugins/gst-nvinfer/ to generate the new libraries : 

	make && sudo CUDA_VER=10.0 make install

●	Patch DeepStream : copy detection module 

	sudo cp -rv <PATH_TO_OPENDATACAM>/deepstream_patch/deepstream_app_config_yoloV3_tiny_http_rtsp.txt /opt/nvidia/deepstream/deepstream-4.0/sources/objectDetector_Yolo/
	cd /opt/nvidia/deepstream/deepstream-4.0/sources/objectDetector_Yolo/
	export CUDA_VER=10.0
	make -C nvdsinfer_custom_impl_Yolo

*Download '.cfg' and '.weights' files using 'prebuild.sh' (e.g: yolov3-tiny) :*

	echo "Downloading yolov3-tiny config and weights files ... "
	wget https://raw.githubusercontent.com/pjreddie/darknet/master/cfg/yolov3-tiny.cfg -q --show-progress
	wget https://pjreddie.com/media/files/yolov3-tiny.weights -q --show-progress
    
●	Patch DeepStream : modify main application 

*Add the folowing lines to file : /opt/nvidia/deepstream/deepstream-4.0/sources/apps/sample_apps/deepstream-app/deepstream_app.h*

	#include "detection_func.h"
	#include "http_stream.h"

*Add the folowing lines to file : /opt/nvidia/deepstream/deepstream-4.0/sources/apps/sample_apps/deepstream-app/deepstream_app.c*
	
	#define JSON_PORT 8070
	#define JSON_TIMEOUT 400000

*Replace in the same file "write_kitti_output" function with:*

	static void
	write_kitti_output (AppCtx * appCtx, NvDsBatchMeta * batch_meta)
	{
	  gchar bbox_file[1024] = { 0 };
	  FILE *bbox_params_dump_file = NULL;

	  if (!appCtx->config.bbox_dir_path)
		return;

	  for (NvDsMetaList * l_frame = batch_meta->frame_meta_list; l_frame != NULL;
		  l_frame = l_frame->next) {
		NvDsFrameMeta *frame_meta = l_frame->data;
		send_json(frame_meta, JSON_PORT, JSON_TIMEOUT);
	  }
	}

*Change line 538 of file = \opt\nvidia\deepstream\deepstream-4.0\sources\apps\apps-common\src\deepstream_sink_bin.c    from :*
	
	g_object_set (G_OBJECT (bin->sink), "host", "224.224.255.255", "port",
		config->udp_port, "async", FALSE, "sync", 0, NULL);
*to :*
	
	g_object_set (G_OBJECT (bin->sink), "host", "127.0.0.1", "port",
		config->udp_port, "async", FALSE, "sync", 0, NULL);

Run command:
	
	sudo cp -r <PATH_TO_OPENDATACAM>/deepstream_patch/src_cpp/ /opt/nvidia/deepstream/deepstream-4.0/sources/apps/sample_apps/deepstream-app/

Replace lines after "all: $(APP)" (included) in "/opt/nvidia/deepstream/deepstream-4.0/sources/apps/sample_apps/deepstream-app/Makefile" with this :

	CPPSRCDIR   = src_cpp

	SOURCES+= $(wildcard $(CPPSRCDIR)/*.cpp)

	INCLUDES+= $(wildcard $(CPPSRCDIR)/*.h)

	OBJS+= $(SOURCES:$(CPPSRCDIR)/%.cpp=$(CPPSRCDIR)/%.o)

	CFLAGS+= -I $(CPPSRCDIR)

	CXX      = g++
	CXXFLAGS +=  -Wall -ansi -g -std=c++11  -DPLATFORM_TEGRA
	CXXFLAGS+= -I../../apps-common/includes -I../../../includes -I $(CPPSRCDIR) -DDS_VERSION_MINOR=0 -DDS_VERSION_MAJOR=4

	CXXFLAGS+= `pkg-config --cflags $(PKGS)`


	all: $(APP)

	%.o: %.c $(INCS) Makefile
		$(CC) -c -o $@ $(CFLAGS) $<
	%.o: %.cpp $(INCS) Makefile
		$(CXX) -c -o $@ $(CXXFLAGS) $<


	$(APP): $(OBJS) Makefile
		$(CXX) -o $(APP) $(OBJS) $(LIBS)

	clean:
		rm -rf $(OBJS) $(APP)
	
Replace in the same file "APP:= deepstream-app" with "APP:= deepstream-app2"

●	Build Deepstream app

	follow README file in /opt/nvidia/deepstream/deepstream-4.0/sources/apps/sample_apps/deepstream-app/


●	Run deepstream to generate tensorRT engine 

	cd /opt/nvidia/deepstream/deepstream-4.0/sources/objectDetector_Yolo/
	../apps/sample_apps/deepstream-app/deepstream-app2 -c /opt/nvidia/deepstream/deepstream-4.0/sources/objectDetector_Yolo/deepstream_app_config_yoloV3_tiny_http_rtsp.txt

*Expected output (press Q after the build is completed to stop deepstream-app):*

	 *** DeepStream: Launched RTSP Streaming at rtsp://localhost:8020/ds-test ***

	Opening in BLOCKING MODE
	0:00:01.038002010  8142   0x557c4e2430 WARN                 nvinfer gstnvinfer.cpp:515:gst_nvinfer_logger:<primary_gie_classifier> NvDsInferContext[UID 1]:useEngineFile(): Failed to read from model engine file
	0:00:01.038094308  8142   0x557c4e2430 INFO                 nvinfer gstnvinfer.cpp:519:gst_nvinfer_logger:<primary_gie_classifier> NvDsInferContext[UID 1]:initialize(): Trying to create engine from model files
	Loading pre-trained weights...
	Loading complete!
	Total Number of weights read : 8858734
		layer               inp_size            out_size       weightPtr
	(1)   conv-bn-leaky     3 x 416 x 416      16 x 416 x 416    496
	(2)   maxpool          16 x 416 x 416      16 x 208 x 208    496
	(3)   conv-bn-leaky    16 x 208 x 208      32 x 208 x 208    5232
	(4)   maxpool          32 x 208 x 208      32 x 104 x 104    5232
	(5)   conv-bn-leaky    32 x 104 x 104      64 x 104 x 104    23920
	(6)   maxpool          64 x 104 x 104      64 x  52 x  52    23920
	(7)   conv-bn-leaky    64 x  52 x  52     128 x  52 x  52    98160
	(8)   maxpool         128 x  52 x  52     128 x  26 x  26    98160
	(9)   conv-bn-leaky   128 x  26 x  26     256 x  26 x  26    394096
	(10)  maxpool         256 x  26 x  26     256 x  13 x  13    394096
	(11)  conv-bn-leaky   256 x  13 x  13     512 x  13 x  13    1575792
	(12)  maxpool         512 x  13 x  13     512 x  13 x  13    1575792
	(13)  conv-bn-leaky   512 x  13 x  13    1024 x  13 x  13    6298480
	(14)  conv-bn-leaky  1024 x  13 x  13     256 x  13 x  13    6561648
	(15)  conv-bn-leaky   256 x  13 x  13     512 x  13 x  13    7743344
	(16)  conv-linear     512 x  13 x  13     255 x  13 x  13    7874159
	(17)  yolo            255 x  13 x  13     255 x  13 x  13    7874159
	(18)  route                  -            256 x  13 x  13    7874159
	(19)  conv-bn-leaky   256 x  13 x  13     128 x  13 x  13    7907439
	(20)  upsample        128 x  13 x  13     128 x  26 x  26        -
	(21)  route                  -            384 x  26 x  26    7907439
	(22)  conv-bn-leaky   384 x  26 x  26     256 x  26 x  26    8793199
	(23)  conv-linear     256 x  26 x  26     255 x  26 x  26    8858734
	(24)  yolo            255 x  26 x  26     255 x  26 x  26    8858734
	Output blob names :
	yolo_17
	yolo_24
	Total number of layers: 50
	Total number of layers on DLA: 0
	Building the TensorRT Engine...
	Building complete!
	0:00:57.905894974  8142   0x557c4e2430 INFO                 nvinfer gstnvinfer.cpp:519:gst_nvinfer_logger:<primary_gie_classifier> NvDsInferContext[UID 1]:generateTRTModel(): Storing the serialized cuda engine to file at /opt/nvidia/deepstream/deepstream-4.0/sources/objectDetector_Yolo/model_b1_fp32.engine
	Deserialize yoloLayerV3 plugin: yolo_17
	Deserialize yoloLayerV3 plugin: yolo_24

	Runtime commands:
			h: Print this help
			q: Quit

			p: Pause
			r: Resume

	NOTE: To expand a source in the 2D tiled display and view object details, left-click on the source.
		To go back to the tiled display, right-click anywhere on the window.


	**PERF: FPS 0 (Avg)
	**PERF: 0.00 (0.00)
	** INFO: <bus_callback:193>: Pipeline ready

	NvMMLiteOpen : Block : BlockType = 4
	** INFO: <bus_callback:179>: Pipeline running

	===== NVMEDIA: NVENC =====
	NvMMLiteBlockCreate : Block : BlockType = 4
	GST_ARGUS: Creating output stream
	CONSUMER: Waiting until producer is connected...
	GST_ARGUS: Available Sensor modes :
	GST_ARGUS: 2592 x 1944 FR = 15.000015 fps Duration = 66666600 ; Analog Gain range min 16.000000, max 128.000000; Exposure Range min 65000, max 60000000;

	GST_ARGUS: 1920 x 1080 FR = 29.999999 fps Duration = 33333334 ; Analog Gain range min 16.000000, max 128.000000; Exposure Range min 60000, max 30000000;

	GST_ARGUS: 1280 x 960 FR = 45.000000 fps Duration = 22222222 ; Analog Gain range min 16.000000, max 128.000000; Exposure Range min 43000, max 20000000;

	GST_ARGUS: 1280 x 720 FR = 59.999999 fps Duration = 16666667 ; Analog Gain range min 16.000000, max 128.000000; Exposure Range min 43000, max 10000000;

	GST_ARGUS: Running with following settings:
	Camera index = 0
	Camera mode  = 3
	Output Stream W = 1280 H = 720
	seconds to Run    = 0
	Frame Rate = 59.999999
	GST_ARGUS: PowerService: requested_clock_Hz=627200000
	GST_ARGUS: Setup Complete, Starting captures for 0 seconds
	GST_ARGUS: Starting repeat capture requests.
	CONSUMER: Producer has connected; continuing.
	H264: Profile = 66, Level = 0
	**PERF: 25.81 (25.81)
	**PERF: 25.75 (25.78)
	**PERF: 25.76 (25.77)
	**PERF: 25.72 (25.76)
	q
	Quitting
	GST_ARGUS: Cleaning up
	CONSUMER: Done Success
	GST_ARGUS: Done Success
	App run successful
	GST_ARGUS:
	PowerServiceHwVic::cleanupResources


●	Follow [Opendatacam without docker](https://github.com/opendatacam/opendatacam/blob/master/documentation/USE_WITHOUT_DOCKER.md) to install Opendatacam and DON'T install darknet 

