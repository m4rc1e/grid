#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"
#include "print.h"

const char* VERSION = "0.0.1";

int main(int argc, char *argv[]) {
    // This can be refactored into an argument parser class
    bool debug = true;
    for (int i = 0; i < argc; i++) {
        if (std::string(argv[i]) == "--debug") {
            debug = true;
        }
        if (std::string(argv[i]) == "--version") {
            std::cout << "version " << VERSION << std::endl; 
            return 0;
        }
    }

    auto printSettings = PrintSettings();
    printSettings.paperWidth = 540;
    printSettings.paperHeight = 700;
    printSettings.composition = PrintSettings::Composition::Single;

    auto myDoc = laid::load_file("../promos/winnie_the_poo/book.xml");
    BuildPDF PDFBuilder(myDoc, "poo2.pdf", printSettings, debug);
    PDFBuilder.Build();
    std::cout << "Done!" << std::endl;
}