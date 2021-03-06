#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <queue>
#include <thread>
#include <mutex>          // std::mutex, std::lock_guard
#include <malloc.h>
#include <opencv2/opencv.hpp> 

#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <errno.h>

#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>

#define __STDC_CONSTANT_MACROS


#ifdef __cplusplus
extern "C"
{
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>

#ifdef __cplusplus
};
#endif

#include "encoder_src_filter.h"
#include "../util/CPictureFormatConversion.h"
#include "camera_src_filter.h"


//test different codec
#define TEST_H264  1
#define TEST_HEVC  0




/*
** 封装过程的数据源的过滤器

*/
using namespace std;
using namespace cv;





//#define TRUE            (1)
//#define FALSE           (0)

//#define FILE_VIDEO      "/dev/video0"
//#define IMAGE           "./img/demo"

//#define IMAGEWIDTH      640
//#define IMAGEHEIGHT     480

//#define FRAME_NUM       4

#define CLEAR(x) memset (&(x), 0, sizeof (x))


Ccamera_src_filter::Ccamera_src_filter(void)
{

    frame_index=0;
    io = IO_METHOD_MMAP;
    fd = -1;
    buffers = NULL;
    n_buffers = 0;
    dev_name = (char *)"/dev/video0";
     
    wid=0;
    height=0;

    

    filename = (char *) "test.yuv\0";
}
Ccamera_src_filter::~Ccamera_src_filter(void)
{

}

void Ccamera_src_filter::errno_exit(const char * s) {
	fprintf(stderr, "%s error %d, %s/n", s, errno, strerror(errno));
 
	exit(EXIT_FAILURE);
}


int Ccamera_src_filter::xioctl(int fd, int request, void * arg) {
	int r;
 
	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);
 
	return r;
}
int Ccamera_src_filter::read_frame(unsigned char * yuv420p_data) {
	struct v4l2_buffer buf;
	unsigned int i;


    fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* Timeout. */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

    while(1)
    {
        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if (-1 == r) {
            if (EINTR == errno)
            	continue;

            errno_exit("select");
        }

        if (0 == r) {
            fprintf(stderr, "select timeout/n");
            exit(EXIT_FAILURE);
            //continue;
        }

        break;
    }



	switch (io) {
	case IO_METHOD_READ:
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;
 
			case EIO:
				/* Could ignore EIO, see spec. */
				/* fall through */
			default:
				errno_exit("read");
			}
		}
 
		process_image(buffers[0].start, buffers[0].length);
 
		break;
 
	case IO_METHOD_MMAP:
		CLEAR(buf);
 
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
 
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
 
			case EIO:
				/* Could ignore EIO, see spec. */
 
				/* fall through */
 
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
 
		assert(buf.index < n_buffers);

        frame_index++;
        cout << "frame index:" <<frame_index << " lenth:" <<buf.length <<endl;
		//process_image(buffers[buf.index].start, buf.length);
		PictureFormatConversion::yuv422_to_yuv420p( wid,height,(unsigned char *)buffers[buf.index].start,yuv420p_data);
 
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
 
		break;
 
	case IO_METHOD_USERPTR:
		CLEAR(buf);
 
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;
 
		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
 
			case EIO:
				/* Could ignore EIO, see spec. */
 
				/* fall through */
 
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
 
		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long) buffers[i].start
					&& buf.length == buffers[i].length)
				break;
 
		assert(i < n_buffers);
 
		process_image((void *) buf.m.userptr, buf.length);
 
		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");
 
		break;
	}
 
	return 1;
}



void Ccamera_src_filter::process_image(const void * p, int size) {
	//fwrite(p, size, 1, fp);
}


void Ccamera_src_filter::stop_capturing(void) {
	enum v4l2_buf_type type;
 
	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
 
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");
 
		break;
	}
}
 
void Ccamera_src_filter::start_capturing(void) {
	unsigned int i;
	enum v4l2_buf_type type;
 
	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;
 
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;
 
			CLEAR(buf);
 
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;
 
			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
 
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
 
		break;
 
	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;
 
			CLEAR(buf);
 
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long) buffers[i].start;
			buf.length = buffers[i].length;
 
			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
 
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
 
		break;
	}
}
 
void Ccamera_src_filter::uninit_device(void) {
	unsigned int i;
 
	switch (io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;
 
	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;
 
	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free(buffers[i].start);
		break;
	}
 
	free(buffers);
}
 
void Ccamera_src_filter::init_read(unsigned int buffer_size) {
	buffers =  (buffer *)calloc(1, sizeof(*buffers));
 
	if (!buffers) {
		fprintf(stderr, "Out of memory/n");
		exit(EXIT_FAILURE);
	}
 
	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);
 
	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory/n");
		exit(EXIT_FAILURE);
	}
}
 
void Ccamera_src_filter::init_mmap(void) {
	struct v4l2_requestbuffers req;
 
	CLEAR(req);
 
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
 
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
					"memory mapping/n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
 
	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s/n", dev_name);
		exit(EXIT_FAILURE);
	}
 
	buffers =(buffer *) calloc(req.count, sizeof(*buffers));
 
	if (!buffers) {
		fprintf(stderr, "Out of memory/n");
		exit(EXIT_FAILURE);
	}
 
	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;
 
		CLEAR(buf);
 
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;
 
		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");
 
		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,
				PROT_READ | PROT_WRITE /* required */,
				MAP_SHARED /* recommended */, fd, buf.m.offset);
 
		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}
 
void Ccamera_src_filter::init_userp(unsigned int buffer_size) {
	struct v4l2_requestbuffers req;
	unsigned int page_size;
 
	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);
 
	CLEAR(req);
 
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;
 
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
					"user pointer i/o/n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}
 
	buffers =(buffer *) calloc(4, sizeof(*buffers));
 
	if (!buffers) {
		fprintf(stderr, "Out of memory/n");
		exit(EXIT_FAILURE);
	}
 
	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = memalign(/* boundary */page_size,
				buffer_size);
 
		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory/n");
			exit(EXIT_FAILURE);
		}
	}
}
 
void Ccamera_src_filter::init_device(void) {
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;
	unsigned int min;
 
	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device/n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}
 
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device/n", dev_name);
		exit(EXIT_FAILURE);
	}
 
	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s does not support read i/o/n", dev_name);
			exit(EXIT_FAILURE);
		}
 
		break;
 
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			fprintf(stderr, "%s does not support streaming i/o/n", dev_name);
			exit(EXIT_FAILURE);
		}
 
		break;
	}
 
	/* Select video input, video standard and tune here. */
 
	CLEAR(cropcap);
 
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */
 
		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

    //set fmt
	CLEAR(fmt);
 
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 320;
	fmt.fmt.pix.height = 180;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
 
	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");


    //查看帧格式
    CLEAR(fmt);
    fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
		errno_exit("VIDIOC_G_FMT");
    //ioctl(fd,VIDIOC_G_FMT,&fmt);
    cout << "Currentdata format (" <<fmt.fmt.pix.width << "x" <<fmt.fmt.pix.height <<") " \
        << "pixFromat :" << (char) (fmt.fmt.pix.pixelformat & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 8) & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 16) & 0xFF) \
                         << (char) ((fmt.fmt.pix.pixelformat >> 24) & 0xFF) << endl;
    wid=fmt.fmt.pix.width;
    height=fmt.fmt.pix.height;
	/* Note VIDIOC_S_FMT may change width and height. */
 
	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

    
    
	switch (io) {
	case IO_METHOD_READ:
		init_read(fmt.fmt.pix.sizeimage);
		break;
 
	case IO_METHOD_MMAP:
		init_mmap();
		break;
 
	case IO_METHOD_USERPTR:
		init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}
 
void Ccamera_src_filter::close_device(void) {
	if (-1 == close(fd))
		errno_exit("close");
 
	fd = -1;
}
 
void Ccamera_src_filter::open_device(void) {
	struct stat st;
 
	if (-1 == stat(dev_name, &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s/n", dev_name, errno,
				strerror(errno));
		exit(EXIT_FAILURE);
	}
 
	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device/n", dev_name);
		exit(EXIT_FAILURE);
	}
 
	fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);
 
	if (-1 == fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s/n", dev_name, errno,
				strerror(errno));
		exit(EXIT_FAILURE);
	}
}

int Ccamera_src_filter::get_wid(void)
{

    return wid;
}
int Ccamera_src_filter::get_height(void)
{


    return height;
}





