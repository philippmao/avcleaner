//
// Created by Vladimir on 20.06.20.
//

#include "Utils.h"
#include <iostream>
#include <regex>
#include <random>
#include <sstream>
#include <vector>
#include <clang/AST/CommentLexer.h>

using namespace Utils;

std::string Utils::randomString(std::string::size_type Length) {

    static auto &chrs = "0123456789"
                        "abcdefghijklmnopqrstuvwxyz"
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    thread_local static std::mt19937 rg{std::random_device{}()};
    thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

    std::string s;

    s.reserve(Length);

    while (Length--)
        s += chrs[pick(rg)];

    return s;
}

std::string Utils::translateStringToIdentifier(const std::string &StrLiteral) {

    std::string NewIdentifier = std::regex_replace(StrLiteral, std::regex("[^A-Za-z]"), "_");
    return "hid_" + NewIdentifier.substr(0, 6) + '_' + randomString(12);
}


void Utils::cleanParameter(std::string &Argument) {

    auto Index = Argument.find_first_of('\"');

    Argument.erase(Argument.begin(), Argument.begin() + Index + 1);

    if (Argument.back() == '\"') {
        Argument.pop_back();
    }
}


std::string
Utils::generateVariableDeclaration(const std::string &StringIdentifier, const std::string &StringValue, std::string StringType) {

    //generate Ciphertext
    int key = 69; //TODO: randomly generate key
    auto PlainText = std::string(StringValue);
    cleanParameter(PlainText);
    std::vector <int> CipherText;
    for(std::string::iterator it = PlainText.begin(); it != PlainText.end(); it++) {
        int nb = (int)*it & 0xff;
        CipherText.push_back(key ^ nb);
        // HACK: This is now only ASCII compatible (all characters > 256 are ignored), the StringType should be bytes TODO
        if(StringType.find("wchar") != std::string::npos){
            it++;
        }
    }
    CipherText.push_back(key ^ 0);

    std::stringstream Result;

    //Result << "\n#ifdef _UNICODE\n\twchar_t\n";
    //Result << "#else\n\tchar\n#endif\n\t";
    if(!StringType.empty()){
        auto pos = StringType.find('*');
        if (pos != std::string::npos)
            StringType.erase(pos);

        Result << StringType << " " << StringIdentifier;
        /*if (StringType.find("char") != std::string::npos && StringType.find("*") == std::string::npos) {
        }*/
        Result << "[]";

        Result << " = {";
    } else {
        llvm::outs() << StringValue <<  " Oups\n";

        Result << "TCHAR " << StringIdentifier << "[] = {";
    }

    //
    for(std::vector<int>::size_type i = 0; i != CipherText.size(); i++) {
        Result << "'\\x" << std::hex << CipherText[i] << "'";
        if(i+1 != CipherText.size()){
            Result << ",";
        }
    }
    Result << "};\n";

    //generate variable declaration with ciphertext string
    /*
    auto CleanString = CipherText.str();
    for (std::string::iterator it = CleanString.begin(); it != CleanString.end(); it++) {

        if (*it != 0) {
            int nb = (int)*it & 0xff;
            Result << "'\\x" << std::hex << nb << "'";
        } else {
            continue;
        }

        uint32_t offset = 1;
        if (it + offset != CleanString.end()) {
            Result << ",";
        }
    }
    if (*Result.str().end() == ',')
        Result << "0};\n";
    else
        Result << ",0};\n";
    */

    
    

    //generate decryption routine
    /*
    char randomly_generated_keyname = keyvalue maybe a hash of the variable name
    
    for(char &x : variable name){
        x = x ^ key;
    }

    */
    std::string keyname = "k_" + randomString(12);
    int key_value = (int)key & 0xff;
    Result << StringType << " " << keyname << " = " << "'\\x" << std::hex << key_value << "'" << ";\n";
    Result << "for(" << StringType <<" &x : " << StringIdentifier << "){\n";
    Result << "x = x ^ " << keyname << ";\n";
    Result << "}\n";

    // Don't understand what this is for, error correction maybe ?
    return std::regex_replace(Result.str(), std::regex(",,"), ",");
}

