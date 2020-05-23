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

#include <iostream>
#include <string>

#include <ff/ff.hpp>
#include <ff/farm.hpp>

#include <utility.hpp>

using namespace ff;

struct Task {
    Task(unsigned char* ptr, size_t size, const std::string& name)
        : ptr(ptr), size(size), filename(name) {}

    unsigned char* ptr;
    size_t size;
    const std::string filename;
};

struct Emitter : ff_node_t<Task> {
    Emitter(const char** argv, int argc) : argv(argv), argc(argc) {}

    // ------------------- utility functions
    // It memory maps the input file and then assigns a task to one Worker
    bool doWork(const std::string& fname, size_t size) {
        unsigned char* ptr = nullptr;
        if (!mapFile(fname.c_str(), size, ptr))
            return false;
        Task* t = new Task(ptr, size, fname);
        ff_send_out(t);
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

        unmapFile(inPtr, inSize);

        const std::string outfile = task->filename + ".zip";
        // write the compressed data into disk
        success &= writeFile(outfile, ptrOut, cmp_len);
        if (success && REMOVE_ORIGIN) {
            unlink(task->filename.c_str());
        }
        delete[] ptrOut;
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
    for (int i = 0; i < nw; ++i)
        workers.push_back(make_unique<Worker>());
    ff_Farm<> farm(std::move(workers), emitter);
    farm.remove_collector();
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
