// Author: WangZhan -> wangzhan.1985@gmail.com
#pragma once
#include "MessagePump.h"
#include "BasicTypes.h"
#include "ScopedHandle.h"

// Typical use #1:
//   // Use only when there are no user's buffers involved on the actual IO,
//   // so that all the cleanup can be done by the message pump.
//   class MyFile : public IOHandler {
//     MyFile() {
//       ...
//       context_ = new IOContext;
//       context_->handler = this;
//       message_pump->RegisterIOHandler(file_, this);
//     }
//     ~MyFile() {
//       if (pending_) {
//         // By setting the handler to NULL, we're asking for this context
//         // to be deleted when received, without calling back to us.
//         context_->handler = NULL;
//       } else {
//         delete context_;
//      }
//     }
//     virtual void OnIOCompleted(IOContext* context, DWORD bytes_transfered,
//                                DWORD error) {
//         pending_ = false;
//     }
//     void DoSomeIo() {
//       ...
//       // The only buffer required for this operation is the overlapped
//       // structure.
//       ConnectNamedPipe(file_, &context_->overlapped);
//       pending_ = true;
//     }
//     bool pending_;
//     IOContext* context_;
//     HANDLE file_;
//   };

class MessagePumpForIO : public MessagePumpForMsg
{
public:
    MessagePumpForIO();
    ~MessagePumpForIO();

    class IOHandler;

    struct IOContext
    {
        OVERLAPPED overlapped;
        IOHandler *pHandler;
    };

    class IOHandler
    {
    public:
        virtual ~IOHandler() {}

        virtual void OnIOCompleted(IOContext *pContext, DWORD dwBytesTranfered,
            DWORD dwError) = 0;
    };

    void RegisterIOHandler(HANDLE fileHandle, IOHandler *pHandler);
    bool WaitForIOCompletion(DWORD dwTimeout);

    virtual void ScheduleWork();
    virtual void ScheduleDelayedWork(DWORD dwDelayedWorkTime);

protected:
    virtual void DoRunLoop();
    bool WaitForWork();

private:
    ScopedHandle m_IOCompletionPort;

    DISALLOW_COPY_AND_ASSIGN(MessagePumpForIO);
};
