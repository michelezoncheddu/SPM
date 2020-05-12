#include <iostream>

// keep in mind that all or some of these includes
// may change depending on the library installation and version
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

int main(int argc, char* argv[]) {
    // declare image containers
    cv::Mat img, dst;
    // read one image from disk from disk
    img = cv::imread("uno.jpg");
    // apply first filter, producing a new image (just for example)
    cv::GaussianBlur(img, dst, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
    // apply second filter. Here we overwrite the input (just for example)
    cv::Sobel(dst, dst, -1, 1, 1);
    // write image to disk
    cv::imwrite("Una.jpg", dst);
    return 0;
}
