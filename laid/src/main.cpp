#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"


// We should be thinking about saddle stiching as well.
// Currently, only perfect binding works
struct PrintSettings {
    // A4 by default. Maybe it should be US Letter
    float paperWidth = 595;
    float paperHeight = 842;
    bool cropMarks = false;
    bool spreads = false;
};


int main(int argc, char *argv[]) {
    // This can be refactored into an argument parser class
    bool debug = true;
    for (int i = 0; i < argc; i++) {
        std::cout << "Argument " << i << ": " << argv[i] << std::endl;
        if (std::string(argv[i]) == "--debug") {
            debug = true;
        }
    }
    auto myDoc = laid::load_file("../sketches/multi_page_two_col.xml");
    BuildPDF PDFBuilder(myDoc, "output3.pdf", debug);
    PDFBuilder.Build();
    std::cout << "Done!" << std::endl;
}