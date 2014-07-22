// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include <functional>
#include <memory>

typedef std::function<void(void)> RunMethord;

struct TaskClosureInt
{
    virtual ~TaskClosureInt() {}
    virtual void Run() const = 0;
};

template<typename T>
struct TaskClosureImpl : public TaskClosureInt
{
    explicit TaskClosureImpl(RunMethord methord) : bStaticFunc(true), runMethord(methord) {}
    TaskClosureImpl(std::shared_ptr<T> pObj, RunMethord methord)
        : bStaticFunc(false), pObject(pObj), runMethord(methord) {}
    ~TaskClosureImpl() {}

    virtual void Run() const override
    {
        if (bStaticFunc)
        {
            runMethord();
        }
        else
        {
            std::shared_ptr<T> pObj = pObject.lock();
            if (pObj)
            {
                runMethord();
            }
        }
    }

    bool bStaticFunc;
    std::weak_ptr<T> pObject;
    RunMethord runMethord;
};

typedef std::shared_ptr<TaskClosureInt> TaskClosure;

template<typename FuncType, typename... ArgsType>
TaskClosure CreateTaskClosure(FuncType func, ArgsType... args)
{
    return TaskClosure(new TaskClosureImpl<char>(std::bind(func, args...)));
}

template<typename FuncType, typename ObjType, typename... ArgsType>
TaskClosure CreateTaskClosure(FuncType func, std::shared_ptr<ObjType> ptr, ArgsType... args)
{
    return TaskClosure(new TaskClosureImpl<ObjType>(ptr, std::bind(func, ptr.get(), args...)));
}
