

macro(set_static_runtime)

    if(MSVC)

	    #We statically link to reduce dependancies
	    foreach(flag_var    CMAKE_CXX_FLAGS 
	                        CMAKE_CXX_FLAGS_DEBUG 
	                        CMAKE_CXX_FLAGS_RELEASE 
	                        CMAKE_CXX_FLAGS_MINSIZEREL 
	                        CMAKE_CXX_FLAGS_RELWITHDEBINFO
	                        CMAKE_C_FLAGS_DEBUG
	                        CMAKE_C_FLAGS_RELEASE
	                        CMAKE_C_FLAGS_MINSIZEREL 
	                        CMAKE_C_FLAGS_RELWITHDEBINFO
	                        )
		    if(${flag_var} MATCHES "/MD")
			    string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		    endif(${flag_var} MATCHES "/MD")
		    if(${flag_var} MATCHES "/MDd")
			    string(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
		    endif(${flag_var} MATCHES "/MDd")
	    endforeach(flag_var)



	    foreach(flag_var    CMAKE_CXX_FLAGS 
	                        CMAKE_CXX_FLAGS_DEBUG 
	                        CMAKE_CXX_FLAGS_RELEASE 
	                        CMAKE_CXX_FLAGS_MINSIZEREL 
	                        CMAKE_CXX_FLAGS_RELWITHDEBINFO
	                        CMAKE_C_FLAGS_DEBUG
	                        CMAKE_C_FLAGS_RELEASE
	                        CMAKE_C_FLAGS_MINSIZEREL 
	                        CMAKE_C_FLAGS_RELWITHDEBINFO
	                        )
            set(${flag_var} "${${flag_var}} -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
	    endforeach(flag_var)
	    
	    
    endif(MSVC)

endmacro(set_static_runtime)



macro(disable_crt_warnings)

    if(MSVC)

	    foreach(flag_var    CMAKE_CXX_FLAGS 
	                        CMAKE_CXX_FLAGS_DEBUG 
	                        CMAKE_CXX_FLAGS_RELEASE 
	                        CMAKE_CXX_FLAGS_MINSIZEREL 
	                        CMAKE_CXX_FLAGS_RELWITHDEBINFO
	                        CMAKE_C_FLAGS_DEBUG
	                        CMAKE_C_FLAGS_RELEASE
	                        CMAKE_C_FLAGS_MINSIZEREL 
	                        CMAKE_C_FLAGS_RELWITHDEBINFO
	                        )
            set(${flag_var} "${${flag_var}} -D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE")
	    endforeach(flag_var)
	    
	    
    endif(MSVC)

endmacro(disable_crt_warnings)

