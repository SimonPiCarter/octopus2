
include(FetchContent)

FetchContent_Declare(
        flecs
        GIT_REPOSITORY https://github.com/SanderMertens/flecs
        GIT_TAG v4.0.1
)

FetchContent_MakeAvailable(flecs)
