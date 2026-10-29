cmake_minimum_required(VERSION 3.19)
project(sy LANGUAGES C CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(sy
    WIN32 MACOSX_BUNDLE
    main.cpp
    src/unicode2gbktab.h
    src/autf8.c src/autf8.h
    src/beastSound.c src/beastSound.h
    src/cxxlog.cpp src/cxxlog.h
    src/edcrypt.c src/edcrypt.h
    src/funchelper.cpp src/funchelper.h
    src/shouyinw.cpp src/shouyinw.h
    src/shouyinw.ui
    src/usecode.cpp src/usecode.h
    #res.qrc
)

target_link_libraries(sy
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS sy
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_compile_definitions(DEFINES QT_MESSAGELOGCONTEXT)

qt_generate_deploy_app_script(
    TARGET sy
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
