#include <dirent.h>

#include <iostream>
#include <thread>

#include <queue.cpp>
#include "BLrr.cpp" // I can use the same BL of the round robin version

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cout << "Usage is: " << argv[0] << " nw iterations src_path dst_path" << std::endl;
        return -1;
    }

    const int nw = atoi(argv[1]);
    const int it = atoi(argv[2]);
    const string src_path{argv[3]};
    const string dst_path{argv[4]};

    // disable integrated multithread support
    cv::setNumThreads(0);

    vector<string> imgs;
    
    // read all the jpg files from the source directory
    struct dirent *entry;
    if (DIR *dir = opendir(src_path.c_str()); dir != NULL) {
        while ((entry = readdir(dir)) != NULL) {
            if (string(entry->d_name).ends_with(".jpg"))
                imgs.push_back(entry->d_name);
        }
        closedir(dir);
    } else {
        perror("opendir");
        return EXIT_FAILURE;
    }

    MySource s(imgs.size(), it);
    MyWorker f(imgs, src_path, dst_path);
    syque<int> t_queue;
    const int EOS = -1;

    auto emit_task = [&](MySource s) {
        while (s.hasNext()) {
            auto t = s.next();
            t_queue.push(t);
        }
        for (int i = 0; i < nw; i++)
            t_queue.push(EOS);
    };

    auto body = [&](MyWorker w, int wn) {
        while (true) {
            auto t = t_queue.pop();
            if (t == EOS)
                break; 
            w.compute(t);
        }
    };

    auto t0 = chrono::system_clock::now();
    auto e_thr = new thread(emit_task, s);

    vector<thread*> tids(nw);
    for (int i = 0; i < nw; i++)
        tids[i] = new thread(body, f, i);

    // now await termination
    e_thr->join();
    for (int i = 0; i < nw; i++)
        tids[i]->join();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - t0).count();

    cout << "Parallel execution with " << nw << " threads took " << elapsed << " msecs" << endl;

    return 0;
}
