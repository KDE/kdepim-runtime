# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause

macro(add_newmailnotification_unittest _source)
    get_filename_component(_name ${_source} NAME_WE)
    ecm_add_test(${_source} ${_name}.h
        TEST_NAME ${_name}
        LINK_LIBRARIES newmailnotifier Qt::Test Qt::Core KPim6::AkonadiCore
)
endmacro()


add_newmailnotification_unittest(newmailnotificationhistorymanagertest.cpp)
