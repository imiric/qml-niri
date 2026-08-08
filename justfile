all: build

build:
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build

install prefix="/usr":
  cmake --install build --prefix {{prefix}}

clean:
  rm -rf build

format:
  git ls-files -z '*.qml' \
    | xargs -0 -r -n 50 qmlformat -i
  git ls-files -z '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' \
    | xargs -0 -r -n 50 clang-format -i

test component:
  QML_IMPORT_PATH="$PWD/build" qml6 test/test_{{component}}.qml
