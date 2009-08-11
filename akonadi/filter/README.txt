This directory contains the sources for the Akonadi Filtering Framework.


The agent subdirectory contains the sources for a wannabe-generic filtering
agent which at the moment of writing supports message/rfc822 PIM items.

The akonadi subdirectory contains the sources for two libraries:
 
  libakonadi-filter
     which contains the core filtering framework

  libakonadi-filter-ui
     which contains the UI facilities for filter editing

The console subdirectory contains a sample UI application that interfaces
the filtering agent above, allows editing filters and triggering them
on Akonadi PIM items.


You can run "make docs" to obtain some nice documentation in ./doc/api.
Upgrade your doxygen to 1.5.9 if you see duplicated scope names around.

If you want to look at the code then the best place to start is probably
akonadi/filter/program.h. It will give you an idea about what is going on.


Authors:

 - Szymon Tomasz Stefanek <s dot stefanek at gmail dot com>
