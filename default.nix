{
  version,
  lib,
  stdenv,
  cmake,
  qt6,
  patchelf,
}:

let
  qmlOutPath = "$out/${qt6.qtbase.qtQmlPrefix}";
in
stdenv.mkDerivation (finalAttrs: {
  pname = "qml-niri";
  inherit version;

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
    patchelf
  ];

  buildInputs = [
    qt6.qtbase
    qt6.qtdeclarative
  ];

  dontWrapQtApps = true;

  installPhase = ''
    mkdir -p ${qmlOutPath}
    cp -R Niri/ ${qmlOutPath}
  '';

  preFixup = ''
    libDir="${qmlOutPath}/Niri"
    for lib in $libDir/*.so; do
      patchelf --remove-rpath "$lib" || true
      patchelf --set-rpath "$libDir:${lib.makeLibraryPath finalAttrs.buildInputs}" "$lib" || true
    done
  '';
})
