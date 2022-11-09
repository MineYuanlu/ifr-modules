aux_source_directory(. DIR_LIB_SRCS)
aux_source_directory(../../lib/mongoose DIR_M_SRCS)
add_library(api ${DIR_LIB_SRCS} ${DIR_M_SRCS})

include_directories(../../lib)
include_directories(..)