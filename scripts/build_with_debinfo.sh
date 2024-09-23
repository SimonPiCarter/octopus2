conan install . -if builds/RelWithDebInfo -s build_type=RelWithDebInfo -pr:b=default -pr:h=default --build=missing
conan build -if builds/RelWithDebInfo/ . && cd builds/RelWithDebInfo/ && make install && ./src/octopus/test/unit_tests
cd -
