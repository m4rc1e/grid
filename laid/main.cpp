#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"


int main() {
    auto myDoc = laid::load_file("sketches/one_page_multi_textruns.xml");
    BuildPDF PDFBuilder(myDoc, "output3.pdf");
    PDFBuilder.Build();
}