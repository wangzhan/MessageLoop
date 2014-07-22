#include "stdafx.h"
#include "../MessageLoop/TaskClosure.h"

void test1(int i)
{
    int a = 10;
}

class testobj
{
public:
    testobj() {}

    void mem2(int a, int b)
    {

    }

    void mem3()
    {
        std::shared_ptr<testobj> t1(this);
    }
};

void test()
{
    /*TaskClosureImpl<int> t1(std::bind(test1, 1));
    t1.Run();

    std::shared_ptr<testobj> obj1(new testobj);
    TaskClosureImpl<testobj> t2(obj1, std::bind(&testobj::mem2, obj1.get(), 1, 1));
    t2.Run();

    TaskClosurePtr t3 = CreateTaskClosure(test1, 1);
    t3->Run();
    delete t3;
    TaskClosurePtr t4 =
    CreateTaskClosure(obj1, &testobj::mem2, 1, 1);
    t4->Run();
    delete t4;*/

    TaskClosure t1 = CreateTaskClosure(test1, 1);
    t1->Run();

    std::shared_ptr<testobj> obj1(new testobj);
    TaskClosure t2 = CreateTaskClosure(&testobj::mem2, obj1.get(), 1, 1);
    t2->Run();
}
