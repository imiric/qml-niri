all: build

build:
  cmake -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build

install prefix="/usr":
  cmake --install build --prefix {{prefix}}

clean:
  rm -rf build

test component:
  QML_IMPORT_PATH="$PWD/build" qml6 test/test_{{component}}.qml
