echo "Don't forget to update the path to your local winsdk"
./CMakeBuild/avcleaner.bin "$1" --strings=true --api=true -- -D "_WIN64" -D "_UNICODE" -D "UNICODE" -D "_WINSOCK_DEPRECATED_NO_WARNINGS"\
     "-I" "/usr/lib/clang/10.0.1/include" \
     "-I" "include/10.0.19041.0/winrt" \
     "-I" "include/10.0.19041.0/um" \
     "-I" "include/10.0.19041.0/ucrt" \
     "-I" "include/10.0.19041.0/shared" \
     "-I" "include/10.0.19041.0/cppwinrt" \
     "-I" "include/14.27.29110/include/" \
     "-w" \
     "-fdebug-compilation-dir"\
     "-fno-use-cxa-atexit" "-fms-extensions" "-fms-compatibility" \
     "-fms-compatibility-version=19.15.26726" "-std=c++14" "-fdelayed-template-parsing" "-fobjc-runtime=gcc" "-fcxx-exceptions" "-fexceptions" "-fdiagnostics-show-option" "-fcolor-diagnostics" "-x" "c++" -ferror-limit=1900 -target x86_64-pc-windows-msvc19.15.26726\
       "-fsyntax-only" "-disable-free" "-disable-llvm-verifier" "-discard-value-names"\
       "-dwarf-column-info" "-debugger-tuning=gdb" "-momit-leaf-frame-pointer" "-v"
