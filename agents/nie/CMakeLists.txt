#
# If any of the .rdfs files change or if Nepomuk changes extensively,
# you'll need to regenerate the sources. You will also need to add/remove
# to the ${nie_SRCS} variable below and to the SVN repository.
#
# run 'make nie-gen' to regenerate the sources.
#

add_custom_command(
  OUTPUT 
    ${CMAKE_CURRENT_BINARY_DIR}/out_headers
    ${CMAKE_CURRENT_BINARY_DIR}/out_sources
    ${CMAKE_CURRENT_BINARY_DIR}/out_includes
  COMMAND ${CMAKE_COMMAND}
  ARGS 
    -DBIN_INSTALL_DIR=${BIN_INSTALL_DIR}
    -Dontofile1=${CMAKE_CURRENT_SOURCE_DIR}/nie.rdfs
    -Dontofile2=${CMAKE_CURRENT_SOURCE_DIR}/nco.rdfs
    -Dontofile3=${CMAKE_CURRENT_SOURCE_DIR}/nmo.rdfs
    -Dontofile4=${CMAKE_CURRENT_SOURCE_DIR}/nfo.rdfs
    -Dtargetdir=${CMAKE_CURRENT_SOURCE_DIR}
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
  DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/out_headers
    ${CMAKE_CURRENT_BINARY_DIR}/out_sources
    ${CMAKE_CURRENT_BINARY_DIR}/out_includes
)

set(nie_SRCS
  affiliation.cpp                                                             
  application.cpp                                                             
  archive.cpp                                                                 
  archiveitem.cpp                                                             
  attachment.cpp                                                              
  audio.cpp                                                                   
  audioimaccount.cpp                                                          
  bbsnumber.cpp                                                               
  carphonenumber.cpp                                                          
  cellphonenumber.cpp                                                         
  compressiontype.cpp                                                         
  contact.cpp                                                                 
  contactgroup.cpp                                                            
  contactlist.cpp                                                             
  contactlistdataobject.cpp                                                   
  contactmedium.cpp                                                           
  cursor.cpp                                                                  
  datacontainer.cpp                                                           
  dataobject.cpp                                                              
  datasource.cpp                                                              
  deletedresource.cpp                                                         
  document.cpp                                                                
  domesticdeliveryaddress.cpp                                                 
  emailaddress.cpp                                                            
  email.cpp                                                                   
  embeddedfiledataobject.cpp                                                  
  executable.cpp                                                              
  faxnumber.cpp                                                               
  filedataobject.cpp                                                          
  filehash.cpp                                                                
  filesystem.cpp                                                              
  filesystemimage.cpp                                                         
  folder.cpp                                                                  
  font.cpp                                                                    
  gender.cpp                                                                  
  harddiskpartition.cpp                                                       
  htmldocument.cpp                                                            
  icon.cpp                                                                    
  imaccount.cpp                                                               
  image.cpp                                                                   
  immessage.cpp                                                               
  informationelement.cpp                                                      
  internationaldeliveryaddress.cpp                                            
  isdnnumber.cpp
  mailbox.cpp
  mailboxdataobject.cpp
  media.cpp
  mediafilelistentry.cpp
  medialist.cpp
  mediastream.cpp
  message.cpp
  messageheader.cpp
  messagingnumber.cpp
  mindmap.cpp
  modemnumber.cpp
  operatingsystem.cpp
  organizationcontact.cpp
  pagernumber.cpp
  paginatedtextdocument.cpp
  parceldeliveryaddress.cpp
  pcsnumber.cpp
  personcontact.cpp
  phonenumber.cpp
  plaintextdocument.cpp
  postaladdress.cpp
  presentation.cpp
  rasterimage.cpp
  remotedataobject.cpp
  remoteportaddress.cpp
  role.cpp
  software.cpp
  softwareitem.cpp
  softwareservice.cpp
  sourcecode.cpp
  spreadsheet.cpp
  textdocument.cpp
  trash.cpp
  vectorimage.cpp
  video.cpp
  videoimaccount.cpp
  videotelephonenumber.cpp
  visual.cpp
  voicephonenumber.cpp
  website.cpp
)

kde4_add_library(nie STATIC ${nie_SRCS})

target_link_libraries(nie ${QT_QTCORE_LIBRARY} ${NEPOMUK_LIBRARIES})
