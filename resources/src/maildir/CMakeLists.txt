
include_directories(
	${CMAKE_SOURCE_DIR}/libakonadi
	${CMAKE_SOURCE_DIR}/akonadiresources/include

)



########### next target ###############

set(maildir_SRCS maildir.cpp )

kde4_automoc(${maildir_SRCS})

kde4_add_executable(maildir RUN_UNINSTALLED ${maildir_SRCS})

target_link_libraries(maildir  akonadi akonadiresources ${KDE4_KDECORE_LIBS} )


