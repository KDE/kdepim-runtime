kde_enable_exceptions()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

set(testsubscriptiondialog_SRCS
  testsubscriptiondialog.cpp
  ../imapaccount.cpp
  ../subscriptiondialog.cpp
  ../imapresource_debug.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_BINARY_DIR}/..)
add_executable(testsubscriptiondialog ${testsubscriptiondialog_SRCS})

target_link_libraries(testsubscriptiondialog KF6::KIOCore KPim6::IMAP KPim6::Mime KF6::ItemModels KF6::ConfigGui Qt::Widgets KF6::I18n KF6::KIOWidgets KPim6::AkonadiCore KPim6::MailTransport)

