#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"
#include "print.h"
#include <algorithm>

const char* VERSION = "0.0.1";


void splitAndInsert(const std::string& str, std::map<std::string, std::string>& userArgs) {
    size_t pos = str.find('=');
    if (pos != std::string::npos) {
        std::string key = str.substr(0, pos);
        std::string value = str.substr(pos + 1);
        value.erase(std::remove(value.begin(), value.end(), '"'), value.end());
        std::cout << key << " " << value << std::endl;
        userArgs[key] = value;
    }
}


int main(int argc, char *argv[]) {
    // This can be refactored into an argument parser class
    if (argc < 2) {
        std::cout << "Usage: grid <input file> <output file> [--debug]" << std::endl;
        return 1;
    }
    bool debug = false;

    auto inputFile = argv[1];
    auto outputFile = argv[2];
    auto userArgs = std::map<std::string, std::string>();

    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug = true;
        }
        if (std::string(argv[i]) == "--version") {
            std::cout << "version " << VERSION << std::endl; 
            return 0;
        } else {
            splitAndInsert(argv[i], userArgs);
        }
    }
    std::cout << userArgs.size() << std::endl;

    auto myDoc = laid::load_file(inputFile);
    myDoc->addVariables(userArgs);
    BuildPDF PDFBuilder(myDoc, outputFile, debug);
    PDFBuilder.Build();
    std::cout << "Done!" << std::endl;
}