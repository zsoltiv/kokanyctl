#include <forward_list>
#include <cstdlib>

#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp"

#include "net.h"
#include "imgproc.h"

struct imgproc_data {
    cv::VideoCapture cap;
    std::forward_list<cv::Mat> frames;
};

extern "C" struct imgproc_data *imgproc_init(struct video_data *video_data)
{
    struct imgproc_data *imgproc = (struct imgproc_data *) malloc(sizeof(struct imgproc_data));

    imgproc->cap = cv::VideoCapture("", cv::CAP_FFMPEG);

    return imgproc;
}

extern "C" void imgproc_thread(void *arg)
{
}
