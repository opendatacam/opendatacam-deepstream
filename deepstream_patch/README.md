# Install deepstream

●	You'll need Jetpack 4.4 to run Deepstream 5.0, get it [here](https://developer.nvidia.com/embedded/jetpack)

●	Download Deepstream 5.0 from this [link](https://developer.nvidia.com/deepstream-download) and make sure to get debian file for jetson

●	Install deepstream

    sudo apt-get update

    sudo apt-get install --fix-broken

    sudo apt install \
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

for issues [link here](https://docs.nvidia.com/metropolis/deepstream/dev-guide/index.html)


# FFMPEG MJPEG workaround

●	Install ffmpeg

    sudo apt-get install ffmpeg

●	Patch ffserver

    sudo cp -r <PATH_TO_OPENDATACAM>/deepstream_patch/ffserver.conf /etc/ffserver.conf


# YOLO and Deepstream 5

●	Patch DeepStream : copy detection module

	cp -rv <PATH_TO_OPENDATACAM>/deepstream_patch/deepstream_app_config_yoloV3_tracker_rtsp.txt /opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/
	cd /opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/
	export CUDA_VER=10.2
	make -C nvdsinfer_custom_impl_Yolo

*Download '.cfg' and '.weights' files using 'prebuild.sh' (e.g: yolov3-tiny) :*

	echo "Downloading yolov3-tiny config and weights files ... "
	wget https://raw.githubusercontent.com/pjreddie/darknet/master/cfg/yolov3.cfg -q --show-progress
	wget https://pjreddie.com/media/files/yolov3.weights -q --show-progress

●	Patch DeepStream : modify main application

*Add the folowing lines to file : /opt/nvidia/deepstream/deepstream-5.0/sources/apps/sample_apps/deepstream-app/deepstream_app.h*

	#include "detection_func.h"
	#include "http_stream.h"

*Add the folowing lines to file : /opt/nvidia/deepstream/deepstream-5.0/sources/apps/sample_apps/deepstream-app/deepstream_app.c*

	#define JSON_PORT 8070
	#define JSON_TIMEOUT 400000

*Replace in the same file "write_kitti_track_output" function with:*

	static void
	write_kitti_track_output (AppCtx * appCtx, NvDsBatchMeta * batch_meta)
	{
		gchar bbox_file[1024] = { 0 };
		FILE *bbox_params_dump_file = NULL;

		if (!appCtx->config.kitti_track_dir_path)
			return;

		for (NvDsMetaList * l_frame = batch_meta->frame_meta_list; l_frame != NULL;
			l_frame = l_frame->next) {
			NvDsFrameMeta *frame_meta = l_frame->data;
			send_json(frame_meta, JSON_PORT, JSON_TIMEOUT);
		}
	}

*Change line 598 of file = \opt\nvidia\deepstream\deepstream-5.0\sources\apps\apps-common\src\deepstream_sink_bin.c    from :*

	g_object_set (G_OBJECT (bin->sink), "host", "224.224.255.255", "port",
		config->udp_port, "async", FALSE, "sync", 0, NULL);
*to :*

	g_object_set (G_OBJECT (bin->sink), "host", "127.0.0.1", "port",
		config->udp_port, "async", FALSE, "sync", 0, NULL);

Run command:

	cp -r <PATH_TO_OPENDATACAM>/deepstream_patch/src_cpp/ /opt/nvidia/deepstream/deepstream-5.0/sources/apps/sample_apps/deepstream-app/

Replace lines after "all: $(APP)" (included) in "/opt/nvidia/deepstream/deepstream-5.0/sources/apps/sample_apps/deepstream-app/Makefile" with this :

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

	follow README file in /opt/nvidia/deepstream/deepstream-5.0/sources/apps/sample_apps/deepstream-app/


●	Run deepstream to generate tensorRT engine

	cd /opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/
	../apps/sample_apps/deepstream-app/deepstream-app2 -c /opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/deepstream_app_config_yoloV3_tiny_http_rtsp.txt

*Expected output (press Q after the build is completed to stop deepstream-app):*


 	*** DeepStream: Launched RTSP Streaming at rtsp://localhost:8020/ds-test ***

	Opening in BLOCKING MODE
	gstnvtracker: Loading low-level lib at /opt/nvidia/deepstream/deepstream-5.0/lib/libnvds_mot_klt.so
	gstnvtracker: Optional NvMOT_RemoveStreams not implemented
	gstnvtracker: Batch processing is OFF
	WARNING: [TRT]: Using an engine plan file across different models of devices is not recommended and is likely to affect performance or even cause errors.
	Deserialize yoloLayerV3 plugin: yolo_83
	Deserialize yoloLayerV3 plugin: yolo_95
	Deserialize yoloLayerV3 plugin: yolo_107
	0:00:09.209912402 30321   0x557ecc4b00 INFO                 nvinfer gstnvinfer.cpp:602:gst_nvinfer_logger:<primary_gie> NvDsInferContext[UID 1]: Info from NvDsInferContextImpl::deserializeEngineAndBackend() <nvdsinfer_context_impl.cpp:1577> [UID = 1]: deserialized trt engine from :/opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/model_b1_gpu0_fp16_608.engine
	INFO: [Implicit Engine Info]: layers num: 4
	0   INPUT  kFLOAT data            3x608x608
	1   OUTPUT kFLOAT yolo_83         255x19x19
	2   OUTPUT kFLOAT yolo_95         255x38x38
	3   OUTPUT kFLOAT yolo_107        255x76x76

	0:00:09.210148863 30321   0x557ecc4b00 INFO                 nvinfer gstnvinfer.cpp:602:gst_nvinfer_logger:<primary_gie> NvDsInferContext[UID 1]: Info from NvDsInferContextImpl::generateBackendContext() <nvdsinfer_context_impl.cpp:1681> [UID = 1]: Use deserialized engine model: /opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/model_b1_gpu0_fp16_608.engine
	0:00:09.333041172 30321   0x557ecc4b00 INFO                 nvinfer gstnvinfer_impl.cpp:311:notifyLoadModelStatus:<primary_gie> [UID 1]: Load new model:/opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/config_infer_primary_yoloV3.txt sucessfully

	Runtime commands:
		h: Print this help
		q: Quit

		p: Pause
		r: Resume

	NOTE: To expand a source in the 2D tiled display and view object details, left-click on the source.
		To go back to the tiled display, right-click anywhere on the window.


	**PERF: FPS 0 (Avg)
	**PERF: 0.00 (0.00)
	** INFO: <bus_callback:183>: Pipeline ready

	Opening in BLOCKING MODE
	NvMMLiteOpen : Block : BlockType = 261
	NVMEDIA: Reading vendor.tegra.display-size : status: 6
	NvMMLiteBlockCreate : Block : BlockType = 261
	** INFO: <bus_callback:169>: Pipeline running

	NvMMLiteOpen : Block : BlockType = 4
	===== NVMEDIA: NVENC =====
	NvMMLiteBlockCreate : Block : BlockType = 4
	KLT Tracker Init
	H264: Profile = 66, Level = 0
	**PERF: 8.45 (8.01)
	**PERF: 7.98 (7.89)
	**PERF: 7.96 (8.07)
	q
	Quitting
	App run successful

You can also change the model input size to get ~14 fps.

●	After this you should be able to find an optimised tensorrt engine called `model_b1_gpu0_fp16.engine` in `/opt/nvidia/deepstream/deepstream-5.0/sources/objectDetector_Yolo/`. Notice that in the configfile `deepstream_app_config_yoloV3_tiny_http_rtsp.txt` we point to `model_b1_gpu0_fp16_608.engine` as the engine file, so either rename the file or change the configfile to reflect the different engine file.

●	Follow [Opendatacam without docker](https://github.com/opendatacam/opendatacam/blob/master/documentation/USE_WITHOUT_DOCKER.md) to install Opendatacam and DON'T install darknet
