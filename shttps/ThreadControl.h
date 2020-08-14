//
// Created by Lukas Rosenthaler on 08.07.20.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h> //for threading , link with lpthread
//#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <unistd.h>
#include <syslog.h>
#include <poll.h>

#include <atomic>

#ifndef SIPI_THREADCONTROL_H
#define SIPI_THREADCONTROL_H

namespace shttps {

    class Server; // Declaration only

    class ThreadControl {
    public:
        typedef struct {
            pthread_t tid;
            int control_pipe;
        } ThreadMasterData;

        typedef struct {
            int control_pipe;
            int result;
            Server *serv;
        } ThreadChildData;

        typedef enum {INVALID_INDEX} ThreadControlErrors;

    private:
        std::vector<ThreadMasterData> thread_list; //!> List of all threads
        std::vector<ThreadChildData> child_data; //!> Data given to the thread
        std::queue<ThreadMasterData> thread_queue; //!> Queue of available threads for processing
        std::mutex thread_queue_mutex;

    public:
        ThreadControl(int n_threads, void *(*start_routine)(void*), Server *serv);

        ~ThreadControl();

        void thread_push(const ThreadMasterData &tinfo);

        bool thread_pop(ThreadMasterData &tinfo);

        int thread_delete(int pos);

        ThreadMasterData &operator[](int index);

        inline int nthreads() const { return thread_list.size(); }

        void join_all();
    };

}

#endif //SIPI_THREADCONTROL_H
