// this is the business logic code
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "BLcode.hpp"

using namespace std;

class MySource : public Source<int> {
   private:
   int i{0}, end;

   public:
    MySource(size_t size, int iterations) {
        end = size * iterations - 1;
    }

    int next() {
        return i++;
    }

    bool hasNext() {
        return i < end;
    }
};

class MyWorker : public Worker<int> {
   private:
   const vector<string> &imgs;
   const string &src_path, &dst_path;

   public:
    MyWorker(const vector<string> &imgs, const string &src_path, const string &dst_path)
        : imgs{imgs}, src_path{src_path}, dst_path{dst_path} {}

    void compute(int i) {
        cv::Mat src, dst;
        src = cv::imread(src_path + imgs[i % imgs.size()]);
        cv::GaussianBlur(src, dst, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
        cv::Sobel(dst, dst, -1, 1, 1);
        cv::imwrite(dst_path + imgs[i % imgs.size()], dst);
    }
};
