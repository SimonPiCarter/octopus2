add_executable(main src/main.cc)
add_executable(main2 src/main2.cc)

target_link_libraries(main octopus)
target_link_libraries(main2 octopus)

# ============================================================================
# Installation
# ============================================================================

install (TARGETS main EXPORT main-export)
install (TARGETS main2 EXPORT main2-export)
