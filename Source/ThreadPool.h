#pragma once

#include <iostream>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include <future>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <atomic>
#include <variant>
#include <cassert>
#include <map>
#include <utility>
#include <functional>

#include "Logger.h"

enum class TaskStatus
{
	InQueue,
	Completed
};

class Task
{
public:
	template <typename FuncRetType, typename ...Args, typename ...FuncTypes>
	Task(FuncRetType(*func)(FuncTypes...), Args&&... args) :
		IsVoid{ std::is_void_v<FuncRetType> }
	{
		if constexpr (std::is_void_v<FuncRetType>)
		{
			VoidFunc = std::bind(func, args...);
			AnyFunc = []()->int { return 0; };
		}
		else
		{
			VoidFunc = []()->void {};
			AnyFunc = std::bind(func, args...);
		}
	}

	void operator() ();
	bool HasResult();
	std::any GetResult() const;

private:
	std::function<void()> VoidFunc;
	std::function<std::any()> AnyFunc;
	std::any AnyFuncResult;
	bool IsVoid;
};

struct TaskInfo
{
	TaskStatus Status = TaskStatus::InQueue;
	std::any Result;
};

class ThreadPool
{
public:
	ThreadPool();

	template <typename Func, typename ...Args, typename ...FuncTypes>
	uint64_t AddTask(Func(*func)(FuncTypes...), Args&&... args)
	{
		const uint64_t TaskId = LastIndex++;

		std::unique_lock<std::mutex> Lock(TasksInfoMutex);
		TasksInfo[TaskId] = TaskInfo();
		Lock.unlock();

		std::lock_guard<std::mutex> QueueLock(QueueMutex);
		Queue.emplace(Task(func, std::forward<Args>(args)...), TaskId);
		QueueCondVar.notify_one();
		return TaskId;
	}

	void Wait(const uint64_t TaskId);
	std::any WaitResult(const uint64_t TaskId);

	template<class T>
	void WaitResult(const uint64_t TaskId, T& Value)
	{
		std::unique_lock<std::mutex> Lock(TasksInfoMutex);
		TasksInfoCondVar.wait(Lock, [this, TaskId]()->bool
			{
				return TaskId < LastIndex&& TasksInfo[TaskId].Status == TaskStatus::Completed;
			});
		Value = std::any_cast<T>(TasksInfo[TaskId].Result);
	}

	void WaitAll();
	bool Calculated(const uint64_t TaskId);
	~ThreadPool();

private:

	void Run();

	std::vector<std::thread> Threads;

	std::queue<std::pair<Task, uint64_t>> Queue;
	std::mutex QueueMutex;
	std::condition_variable QueueCondVar;

	std::unordered_map<uint64_t, TaskInfo> TasksInfo;
	std::condition_variable TasksInfoCondVar;
	std::mutex TasksInfoMutex;

	std::condition_variable WaitAllCondVar;

	std::atomic<bool> Quit{ false };
	std::atomic<uint64_t> LastIndex{ 0 };
	std::atomic<uint64_t> CompletedTasks{ 0 };
};
