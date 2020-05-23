/*
 * Simple file compressor using miniz and the FastFlow farm.
 *
 * miniz source code: https://github.com/richgel999/miniz
 * https://code.google.com/archive/p/miniz/
 *
 */
/* Author: Michele Zoncheddu <m.zoncheddu@studenti.unipi.it>
 * This code is a mix of POSIX C code and some C++ library call
 * (mainly for strings manipulation).
 */

#include <miniz.h>

#include <cmath>
#include <iostream>
#include <string>

#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include <utility.hpp>

#ifndef BIGFILE_LOW_THRESHOLD
    #define BIGFILE_LOW_THRESHOLD 5 // MB
#endif

using namespace ff;

constexpr long THRESHOLD = BIGFILE_LOW_THRESHOLD * 1000000; // from MB to bytes

struct Task {
    Task(unsigned char* ptr, size_t size, const std::string& name, int part, int totalsize)
        : ptr(ptr), size(size), filename(name), part(part), totalsize(totalsize) {}

    unsigned char* ptr;
    const size_t size;
    const std::string filename;
    const int part;
    const size_t totalsize;
};

struct Emitter : ff_node_t<Task> {
    Emitter(const char** argv, int argc) : argv(argv), argc(argc) {}

    // ------------------- utility functions
    // It memory maps the input file and then assigns a task to one Worker
    bool doWork(const std::string& fname, size_t size) {
        unsigned char* ptr = nullptr;
        if (!mapFile(fname.c_str(), size, ptr))
            return false;
        if (size <= THRESHOLD) {
            Task* t = new Task(ptr, size, fname, 0, size);
            ff_send_out(t);
        } else {
            const int parts = ceil(size * 1.0 / THRESHOLD);
            for (int i = 0; i < parts - 1; ++i) {
                Task* t = new Task(ptr + THRESHOLD * i, THRESHOLD, fname, i + 1, size);
                ff_send_out(t);
            }
            // last part
            Task* t = new Task(ptr + THRESHOLD * (parts - 1), size % THRESHOLD, fname, parts, size);
            ff_send_out(t);
        }
        return true;
    }
    // walks through the directory tree rooted in dname
    bool walkDir(const std::string& dname, size_t size) {
        DIR* dir;
        if ((dir = opendir(dname.c_str())) == NULL) {
            perror("opendir");
            fprintf(stderr, "Error: opendir %s\n", dname.c_str());
            return false;
        }
        struct dirent* file;
        bool error = false;
        while ((errno = 0, file = readdir(dir)) != NULL) {
            struct stat statbuf;
            std::string filename = dname + "/" + file->d_name;
            if (stat(filename.c_str(), &statbuf) == -1) {
                perror("stat");
                fprintf(stderr, "Error: stat %s\n", filename.c_str());
                return false;
            }
            if (S_ISDIR(statbuf.st_mode)) {
                if (!isdot(filename.c_str())) {
                    if (!walkDir(filename, statbuf.st_size))
                        error = true;
                }
            } else {
                if (!doWork(filename, statbuf.st_size))
                    error = true;
            }
        }
        if (errno != 0) {
            perror("readdir");
            error = true;
        }
        closedir(dir);
        return !error;
    }
    // -------------------

    Task* svc(Task*) {
        for (long i = 0; i < argc; ++i) {
            struct stat statbuf;
            if (stat(argv[i], &statbuf) == -1) {
                perror("stat");
                fprintf(stderr, "Error: stat %s\n", argv[i]);
                continue;
            }
            bool dir = false;
            if (S_ISDIR(statbuf.st_mode)) {
                success &= walkDir(argv[i], statbuf.st_size);
            } else {
                success &= doWork(argv[i], statbuf.st_size);
            }
        }
        return EOS;
    }

    void svc_end() {
        if (!success) {
            printf("Read stage: Exiting with (some) Error(s)\n");
            return;
        }
    }

    const char** argv;
    const int argc;
    bool success = true;
};

struct Worker : ff_node_t<Task> {
    Task* svc(Task* task) {
        unsigned char* inPtr = task->ptr;
        const size_t inSize = task->size;
        const int part = task->part;
        const bool splitted = task->part > 0;
        
        // get an estimation of the maximum compression size
        unsigned long cmp_len = compressBound(inSize);
        // allocate memory to store compressed data in memory
        unsigned char* ptrOut = new unsigned char[cmp_len];
        if (compress(ptrOut, &cmp_len, (const unsigned char*)inPtr, inSize) != Z_OK) {
            printf("Failed to compress file %s in memory\n", task->filename.c_str());
            success = false;
            delete[] ptrOut;
            return GO_ON;
        }

        if (!splitted)
            unmapFile(inPtr, inSize);

        const std::string outfile = task->filename + (splitted ? ".part" + std::to_string(part) : "") + ".zip";
        // write the compressed data into disk
        success &= writeFile(outfile, ptrOut, cmp_len);
        if (success && REMOVE_ORIGIN)
            unlink(task->filename.c_str());
        delete[] ptrOut;

        if (splitted) // send to the collector
            return task;
        else 
            delete task;
        return GO_ON;
    }

    void svc_end() {
        if (!success) {
            printf("Compressor stage: Exiting with (some) Error(s)\n");
            return;
        }
    }
    bool success = true;
};

struct Collector : ff_node_t<Task> {
    Task* svc(Task* task) {
        if (auto item = map.find(task->filename); item == map.end()) {
            map[task->filename] = {task->totalsize - task->size, task->ptr};
        } else {
            map[task->filename].first -= task->size;
            if (task->part == 1) // update the starting pointer
                map[task->filename].second = task->ptr;
            if (map[task->filename].first == 0) { // all data received
                unmapFile(map[task->filename].second, task->totalsize);
                map.erase(item);
                const std::string command = "tar cf " + task->filename + ".zip " + task->filename + ".part*.zip";
                if (system(command.c_str()) != 0)
                    std::cout << "Error executing: " << command << std::endl;
            }
        }
        delete task;
        return GO_ON;
    }

    std::unordered_map<std::string, std::pair<size_t, unsigned char*>> map;
};

static inline void usage(const char* argv0) {
    printf("--------------------\n");
    printf("Usage: %s nw file-or-directory [file-or-directory]\n", argv0);
    printf("\nModes: COMPRESS ONLY\n");
    printf("--------------------\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage(argv[0]);
        return -1;
    }
    const int nw = atoi(argv[1]);
    argc-=2;

    ffTime(START_TIME);
    Emitter emitter(const_cast<const char**>(&argv[2]), argc);
    std::vector<std::unique_ptr<ff_node>> workers;
    Collector collector;
    for (int i = 0; i < nw; ++i)
        workers.push_back(make_unique<Worker>());
    ff_Farm<> farm(std::move(workers), emitter, collector);
    if (farm.run_and_wait_end() < 0) {
        error("running farm");
        return -1;
    }
    ffTime(STOP_TIME);
    std::cout << "Time with " << nw << " nw: " << ffTime(GET_TIME) << " (ms)" << std::endl;

    bool success = true;
    success &= emitter.success;
    if (success)
        printf("Done.\n");

    return 0;
}
