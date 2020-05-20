#include <dirent.h>

#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <utimer.cpp>

int main(int argc, char* argv[]) {
    // disable integrated multithread support
    cv::setNumThreads(0);

    const std::string src_path{"brina/"};
    const std::string dst_path{"dest/"};
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

    long t1{0}, t2{0}, t3{0}, t4{0}, t;
    for (const auto &img: imgs) {
        {
            utimer timer("imread", &t);
            src = cv::imread(src_path + img);
        }
        t1 += t;
        {
            utimer timer("GaussianBlur", &t);
            cv::GaussianBlur(src, dst, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
        }
        t2 += t;
        {
            utimer timer("Sobel", &t);
            cv::Sobel(dst, dst, -1, 1, 1);
        }
        t3 += t;
        {
            utimer timer("imwrite", &t);
            cv::imwrite(dst_path + img, dst);
        }
        t4 += t;
    }
    std::cout << t1 / imgs.size() << std::endl;
    std::cout << t2 / imgs.size() << std::endl;
    std::cout << t3 / imgs.size() << std::endl;
    std::cout << t4 / imgs.size() << std::endl;
    return 0;
}
