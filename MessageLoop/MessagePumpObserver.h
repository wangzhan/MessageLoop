// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include <vector>

class MessagePumpObserver
{
public:
    ~MessagePumpObserver() {}

    virtual void WillProcessMessage(const MSG &msg) = 0;
    virtual void DidProcessMessage(const MSG &msg) = 0;
};

typedef std::vector<MessagePumpObserver*> MESSAGE_PUMP_OBSERVER_VECT;
typedef MESSAGE_PUMP_OBSERVER_VECT::iterator MESSAGE_PUMP_OBSERVER_VECT_ITER;

#define FOR_EACH_PUMP_OBSERVER(vect, func) { \
    for (MESSAGE_PUMP_OBSERVER_VECT_ITER iter = vect.begin(); iter != vect.end(); ++iter) \
    {\
        (*iter)->func; \
    }\
}
