#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"



int main(int argc, char *argv[]) {
    // This can be refactored into an argument parser class
    bool debug = true;
    for (int i = 0; i < argc; i++) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
        if (std::string(argv[i]) == "--debug") {
            debug = true;
        }
    }
    auto myDoc = laid::load_file("../sketches/box_flow.xml");
    BuildPDF PDFBuilder(myDoc, "output3.pdf", debug);
    PDFBuilder.Build();
    std::cout << "Done!" << std::endl;
}