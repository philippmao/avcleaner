WIN_INCLUDE="/home/vladimir/dev/anti-av/hkclnr/avcleaner"
CLANG_PATH="/usr/lib/clang/8.0.1/"
clang-query "$1" -- -Xclang -ast-dump -D "_WIN64" -D "_UNICODE" -D "UNICODE" -D "_WINSOCK_DEPRECATED_NO_WARNINGS"  -ferror-limit 500 -target x86_64-pc-windows-msvc19.15.26726\
  "-fsyntax-only" "-disable-free" "-disable-llvm-verifier" "-discard-value-names"\
  "-mrelocation-model" "pic" "-pic-level" "2" "-mthread-model" "posix" "-fmath-errno" \
  "-masm-verbose" "-mconstructor-aliases" "-munwind-tables" "-target-cpu" "x86-64" \
  "-dwarf-column-info" "-debugger-tuning=gdb" "-momit-leaf-frame-pointer" "-v"\
  "-resource-dir" "/usr/lib/clang/10.0.1/" \
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
  "-ferror-limit" "190" "-fmessage-length" "237" "-fno-use-cxa-atexit" "-fms-extensions" "-fms-compatibility" \
  "-fms-compatibility-version=19.15.26726" "-std=c++14" "-fdelayed-template-parsing" "-fobjc-runtime=gcc" "-fcxx-exceptions" "-fexceptions" "-fseh-exceptions" "-fdiagnostics-show-option" "-fcolor-diagnostics" "-x" "c++"

