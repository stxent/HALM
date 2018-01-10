# Copyright (C) 2017 xent
# Project is distributed under the terms of the GNU General Public License v3.0

function(parse_config _config_file)
    file(STRINGS "${_config_file}" _config_data)

    foreach(_entry ${_config_data})
        string(FIND "${_entry}" "CONFIG_" _entry_is_config)
        if(${_entry_is_config} EQUAL 0)
            string(REGEX REPLACE "(CONFIG_[^=]+)=\"?([^\"]+)\"?" "\\1;\\2" _splitted_entry ${_entry})
            list(GET _splitted_entry 0 _entry_name)
            list(GET _splitted_entry 1 _entry_value)
            if("${_entry_value}" STREQUAL "y")
                set(${_entry_name} ON PARENT_SCOPE)
                add_definitions(-D${_entry_name})
            elseif("${_entry_value}" STREQUAL "n")
                set(${_entry_name} OFF PARENT_SCOPE)
            else()
                set(${_entry_name} "${_entry_value}" PARENT_SCOPE)
                # Entry values should not contain any special characters, symbols or spaces
                add_definitions(-D${_entry_name}=${_entry_value})
            endif()
        endif()
    endforeach()
endfunction()
