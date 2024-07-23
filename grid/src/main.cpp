#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"
#include "print.h"

const char* VERSION = "0.0.1";

int main(int argc, char *argv[]) {
    // This can be refactored into an argument parser class
    if (argc < 3) {
        std::cout << "Usage: grid <input file> <output file> [--debug]" << std::endl;
        return 1;
    }
    bool debug = false;
    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug = true;
        }
        if (std::string(argv[i]) == "--version") {
            std::cout << "version " << VERSION << std::endl; 
            return 0;
        }
    }

    auto inputFile = argv[1];
    auto outputFile = argv[2];

    auto myDoc = laid::load_file(inputFile);
    BuildPDF PDFBuilder(myDoc, outputFile, debug);
    PDFBuilder.Build();
    std::cout << "Done!" << std::endl;
}