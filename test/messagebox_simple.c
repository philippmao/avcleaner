#include <windows.h>


int test3(){
    int test;
    /*MessageBox(NULL, L"Test", L"Something", MB_OK);
    MessageBoxW(NULL, L"Test", L"Something", MB_OK);
    MessageBox(NULL, L"Test", L"Something", MB_OK);
    MessageBoxA(NULL, "Test", "Something", MB_OK);
    */
    return 0;
}

int main(int argc, char** argv) {

    int test;
    int what;

    MessageBox(NULL, L"Test", L"Something", MB_OK);
    MessageBox(NULL, L"Test", L"Something", MB_OK);
    MessageBoxW(NULL, L"Test", L"Something", MB_OK);
    MessageBoxA(NULL, "Test", "Something", MB_OK);
    test3();
    return 0;
}