#ifndef WEAVERLOGGER_H
#define WEAVERLOGGER_H

#include <weaverextensions.h>

namespace KPIM {
namespace ThreadWeaver {

    /** A WeaverThreadLogger may be attached to a Weaver to gain debug
       information about thread execution.  */
    class WeaverThreadLogger : public WeaverExtension
    {
        Q_OBJECT
    public:
        WeaverThreadLogger( QObject *parent = 0, const char *name = 0);
        ~WeaverThreadLogger();
        void threadCreated (Thread *);
        void threadDestroyed (Thread *);
        void threadBusy (Thread *);
        void threadSuspended (Thread *);
    };

}
}

#endif // WEAVERLOGGER_H
