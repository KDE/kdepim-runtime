#
# If any of the .rdfs files change or if Nepomuk changes extensively,
# you may need to add/remove source files ${nie_SRCS} variable below.
#

set(nie_SRCS
  ${CMAKE_CURRENT_BINARY_DIR}/affiliation.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/application.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/archive.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/archiveitem.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/attachment.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/audio.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/audioimaccount.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/bbsnumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/carphonenumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/cellphonenumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/compressiontype.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/contact.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/contactgroup.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/contactlist.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/contactlistdataobject.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/contactmedium.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/cursor.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/datacontainer.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/dataobject.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/datasource.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/deletedresource.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/document.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/domesticdeliveryaddress.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/emailaddress.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/email.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/embeddedfiledataobject.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/executable.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/faxnumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/filedataobject.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/filehash.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/filesystem.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/filesystemimage.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/folder.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/font.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/gender.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/harddiskpartition.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/htmldocument.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/icon.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/imaccount.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/image.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/immessage.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/informationelement.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/internationaldeliveryaddress.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/isdnnumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/mailbox.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/mailboxdataobject.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/media.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/mediafilelistentry.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/medialist.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/mediastream.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/message.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/messageheader.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/messagingnumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/mindmap.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/modemnumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/operatingsystem.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/organizationcontact.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/pagernumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/paginatedtextdocument.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/parceldeliveryaddress.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/pcsnumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/personcontact.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/phonenumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/plaintextdocument.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/postaladdress.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/presentation.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/rasterimage.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/remotedataobject.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/remoteportaddress.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/role.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/software.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/softwareitem.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/softwareservice.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/sourcecode.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/spreadsheet.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/textdocument.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/trash.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/vectorimage.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/video.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/videoimaccount.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/videotelephonenumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/visual.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/voicephonenumber.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/website.cpp
)

add_custom_command(
  OUTPUT 
    ${CMAKE_CURRENT_BINARY_DIR}/out_headers
    ${CMAKE_CURRENT_BINARY_DIR}/out_sources
    ${CMAKE_CURRENT_BINARY_DIR}/out_includes
    ${nie_SRCS}
  COMMAND ${CMAKE_COMMAND}
  ARGS 
    -DBIN_INSTALL_DIR=${BIN_INSTALL_DIR}
    -Dontofile1=${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
    -Dontofile2=${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
    -Dontofile3=${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs
    -Dontofile4=${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs
    -Dtargetdir=${CMAKE_CURRENT_BINARY_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/rcgen.cmake
  MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
    ${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
    ${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs 
    ${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs 
)

add_custom_target(
  nie-gen
  ALL
  DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/out_headers
    ${CMAKE_CURRENT_BINARY_DIR}/out_sources
    ${CMAKE_CURRENT_BINARY_DIR}/out_includes
)

kde4_add_library(nie STATIC ${nie_SRCS})
add_dependencies(nie nie-gen)

target_link_libraries(nie ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
