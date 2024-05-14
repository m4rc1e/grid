#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"


int main() {
    auto myDoc = laid::load_file("sketches/auto_leading.xml");
    BuildPDF PDFBuilder(myDoc, "output3.pdf");
    PDFBuilder.Build();
}