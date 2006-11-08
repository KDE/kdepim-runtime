if(KDE4_BUILD_TESTS)
set(example_SRCS example.cpp )

kde4_automoc(${example_SRCS})

kde4_add_kcfg_files(example_SRCS exampleprefs_base.kcfgc )

kde4_add_executable(example ${example_SRCS})

target_link_libraries(example  ${KDE4_KDECORE_LIBS} kdepim )

endif(KDE4_BUILD_TESTS)

