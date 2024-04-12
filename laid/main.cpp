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
    masterPage.marginLeft = 48;
    masterPage.marginRight = 36;
    masterPage.cols = 4;
    masterPage.rows = 4;
    masterPage.gap = 12;

    auto page = std::make_shared<laid::Page>(masterPage);
    auto first = masterPage.getRect(1, 1);
    auto box = std::make_shared<laid::Box>(first.startX, first.startY, first.endX - first.startX, first.endY - first.startY);
    box->addText("Hello, World! This should hopefully wrap, like a burrito. I'm hoping that this demo of a land filled site is just perfect for me and you. Listening to Donato dozzy is a welcome releif. I still need to keep going but this work will eventually pay off one day far into the future into a new land of hope and promise. We are getting very very close to the hit point and I cannot wait for it.", style);

    auto second = masterPage.getRect(2, 1);
    auto box2 = std::make_shared<laid::Box>(second.startX, second.startY, second.endX - second.startX, second.endY - second.startY);
    box->next = box2;
    page->addBox(box);
    page->addBox(box2);

    auto third = masterPage.getRect(1, 0);
    auto fouth = masterPage.getRect(2, 0);
    auto pic_box = std::make_shared<laid::Box>(third.startX, third.startY, fouth.endX - third.startX, third.endY - third.startY);
    pic_box->addImage("image.png");
    page->addBox(pic_box);
    

    doc.addPage(page);
    


    

    RenderPDF(doc);
}