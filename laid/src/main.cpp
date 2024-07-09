#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"
#include "print.h"


int main(int argc, char *argv[]) {
    // This can be refactored into an argument parser class
    bool debug = true;
    for (int i = 0; i < argc; i++) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
        if (std::string(argv[i]) == "--debug") {
            debug = true;
        }
    }

    auto printSettings = PrintSettings();
    printSettings.paperWidth = 2000;
    printSettings.paperHeight = 2000;
    printSettings.composition = PrintSettings::Composition::Spreads;

    auto myDoc = laid::load_file("../sketches/demo.xml");
    BuildPDF PDFBuilder(myDoc, "demo.pdf", printSettings, debug);
    PDFBuilder.Build();
    std::cout << "Done!" << std::endl;
}