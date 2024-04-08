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
    masterPage.height = 842;
    masterPage.marginTop = 24;
    masterPage.marginBottom = 24;
    masterPage.marginLeft = 24;
    masterPage.marginRight = 24;
    masterPage.cols = 4;
    masterPage.rows = 4;
    masterPage.gap = 12;

    auto page = laid::Page(masterPage);
    auto first = masterPage.getRect(1, 1);
    auto box = laid::Box(first.startX, first.startY, first.endX - first.startX, first.endY - first.startY);
    box.addText("Hello, World! This should hopefully wrap, like a burrito", style);
    page.addBox(box);
    doc.addPage(page);
    

    auto second = masterPage.getRect(2, 2);
    auto box2 = laid::Box(second.startX, second.startY, second.endX - second.startX, second.endY - second.startY);
    box2.addText("We meet again", style);
    page.addBox(box2);
    RenderPDF(doc);
}