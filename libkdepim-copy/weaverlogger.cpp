#include <weaver.h>
#include "weaverlogger.h"

namespace KPIM {
namespace ThreadWeaver {

    extern void debug(int severity, const char * cformat, ...);

    WeaverThreadLogger::WeaverThreadLogger( QObject *parent, const char *name)
        : WeaverExtension (parent, name)
    {
    }

    WeaverThreadLogger::~WeaverThreadLogger ()
    {
    }

    void WeaverThreadLogger::threadCreated (KPIM::ThreadWeaver::Thread *thread)
    {
        debug ( 1, "WeaverThreadLogger: thread (ID: %i) created.\n", 
                thread->id() );
    }

    void WeaverThreadLogger::threadDestroyed (KPIM::ThreadWeaver::Thread *thread)
    {
        debug ( 1, "WeaverThreadLogger: thread (ID: %i) destroyed.\n", 
                thread->id() );
    }

    void WeaverThreadLogger::threadBusy (KPIM::ThreadWeaver::Thread *thread)
    {
        debug ( 1, "WeaverThreadLogger: thread (ID: %i) is processing a job.\n",
                thread->id() );
    }

    void WeaverThreadLogger::threadSuspended (KPIM::ThreadWeaver::Thread *thread)
    {
        debug ( 1, "WeaverThreadLogger: thread (ID: %i) suspended.\n",
                thread->id() );
    }

}
}
