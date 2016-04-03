

set(pop3_optional_includes)
set(pop3_optional_libs)

if(Sasl2_FOUND)
   set(pop3_optional_includes ${pop3_optional_includes} ${Sasl2_INCLUDE_DIRS})
   set(pop3_optional_libs     ${pop3_optional_libs}     ${Sasl2_LIBRARIES})
   set(HAVE_LIBSASL2 1)
else()
   set(HAVE_LIBSASL2 0)
endif()


configure_file(pop3-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/pop3-config.h )

include_directories(  ${pop3_optional_includes}  )

########### next target ###############

set(kio_pop3_PART_SRCS pop3.cpp pop3_debug.cpp )

ecm_qt_declare_logging_category(kio_pop3_PART_SRCS HEADER pop3_debug.h IDENTIFIER POP3_LOG CATEGORY_NAME log_pop3)

add_library(kio_pop3 MODULE ${kio_pop3_PART_SRCS})


target_link_libraries(kio_pop3  KF5::KIOCore ${pop3_optional_libs} KF5::I18n )
set_target_properties(kio_pop3 PROPERTIES OUTPUT_NAME "pop3")

install(TARGETS kio_pop3  DESTINATION ${KDE_INSTALL_PLUGINDIR}/kf5/kio/ )


########### install files ###############

install( FILES pop3.protocol pop3s.protocol  DESTINATION  ${KDE_INSTALL_KSERVICES5DIR} )



