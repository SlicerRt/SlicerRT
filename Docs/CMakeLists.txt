project(SlicerRt_documentation)

#set(CMAKE_VERBOSE_MAKEFILE ON) # set to ON for debugging; or rather use --verbose when calling cmake
cmake_minimum_required(VERSION 3.10)
if(CMAKE_VERSION VERSION_LESS 3.21)
    get_directory_property(hasParent PARENT_DIRECTORY)
    if(hasParent)
        set(builtin_docu ON)
    else()
        set(builtin_docu OFF)
    endif()
else()
    if(PROJECT_IS_TOP_LEVEL)
      set(builtin_docu OFF)
    else()
      set(builtin_docu ON)
    endif()
endif()

if(builtin_docu)
  message(STATUS "SlicerRt HTML documentation will be built with doxygen as a step of the SlicerRt build.")
  # TODO: https://stackoverflow.com/questions/16408060/how-do-i-add-a-dependency-on-a-script-to-a-target-in-cmake
else()
  message(STATUS "SlicerRt HTML documentation will be built with doxygen as a standalone.")
  # A typical way is:
  # cd /tmp/ && mkdir builddocu && cd builddocu  cmake -B./ -S/opt/SlicerRt/Docs/ -DDOCU_LOCATION=/tmp/docoutput -DDOCU_THREADS=8 -DDOCU_LOGFILE=/tmp/warnings.txt -DDOXYGEN_EXECUTABLE=/opt/doxygen/build/bin/doxygen -DDOCU_QHG_LOCATION=qhelpgenerator-qt5
endif()

if(NOT DOXYGEN_EXECUTABLE)
    set(DOXYGEN_EXECUTABLE "doxygen")
endif()
set(DOXYGEN_EXECUTABLE ${DOXYGEN_EXECUTABLE} CACHE STRING "Path to custom doxygen executable") # Use e.g. -DDOXYGEN_EXECUTABLE=/opt/doxygen/build/bin/doxygen for a local built doxygen. Leave empty for system-wide doxygen.
#list(APPEND CMAKE_MODULE_PATH "/opt/doxygen") # Uncomment this if you have a custom installation of Doxygen in /opt/
find_package(Doxygen 1.9.1 REQUIRED dot) # OPTIONAL_COMPONENTS dot

if(GIT_DESCRIBE_ALL)
    set (SLICERRT_GIT_VERSION ${GIT_DESCRIBE_ALL})
else()
    message(STATUS "Cannot determine git tag.")
endif()

#execute_process(COMMAND find ../ -type d -name dictpch -prune -o -type d -name inc OUTPUT_VARIABLE INC_FOLDERS OUTPUT_STRIP_TRAILING_WHITESPACE WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
#string(REPLACE "\n" ";" INC_FOLDERS ${INC_FOLDERS})
#list(APPEND INC_FOLDERS ../xxx/include)#different pattern


set (DOCU_QHG_LOCATION qhelpgenerator CACHE STRING "Path to qhelpgenerator")
#Pass instead -DDOCU_QHG_LOCATION=/usr/lib/x86_64-linux-gnu/qt4/bin/qhelpgenerator if qt4dev-tools or qt5/ if qt5-assistant
#Pass instead -DDOCU_QHG_LOCATION=/opt/Qt/5.15.2/gcc_64/bin/qhelpgenerator e.g. if custom installation
#or qhelpgenerator-qt5
set (DOCU_LOCATION $ENV{HOME}/SlicerRT_doc CACHE STRING "Documentation output directory")
#Pass instead e.g. -DDOCU_LOCATION=/tmp/SlicerRT_doc if system-wide installation
set (DOCU_THREADS 0 CACHE STRING "Number of threads for building the documentation (use 0 for maximum threads, N for specific number)")
set (DOCU_LOGFILE "" CACHE STRING "File where to write warnings to (leave empty for printing into console)")

find_program(QHELPGENERATOR "qhelpgenerator")
if(QHELPGENERATOR)
    message(STATUS "Doxygen QCH file will be built with ${DOCU_QHG_LOCATION}")
else()
    message(WARNING "Doxygen QCH file cannot be generated, no valid qhelpgenerator was found")
endif()

# TODO run -x_noenv to see what's missing
set (DOCU_INPUT ../
     CACHE STRING "Doxyfile input files or folders")

set(DOXYGEN_OUTPUT_DIRECTORY       "${DOCU_LOCATION}")
set(DOXYGEN_PROJECT_NAME           "SlicerRT")
set(DOXYGEN_PROJECT_NUMBER         "${SLICERRT_GIT_VERSION}")
set(DOXYGEN_PROJECT_BRIEF          "Code Documentation")
#set(DOXYGEN_PROJECT_LOGO           "logo.gif")
set(DOXYGEN_ALWAYS_DETAILED_SEC    YES)
set(DOXYGEN_JAVADOC_AUTOBRIEF      YES)
set(DOXYGEN_QT_AUTOBRIEF           YES)
set(DOXYGEN_TAB_SIZE               2)
#set(DOXYGEN_EXTENSION_MAPPING      h=C++ icc=C++ def=C++ pyzdoc=C++ cu=C++ cuh=C++)
set(DOXYGEN_TOC_INCLUDE_HEADINGS   3)
set(DOXYGEN_BUILTIN_STL_SUPPORT    YES)
set(DOXYGEN_LOOKUP_CACHE_SIZE      4)
set(DOXYGEN_EXTRACT_ALL            YES)
set(DOXYGEN_EXTRACT_PRIVATE        YES)
set(DOXYGEN_EXTRACT_STATIC         YES)
#set(DOXYGEN_EXTRACT_ANON_NSPACES   YES)
set(DOXYGEN_HIDE_IN_BODY_DOCS      YES)
set(DOXYGEN_CASE_SENSE_NAMES       YES) # default value is system dependent, so need to set this !
set(DOXYGEN_WARN_NO_PARAMDOC       NO) # If you set this to yes for finding all missing documentation, you need to set EXTRACT_ALL to false, see https://github.com/doxygen/doxygen/issues/880
set(DOXYGEN_SORT_BRIEF_DOCS        YES)
set(DOXYGEN_SORT_MEMBERS_CTORS_1ST YES)
set(DOXYGEN_GENERATE_TODOLIST      NO)
set(DOXYGEN_GENERATE_BUGLIST       NO)
#set(DOXYGEN_LAYOUT_FILE            DoxygenLayout.xml)
set(DOXYGEN_WARN_LOGFILE           ${DOCU_LOGFILE})
set(DOXYGEN_WARN_LINE_FORMAT       "$file:$line")
#set(DOXYGEN_FILE_PATTERNS          *.c *.C *.cc *.cpp *.cxx *.def *.dox *.f *.h *.hh *.hpp *.hxx *.icc *.inc *.inl *.js *.m *.md *.mm *.py *.pyzdoc)
set(DOXYGEN_RECURSIVE YES)
#set(DOXYGEN_EXCLUDE_PATTERNS       )
set(DOXYGEN_SOURCE_BROWSER         YES)
set(DOXYGEN_STRIP_CODE_COMMENTS    NO)
set(DOXYGEN_HTML_TIMESTAMP         YES)
if(QHELPGENERATOR)
set(DOXYGEN_GENERATE_QHP           YES)
set(DOXYGEN_QCH_FILE               "SlicerRT.qch")
set(DOXYGEN_QHP_NAMESPACE          Slicer.SlicerRT)
set(DOXYGEN_QHG_LOCATION           "${DOCU_QHG_LOCATION}")
set(DOXYGEN_QHP_VIRTUAL_FOLDER     "SlicerRTdoc")
endif()
set(DOXYGEN_DISABLE_INDEX          YES)
set(DOXYGEN_GENERATE_TREEVIEW      YES)
set(DOXYGEN_USE_MATHJAX            YES)
#set(DOXYGEN_MATHJAX_RELPATH        ../mathjax)
#set(DOXYGEN_MATHJAX_VERSION        MathJax_3)
set(DOXYGEN_GENERATE_LATEX         NO)
#set(DOXYGEN_LATEX_CMD_NAME        latex)
set(DOXYGEN_MACRO_EXPANSION        YES)
set(DOXYGEN_SKIP_FUNCTION_MACROS   NO)
#set(DOXYGEN_EXPAND_ONLY_PREDEF    YES)
#set(DOXYGEN_SEARCH_INCLUDES       NO)
set(DOXYGEN_PREDEFINED             "")
#set(DOXYGEN_INCLUDE_PATH           "${INC_FOLDERS}")
set(DOXYGEN_STRIP_FROM_PATH        "${DOCU_LOCATION}" "../../")
#set(DOXYGEN_STRIP_FROM_INC_PATH    "${INC_FOLDERS}")
set(DOXYGEN_GENERATE_TAGFILE       "${DOCU_LOCATION}/html/SlicerRT.tag")
set(DOXYGEN_HIDE_UNDOC_RELATIONS   NO)
set(DOXYGEN_HAVE_DOT               YES) # set to NO for getting faster results when debugging
set(DOXYGEN_DOT_MULTI_TARGETS      YES)
set(DOXYGEN_DOT_NUM_THREADS        ${DOCU_THREADS})
set(DOXYGEN_NUM_PROC_THREADS       ${DOCU_THREADS})
set(DOXYGEN_GROUP_GRAPHS           NO)
set(DOXYGEN_DOT_IMAGE_FORMAT       svg)
set(DOXYGEN_INTERACTIVE_SVG        YES)
set(DOXYGEN_DOT_GRAPH_MAX_NODES    200)
#set(DOXYGEN_ENABLED_SECTIONS       HIDDEN_SYMBOLS) # uncomment this to also show in the HTML the internal documentation inteded only for SlicerRT developers
#set(DOXYGEN_CLANG_ASSISTED_PARSING YES)

#TODO: if(NOT GRAPHVIZ_FOUND) set(gvizveto...)

doxygen_add_docs(dox ALL
  ${DOCU_INPUT}
  #ALLOW_DUPLICATE_CUSTOM_TARGETS
  #USE_STAMP_FILE
  COMMENT "Generating doxygen documentation for ${PROJECT_NAME}"
)

if(COMMAND ${DOCU_QHG_LOCATION})
add_custom_target(Zipping ALL
    COMMAND gzip -f ${DOCU_LOCATION}/html/SlicerRT.tag
    COMMAND gzip -f ${DOCU_LOCATION}/html/SlicerRT.qch
)
add_dependencies(Zipping dox)
endif()

set_property(
  TARGET dox
  APPEND
  PROPERTY ADDITIONAL_CLEAN_FILES Doxyfile.dox filter
)

