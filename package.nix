{
  lib,
  stdenv,
  cmake,
  gtest,
  cxxStandard ? "20"
}:
stdenv.mkDerivation {
  pname = "cpp-extensions";
  version = "0.1.0";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [ cmake ];

  cmakeFlags = with lib; [
    (cmakeFeature "CMAKE_CXX_STANDARD" "${cxxStandard}")
	(cmakeBool "EXT_BUILD_TESTS" true)
	(cmakeBool "EXT_TEST_EXTERNAL_LIBS" false)
  ];

  doCheck = true;

  checkInputs = [ gtest ];
}
