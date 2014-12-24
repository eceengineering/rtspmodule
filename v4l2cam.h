#ifndef V4L2CAM_H_
#define V4L2CAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include 	<linux/videodev2.h>
#define 	V4L2_MAX_BUFFER_COUNT 4

struct capture_info 
{
	int 	width;
	int 	height;
	int 	fd;
	char 	*device_name;
	char 	*userptr[V4L2_MAX_BUFFER_COUNT];
	struct v4l2_buffer v4l2buf[V4L2_MAX_BUFFER_COUNT];
};

int init_camera		(struct capture_info *cinfo);
int start_camera	(struct capture_info *cinfo);
int close_camera	(struct capture_info *cinfo);
int get_camera_frame	(struct capture_info *cinfo);
int put_camera_frame	(struct capture_info *cinfo, int buf_no);

#ifdef __cplusplus
}
#endif

#endif /* V4L2CAM_H_ */
