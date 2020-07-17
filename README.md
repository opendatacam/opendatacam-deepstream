
# Goal
The goal of this pull request is to replace Darknet with Deepstream SDK in order to [avoid darknet memory leak](https://github.com/opendatacam/opendatacam/issues/112).

# Deepstream SDK limitation
## Issue
Current version of Deepstream SDK (4.0.x) still [has confidence problem](https://devtalk.nvidia.com/default/topic/1058661/deepstream-sdk/nvinfer-is-not-populating-confidence-field-in-nvdsobjectmeta-ds-4-0-/post/5373361/#5373361) so in order to enable the object tracking in Opendatacam we need to patch this SDK to add publication of the confidence.

## Proposed solution
Note: This problem will be solved in Deepstream 5.0.
Until version 5.0 is out we have to manually patch the SDK; see [this readme](https://github.com/opendatacam/opendatacam-deepstream/tree/tracker_ds5/deepstream_patch)

# Other Tools in the Architecture
In order to connect the output of Deepstream (in RTSP) to MJPEG proxy of OpenDataCam we added 2 components :
- FFMPEG
- FFServer

## Credits
PoC developed by [Anas BAHOU](https://github.com/anasBahou) at [AtosInnovationBordeaux](https://github.com/AtosInnovationBordeaux) ==> [Pull request link](https://github.com/opendatacam/opendatacam-deepstream/pull/2)
