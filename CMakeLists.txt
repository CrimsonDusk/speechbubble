project (ldforge)
add_subdirectory (updaterevision)
add_subdirectory (metacollector)
cmake_minimum_required (VERSION 2.6)
find_package (Qt4 REQUIRED)

include_directories (${QT_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})

set (SPEECHBUBBLE_SOURCES
	src/channel.cc
	src/commands.cc
	src/config.cc
	src/connection.cc
	src/context.cc
	src/crashcatcher.cc
	src/lineedit.cc
	src/main.cc
	src/mainwindow.cc
	src/misc.cc
	src/user.cc
	src/xml_document.cc
	src/xml_node.cc
	src/xml_scanner.cc
)

set (SPEECHBUBBLE_HEADERS
	src/channel.h
	src/commands.h
	src/config.h
	src/context.h
	src/connection.h
	src/crashcatcher.h
	src/format.h
	src/lineedit.h
	src/macros.h
	src/main.h
	src/mainwindow.h
	src/misc.h
	src/user.h
	src/xml_document.h
	src/xml_node.h
	src/xml_scanner.h
)

set (SPEECHBUBBLE_FORMS
	ui/entrylist.ui
	ui/bombbox.ui
	ui/connectTo.ui
	ui/main.ui
)

get_target_property (UPDATEREVISION_EXE updaterevision LOCATION)
get_target_property (METACOLLECTOR_EXE metacollector LOCATION)

add_custom_target (revision_check ALL
    COMMAND ${UPDATEREVISION_EXE} gitinfo.h
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS updaterevision)

add_custom_target (metacollection ALL
    COMMAND ${METACOLLECTOR_EXE} ${SPEECHBUBBLE_HEADERS} ${CMAKE_BINARY_DIR}/metadata.h ${CMAKE_BINARY_DIR}/metadata.cc
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS metacollector)

if (CMAKE_COMPILER_IS_GNUCXX)
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Werror=switch -Werror=uninitialized -Werror=address -Werror=return-type -Werror=maybe-uninitialized -Werror=format -Werror=double-promotion -std=c++11")
endif()

include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_BINARY_DIR}/src")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}")
include_directories ("${CMAKE_CURRENT_SOURCE_DIR}/src")

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEBUG")
endif()

qt4_wrap_cpp (SPEECHBUBBLE_MOC ${SPEECHBUBBLE_HEADERS})
qt4_wrap_ui (SPEECHBUBBLE_FORMS_HEADERS ${SPEECHBUBBLE_FORMS})

add_executable (speechbubble
	${SPEECHBUBBLE_SOURCES}
	${SPEECHBUBBLE_FORMS_HEADERS}
	${SPEECHBUBBLE_MOC}
)

target_link_libraries (speechbubble
	${QT_QTCORE_LIBRARY}
	${QT_QTGUI_LIBRARY}
	${QT_QTNETWORK_LIBRARY}
)

add_dependencies (speechbubble revision_check)
add_dependencies (speechbubble metacollection)

install (TARGETS speechbubble RUNTIME DESTINATION bin)
