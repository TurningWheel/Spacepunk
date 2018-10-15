# sets CHAISCRIPT_INCLUDE_DIR if found

FIND_PATH(CHAISCRIPT_INCLUDE_DIR  chaiscript/chaiscript.hpp
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local/include
    /usr/include
    /sw/include
    /opt/local/include
    /opt/csw/include
    /opt/include
    /usr/freeware/include
    /devel
)

set(CHAISCRIPT_FOUND "NO")

if(CHAISCRIPT_INCLUDE_DIR)
  set(CHAISCRIPT_FOUND "YES")
  message("Chaiscript Includes " ${CHAISCRIPT_INCLUDE_DIR})
endif(CHAISCRIPT_INCLUDE_DIR)
