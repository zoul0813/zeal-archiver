# Called by: find_package(ZAR REQUIRED)

include("${CMAKE_CURRENT_LIST_DIR}/zar_init.cmake")

set(libraries zar)

foreach(lib ${libraries})
    add_library(${lib} INTERFACE IMPORTED)

    set_target_properties(${lib} PROPERTIES
        IMPORTED_LIBNAME "${lib}"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../include"
        INTERFACE_LINK_DIRECTORIES "${CMAKE_CURRENT_LIST_DIR}/../lib"
    )
endforeach()
