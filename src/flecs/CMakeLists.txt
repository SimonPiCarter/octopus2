
include(FetchContent)

FetchContent_Declare(
        flecs
        GIT_REPOSITORY https://github.com/SanderMertens/flecs
        GIT_TAG v4.1.0
)

FetchContent_MakeAvailable(flecs)
