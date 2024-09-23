conan install . -if builds/Debug -s build_type=Debug -pr:b=default -pr:h=default
conan build -if builds/Debug/ .
cd builds/Debug/
make install
./src/octopus/test/unit_tests
cd -
