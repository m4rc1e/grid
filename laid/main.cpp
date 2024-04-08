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

    auto page = laid::Page(masterPage);
    auto box = laid::Box(24, 24, 400, 200);
    box.addText("Hello, World! ", style);
    box.addText("New order", style);
    page.addBox(box);
    doc.addPage(page);
    

    auto box2 = laid::Box(200, 200, 200, 200);
    box2.addText("We meet again", style);
    page.addBox(box2);
    RenderPDF(doc);
}