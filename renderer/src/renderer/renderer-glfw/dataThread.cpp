#include <unistd.h> // usleep
#include <renderer/map.h>
#include "dataThread.h"
#include "threadName.h"
#include "gpuContext.h"
#include <GLFW/glfw3.h>

namespace
{
    void run(DataThread *data)
    {
        data->run();
    }
}

DataThread::DataThread(GLFWwindow *shared) : window(nullptr),
    map(nullptr), stop(false)
{
    window = glfwCreateWindow(1, 1, "data context", NULL, shared);
    glfwSetWindowUserPointer(window, this);
    glfwHideWindow(window);
    initializeGpuContext();
    thr = std::thread(&::run, this);
}

DataThread::~DataThread()
{
    stop = true;
    thr.join();
    glfwDestroyWindow(window);
    window = nullptr;
}

void DataThread::run()
{
    setThreadName("data");
    glfwMakeContextCurrent(window);
    while (!stop && !map)
        usleep(1000);
    setThreadName("downloader"); // the downloader threads inherits the name
    map->dataInitialize(nullptr);
    setThreadName("data");
    while (!stop)
    {
        if (map->dataTick())
            usleep(5000);
    }
    map->dataFinalize();
}
