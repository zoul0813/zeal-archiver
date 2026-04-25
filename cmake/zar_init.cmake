if (NOT DEFINED ZAR_DIR)
    set(ZAR_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

function(zar_create target)
    set(target_name ${ARGV0})
    list(REMOVE_AT ARGV 0)

    set(ZAR_INPUT)
    set(ZAR_OUTPUT)
    set(ZAR_HEADER_SET FALSE)
    set(ZAR_HEADER)
    set(current_key)

    foreach(arg IN LISTS ARGV)
        if(arg STREQUAL "VERBOSE" OR arg STREQUAL "EXTRACT" OR arg STREQUAL "LIST")
            unset(current_key)
        elseif(arg STREQUAL "INPUT" OR arg STREQUAL "OUTPUT" OR arg STREQUAL "HEADER")
            set(current_key ${arg})
            if(arg STREQUAL "HEADER")
                set(ZAR_HEADER_SET TRUE)
            endif()
        elseif(current_key STREQUAL "INPUT")
            set(ZAR_INPUT ${arg})
            unset(current_key)
        elseif(current_key STREQUAL "OUTPUT")
            set(ZAR_OUTPUT ${arg})
            unset(current_key)
        elseif(current_key STREQUAL "HEADER")
            set(ZAR_HEADER ${arg})
            unset(current_key)
        else()
            message(FATAL_ERROR "Unknown zar_create argument: ${arg}")
        endif()
    endforeach()

    if(NOT ZAR_INPUT)
        message(FATAL_ERROR "zar_create requires INPUT")
    endif()
    if(NOT ZAR_OUTPUT)
        message(FATAL_ERROR "zar_create requires OUTPUT")
    endif()

    if(NOT Python3_EXECUTABLE)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    endif()

    if(IS_ABSOLUTE "${ZAR_INPUT}")
        set(input_abs "${ZAR_INPUT}")
    else()
        set(input_abs "${CMAKE_BINARY_DIR}/${ZAR_INPUT}")
    endif()

    if(IS_ABSOLUTE "${ZAR_OUTPUT}")
        set(output_abs "${ZAR_OUTPUT}")
    elseif(CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(output_abs "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${ZAR_OUTPUT}")
    else()
        set(output_abs "${CMAKE_CURRENT_BINARY_DIR}/${ZAR_OUTPUT}")
    endif()

    set(generated_outputs "${output_abs}")
    set(header_arg)
    set(header_dir)
    if(ZAR_HEADER_SET)
        if(NOT ZAR_HEADER)
            if(IS_ABSOLUTE "${ZAR_INPUT}")
                set(header_base_dir "${ZAR_INPUT}")
            else()
                set(header_base_dir "${CMAKE_SOURCE_DIR}/${ZAR_INPUT}")
            endif()
            set(ZAR_HEADER "${header_base_dir}/${target_name}.h")
        elseif(NOT IS_ABSOLUTE "${ZAR_HEADER}")
            set(ZAR_HEADER "${CMAKE_SOURCE_DIR}/${ZAR_HEADER}")
        endif()

        list(APPEND header_arg -c "${ZAR_HEADER}")
        list(APPEND generated_outputs "${ZAR_HEADER}")
        get_filename_component(header_dir "${ZAR_HEADER}" DIRECTORY)
    endif()

    file(GLOB zar_input_entries CONFIGURE_DEPENDS "${input_abs}/*")
    set(zar_input_files)
    foreach(input_entry IN LISTS zar_input_entries)
        if(NOT IS_DIRECTORY "${input_entry}")
            list(APPEND zar_input_files "${input_entry}")
        endif()
    endforeach()

    get_filename_component(output_dir "${output_abs}" DIRECTORY)
    get_filename_component(output_name_we "${output_abs}" NAME_WE)
    string(MAKE_C_IDENTIFIER "${output_name_we}" output_name_safe)
    set(custom_target_name "${target_name}_${output_name_safe}_zar")
    set(make_header_dir_command)
    if(header_dir)
        set(make_header_dir_command COMMAND ${CMAKE_COMMAND} -E make_directory "${header_dir}")
    endif()

    add_custom_command(
        OUTPUT ${generated_outputs}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${output_dir}"
        ${make_header_dir_command}
        COMMAND ${Python3_EXECUTABLE} "${ZAR_DIR}/../zar.py"
                -i "${input_abs}"
                -o "${output_abs}"
                ${header_arg}
        DEPENDS ${zar_input_files} "${ZAR_DIR}/../zar.py"
        COMMENT "Creating ZAR archive ${output_name_we}"
        VERBATIM
    )

    add_custom_target(${custom_target_name} ALL DEPENDS ${generated_outputs})
    add_dependencies(${target_name} ${custom_target_name})
    if(header_dir)
        target_include_directories(${target_name} PRIVATE "${header_dir}")
    endif()
endfunction()
