#include <iostream>
#include "models.h"
#include "pdfview.h"


int main() {
    auto doc = laid::Document();

    auto style = laid::ParagraphStyle();
    style.name = "p1";
    doc.addParagraphStyle(style);

    auto masterPage = laid::MasterPage();
    masterPage.width = 592;
    masterPage.height = 826;

    auto masterPage2 = laid::MasterPage();
    masterPage2.width = 200;
    masterPage2.height = 200;

    auto page = laid::Page(masterPage);
    auto page2 = laid::Page(masterPage2);
    doc.addPage(page);
    doc.addPage(page2);
    RenderPDF(doc);

}