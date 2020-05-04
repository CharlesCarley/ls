# -----------------------------------------------------------------------------
#
#   Copyright (c) 2019 Charles Carley.
#
#   This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
#   Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
# ------------------------------------------------------------------------------

# Utility project to organize build scripts better
if (MSVC)
    file(GLOB CM_FILES *.cmake)
    file(GLOB IN_FILES *.in)
    file(GLOB MD_FILES ../*.md)
    file(GLOB PY_FILES ../*.py)
    file(GLOB BC_FILES ../*.bat)

    set(CMAKE_FILES
        ${CM_FILES}
        ${IN_FILES}
        ${MD_FILES}
        ${PY_FILES}
        ${BAT_FILES}
        ../.clang-format
        ../.github/workflows/autobuild.yml
    )
	source_group("CMake"        FILES ${CM_FILES})
	source_group("Markdown"     FILES ${MD_FILES})
	source_group("Include"      FILES ${IN_FILES})
    source_group("Python"       FILES ${PY_FILES})
    source_group("Batch"        FILES ${BC_FILES})

    set_source_files_properties(
        ${CMAKE_FILES} 
        PROPERTIES 
        VS_TOOL_OVERRIDE 
        "None"
    )

    add_custom_target(ALL_CMAKE ALL SOURCES ${CMAKE_FILES})

endif()
