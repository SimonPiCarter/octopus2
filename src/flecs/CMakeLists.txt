
include(FetchContent)

FetchContent_Declare(
        flecs
        GIT_REPOSITORY https://github.com/SanderMertens/flecs
        GIT_TAG v3.2.11
)

FetchContent_MakeAvailable(flecs)
