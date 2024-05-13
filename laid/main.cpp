#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"


int main() {
    auto myDoc = laid::load_file("sketches/multi_page_one_box.xml");
    BuildPDF PDFBuilder(myDoc, "output3.pdf");
    PDFBuilder.Build();
}