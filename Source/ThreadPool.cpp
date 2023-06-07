#include "ThreadPool.h"

void Task::operator() ()
{
    VoidFunc();
    AnyFuncResult = AnyFunc();
}

bool Task::HasResult()
{
    return !IsVoid;
}

std::any Task::GetResult() const
{
    assert(!IsVoid);
    assert(AnyFuncResult.has_value());
    return AnyFuncResult;
}

ThreadPool::ThreadPool()
{
    unsigned int MaxThreads = std::thread::hardware_concurrency();
    Threads.reserve(MaxThreads);
    for (unsigned int i = 0; i < MaxThreads; ++i)
    {
        Threads.emplace_back(&ThreadPool::Run, this);
    }
    Logger::Info("Using " + std::to_string(MaxThreads) + " threads");
}

void ThreadPool::Wait(const uint64_t TaskId)
{
    std::unique_lock<std::mutex> Lock(TasksInfoMutex);
    TasksInfoCondVar.wait(Lock, [this, TaskId]()->bool
        {
            return TaskId < LastIndex&& TasksInfo[TaskId].Status == TaskStatus::Completed;
        });
}

std::any ThreadPool::WaitResult(const uint64_t TaskId)
{
    std::unique_lock<std::mutex> Lock(TasksInfoMutex);
    TasksInfoCondVar.wait(Lock, [this, TaskId]()->bool
        {
            return TaskId < LastIndex&& TasksInfo[TaskId].Status == TaskStatus::Completed;
        });
    return TasksInfo[TaskId].Result;
}

void ThreadPool::WaitAll()
{
    std::unique_lock<std::mutex> Lock(TasksInfoMutex);
    WaitAllCondVar.wait(Lock, [this]()->bool { return CompletedTasks == LastIndex; });
}

bool ThreadPool::Calculated(const uint64_t TaskId)
{
    std::lock_guard<std::mutex> Lock(TasksInfoMutex);
    return TaskId < LastIndex&& TasksInfo[TaskId].Status == TaskStatus::Completed;
}

ThreadPool::~ThreadPool()
{
    Quit = true;
    QueueCondVar.notify_all();
    for (size_t i = 0; i < Threads.size(); ++i) {
        Threads[i].join();
    }
}

void ThreadPool::Run()
{
    while (!Quit)
    {
        std::unique_lock<std::mutex> QueueLock(QueueMutex);
        QueueCondVar.wait(QueueLock, [this]()->bool { return !Queue.empty() || Quit; });

        if (!Queue.empty() && !Quit)
        {
            std::pair<Task, uint64_t> Task = std::move(Queue.front());
            Queue.pop();
            QueueLock.unlock();

            Task.first();

            std::lock_guard<std::mutex> TaskInfoLock(TasksInfoMutex);
            if (Task.first.HasResult())
            {
                TasksInfo[Task.second].Result = Task.first.GetResult();
            }
            TasksInfo[Task.second].Status = TaskStatus::Completed;
            ++CompletedTasks;
        }
        WaitAllCondVar.notify_all();
        TasksInfoCondVar.notify_all(); // notify for wait function
    }
}
