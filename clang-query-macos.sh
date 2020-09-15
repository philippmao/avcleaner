WIN_INCLUDE="/Users/vladimir/dev/anti-av/hkclnr/avcleaner"
CLANG_PATH="/usr/local/Cellar/llvm/9.0.1"#"/usr/lib/clang/8.0.1/"

clang -cc1 -ast-dump "$1" -D "_WIN64" -D "_UNICODE" -D "UNICODE" -D "_WINSOCK_DEPRECATED_NO_WARNINGS"\
  "-I" "/usr/lib/clang/10.0.1/include" \
     "-I" "include/10.0.19041.0/winrt" \
     "-I" "include/10.0.19041.0/um" \
     "-I" "include/10.0.19041.0/ucrt" \
     "-I" "include/10.0.19041.0/shared" \
     "-I" "include/10.0.19041.0/cppwinrt" \
     "-I" "include/14.27.29110/include/" \
  "-fdeprecated-macro" \
  "-w" \
  "-fdebug-compilation-dir"\
  "-fno-use-cxa-atexit" "-fms-extensions" "-fms-compatibility" \
  "-fms-compatibility-version=19.15.26726" "-std=c++14" "-fdelayed-template-parsing" "-fobjc-runtime=gcc" "-fcxx-exceptions" "-fexceptions" "-fseh-exceptions" "-fdiagnostics-show-option" "-fcolor-diagnostics" "-x" "c++"

