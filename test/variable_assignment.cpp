using namespace std;
#include<string>
#include <stdio.h>
#include <iostream>

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

    
    const char* test = "Hank stop riding the perfect man!";

    printf(test);

    wchar_t* test2 = L"I gotta admit I always wanted to get Edgar Allan Poe in a headlock. That thing is like a pumpkin!";

    wprintf(test2);

    static const char test3[] = "Come on dad! I’m going on a date with… the Venture brothers.";

    printf(test3);
    
    //TODO: add handling for std::string
    std::string test4 = "What is going on here, gremlin?";

    std::cout << test4;

    std::wstring test5 = L"While you were wasting your time castrating a priceless antique, I was systematically feeding babies to hungry mutated puppies!";

    std::wcout << test5;

    return 0;
}