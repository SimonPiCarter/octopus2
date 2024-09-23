conan install . -if builds/Release -s build_type=Release -pr:b=default -pr:h=default
conan build -if builds/Release/ .
cd builds/Release/
make install
./src/octopus/test/unit_tests
cd -
