#set(ENV{VCINSTALLDIR} "${VC_DIR}")
execute_process(COMMAND ${QtCore_location}/windeployqt --release --qmldir "${QML_DIR}" "${CMAKE_INSTALL_PREFIX}/lib/${LIB_PREFIX_STR}qmlwrap.dll")
