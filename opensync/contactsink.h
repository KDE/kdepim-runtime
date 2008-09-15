#ifndef __CONTACTSINK_H
#define __CONTACTSINK_H

#include "datasink.h"

using namespace Akonadi;

class ContactSink : public DataSink
{
  Q_OBJECT
  
  public:
    ContactSink();
    ~ContactSink() {}
    
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
