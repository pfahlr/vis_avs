add_library(avs-core STATIC
  src/core.cpp
)
target_include_directories(avs-core PUBLIC include)
target_compile_options(avs-core PRIVATE -Wall -Wextra -Werror)
