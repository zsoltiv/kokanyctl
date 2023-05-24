#include <cstdlib>

#include "SDL_net.h"
#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgproc.hpp"

#include "net.h"
#include "video.h"
#include "imgproc.h"

struct imgproc_data {
    cv::VideoCapture cap;
    cv::Ptr<cv::FeatureDetector> detector;
    cv::BFMatcher matcher;
};

extern "C" struct imgproc_data *imgproc_init(struct video_data *video_data, IPaddress *video_addr)
{
    struct imgproc_data *imgproc = (struct imgproc_data *) malloc(sizeof(struct imgproc_data));

    const std::string url(net_ffmpeg_format_url(video_addr));
    imgproc->cap = cv::VideoCapture(url, cv::CAP_FFMPEG);
    imgproc->cap.set(cv::CAP_PROP_FRAME_WIDTH, video_get_width(video_data));
    imgproc->cap.set(cv::CAP_PROP_FRAME_HEIGHT, video_get_height(video_data));
    imgproc->cap.set(cv::CAP_PROP_FPS, 24);

    imgproc->detector = cv::ORB::create();
    imgproc->matcher = cv::BFMatcher();

    return imgproc;
}

extern "C" void imgproc_thread(void *arg)
{
    struct imgproc_data *imgproc = (struct imgproc_data *)arg;

    while(true) {
    }
}
