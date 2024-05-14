#include <iostream>
#include "models.h"
#include "pdfview.h"
#include "xmlparser.h"


int main() {
    auto myDoc = laid::load_file("sketches/multi_page_two_col.xml");
    BuildPDF PDFBuilder(myDoc, "output3.pdf");
    PDFBuilder.Build();
}