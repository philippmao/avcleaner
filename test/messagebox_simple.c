#include <Windows.h>

int main(int argc, char** argv) {


    MessageBoxA(NULL, "Test", L"Something", MB_OK);
    MessageBoxA(NULL, L"Another test", L"Another something", MB_OK);
    return 0;
}
