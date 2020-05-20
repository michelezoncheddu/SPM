// this is the business logic code
#include <iostream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "BLcode.hpp"

using namespace std;

class MySource : public Source<pair<int, int>*> {
   private:
   const int nw;
   int i = 0, begin = 0, end, partsize, remaining;

   public:
    MySource(size_t size, int iterations, int nw) : nw{nw} {
        end = size * iterations - 1;
        partsize  = (end + 1) / nw;
        remaining = (end + 1) % nw;
    }

    pair<int, int>* next() {
        end = begin + partsize + (remaining > 0 ? 1 : 0);
        pair<int, int>* p = new pair<int, int>{begin, end};
        --remaining;
        begin = end;
        ++i;
        return p;
    }

    bool hasNext() {
        return i < nw;
    }
};

class MyWorker : public Worker<pair<int, int>*> {
   private:
   const vector<string> &imgs;
   const string &src_path, &dst_path;

   public:
    MyWorker(const vector<string> &imgs, const string &src_path, const string &dst_path)
        : imgs{imgs}, src_path{src_path}, dst_path{dst_path} {}

    void compute(pair<int, int> *range) {
        cv::Mat src, dst;
        for (int i = range->first; i < range->second; ++i) {
            src = cv::imread(src_path + imgs[i % imgs.size()]);
            cv::GaussianBlur(src, dst, cv::Size(3, 3), 0, 0, cv::BORDER_DEFAULT);
            cv::Sobel(dst, dst, -1, 1, 1);
            cv::imwrite(dst_path + imgs[i % imgs.size()], dst);
        }

        delete range;
    }
};
