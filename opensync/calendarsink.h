#ifndef __CALENDARSINK_H
#define __CALENDARSINK_H

#include "datasink.h"

using namespace Akonadi;

class CalendarSink : public DataSink
{
  Q_OBJECT
  
  public:
    CalendarSink();
    ~CalendarSink() {}
    
    //bool initialize( OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncObjTypeSink *sink, OSyncError **error );

    //void getChanges();
    //void commit( OSyncChange *change );
    //void syncDone();

    void reportChange( const Item & );

  protected:
    int createItem( OSyncData *data );

    int modifyItem( OSyncData *data );

    //virtual void deleteItem( OSyncData *data );
};

#endif
