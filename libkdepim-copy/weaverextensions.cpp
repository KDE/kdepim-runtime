#include "weaverextensions.h"
#include "weaver.h"

namespace KPIM {
namespace ThreadWeaver {

    WeaverExtension::WeaverExtension ( QObject *parent, const char *name)
        : QObject (parent, name)
    {
    }

    void WeaverExtension::attach (Weaver *w)
    {
        connect (w, SIGNAL (threadCreated (Thread *) ),
                 SLOT (threadCreated (Thread *) ) );
        connect (w, SIGNAL (threadDestroyed (Thread *) ),
                 SLOT (threadDestroyed (Thread *) ) );
        connect (w, SIGNAL (threadBusy (Thread *) ),
                 SLOT (threadBusy (Thread *) ) );
        connect (w, SIGNAL (threadSuspended (Thread *) ),
                 SLOT (threadSuspended (Thread *) ) );
    }

    WeaverExtension::~WeaverExtension()
    {
    }

    void WeaverExtension::threadCreated (Thread *)
    {
    }

    void WeaverExtension::threadDestroyed (Thread *)
    {
    }

    void WeaverExtension::threadBusy (Thread *)
    {
    }

    void WeaverExtension::threadSuspended (Thread *)
    {
    }

}
}

#include "weaverextensions.moc"
