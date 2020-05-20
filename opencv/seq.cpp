#include <dirent.h>

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <utimer.cpp>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage is: " << argv[0] << " iterations src_path dst_path" << std::endl;
        return -1;
    }

    const int it = atoi(argv[1]);
    const std::string src_path{argv[2]};
    const std::string dst_path{argv[3]};

    // disable integrated multithread support
    cv::setNumThreads(0);

    std::vector<std::string> imgs;
    
    // read all the jpg files from the source directory
    struct dirent *entry;
    if (DIR *dir = opendir(src_path.c_str()); dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (std::string(entry->d_name).ends_with(".jpg"))
                imgs.push_back(entry->d_name);
        }
        closedir(dir);
    } else {
        perror("opendir");
        return EXIT_FAILURE;
    }

    cv::Mat src, dst;

    auto t0 = std::chrono::system_clock::now();
    for (int i = 0; i < it; ++i) {
        for (const auto &img: imgs) {
            src = cv::imread(src_path + img);
            cv::GaussianBlur(src, dst, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
            cv::Sobel(dst, dst, -1, 1, 1);
            cv::imwrite(dst_path + img, dst);
        }
    }
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - t0).count();
    std::cout << "Sequential execution took " << elapsed << " msecs" << std::endl;

    return 0;
}
