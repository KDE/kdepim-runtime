#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kconfig.h>
#include <kdebug.h>

#include "resourcecalendar.h"
#include "calendar.h"

using namespace KCal;

ResourceCalendar::ResourceCalendar( const KConfig *config )
    : KRES::Resource( config )
{
}

ResourceCalendar::~ResourceCalendar()
{
}

void ResourceCalendar::writeConfig( KConfig* config )
{
  KRES::Resource::writeConfig( config );
}


