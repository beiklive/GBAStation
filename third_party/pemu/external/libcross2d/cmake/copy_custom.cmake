cmake_minimum_required(VERSION 3.10)

# copy only if source exist

if (IS_DIRECTORY "${SRC}")
    message(STATUS "copying \"${SRC}\" directory to \"${DST}\"...")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_directory ${SRC} ${DST})
else ()
    file(GLOB FILES "${SRC}")
    foreach (F ${FILES})
        file(COPY ${F} DESTINATION ${DST})
        message(STATUS "copying \"${F}\" file to \"${DST}\"...")
    endforeach ()
endif ()
