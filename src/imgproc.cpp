#include <cstdlib>
#include <cstring>
#include <vector>
#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

#include "SDL_net.h"
#include "opencv2/core.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

#include "net.h"
#include "utils.h"
#include "video.h"
#include "imgproc.h"

#define HAZMAT_DIR "hazmat"

struct imgproc_data {
    cv::VideoCapture cap;
    cv::Ptr<cv::FeatureDetector> detector;
    cv::BFMatcher matcher;
    std::vector<std::vector<cv::KeyPoint>> hazmat_keypoints;
    std::vector<cv::Mat> hazmat_descriptors;
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
    imgproc->matcher = cv::BFMatcher(cv::NORM_HAMMING);

    DIR *dir = opendir(HAZMAT_DIR);
    if(!dir)
        ctl_die((const char *) "opendir()\n");
    struct dirent *e;
    while((e = readdir(dir))) {
        std::string path(HAZMAT_DIR);
        path += '/';
        path += e->d_name;
        cv::Mat img = cv::imread(path);
        std::vector<cv::KeyPoint> keypoints;
        cv::Mat descriptors;
        imgproc->detector->detectAndCompute(img, cv::noArray(), keypoints, descriptors);
        imgproc->hazmat_descriptors.push_back(descriptors);
        imgproc->hazmat_keypoints.push_back(keypoints);
    }
    closedir(dir);

    return imgproc;
}

extern "C" void imgproc_thread(void *arg)
{
    struct imgproc_data *imgproc = (struct imgproc_data *)arg;
    cv::Mat frame, descriptors;
    std::vector<cv::KeyPoint> keypoints;

    while(!imgproc->cap.isOpened());

    while(true) {
        imgproc->cap >> frame;
        imgproc->detector->detectAndCompute(frame, cv::noArray(), keypoints, descriptors);
        for(int i = 0; i < imgproc->hazmat_keypoints.size(); i++) {
            std::vector<cv::DMatch> matches;
            imgproc->matcher.match(imgproc->hazmat_descriptors[i], descriptors, matches);
        }
    }
}
