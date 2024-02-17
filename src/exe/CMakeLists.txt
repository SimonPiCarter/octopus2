add_executable(main src/main.cc)

target_link_libraries(main octopus)

# ============================================================================
# Installation
# ============================================================================

install (TARGETS main EXPORT main-export)
