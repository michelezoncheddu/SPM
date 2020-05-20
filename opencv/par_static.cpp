#include <dirent.h>

#include <iostream>
#include <thread>

#include <queue.cpp>
#include "BLstatic.cpp"

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


    MySource s{imgs.size(), it, nw};
    MyWorker f{imgs, src_path, dst_path};
    syque<pair<int, int>*> *t_queue = new syque<pair<int, int>*>[nw];
    pair<int, int> *EOS = nullptr;

    auto emit_task = [&](MySource s) {
        int worker_n = 0;
        while (s.hasNext()) {
            auto t = s.next();
            t_queue[worker_n].push(t);
            worker_n = (worker_n + 1) % nw;
        }
        for (int i = 0; i < nw; i++)
            t_queue[i].push(EOS);
    };

    auto body = [&](MyWorker w, int wn) {
        while (true) {
            auto t = t_queue[wn].pop();
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
    delete[] t_queue;

    cout << "Parallel execution with " << nw << " threads took " << elapsed << " msecs" << endl;

    return 0;
}
