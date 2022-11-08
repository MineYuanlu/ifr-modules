aux_source_directory(. DIR_LIB_SRCS)
add_library(plan ${DIR_LIB_SRCS})

include_directories(../../lib)
include_directories(..)