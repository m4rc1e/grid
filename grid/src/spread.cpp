#include "models.h"
#include "pdfview.h"

int main() {

    auto doc = std::make_shared<laid::Document>();
    laid::MasterPage master;;
    master.width = 300;
    master.height = 300;
    master.name = std::string("master");
    master.baseline = 0;
    master.rows = 3;
    master.cols = 3;
    master.marginTop = 10;
    auto spread = std::make_shared<laid::Spread>(master, master);
    spread->overflow = true;
    auto box1 = std::make_shared<laid::Box>(0, 10, 100, 100);
    auto box2 = std::make_shared<laid::Box>(400, 10, 100, 100);
    auto box3 = std::make_shared<laid::Box>(200, 100, 300, 100);

    box1->next = box2;
    box2->prev2 = box1;

    spread->addBox(box1);
    spread->addBox(box2);
    spread->addBox(box3);
    
    auto style = std::make_shared<laid::Style>();
    style->name = "p";
    style->fontName = "Inter";
    style->fontSize = 12;

    auto para = std::make_shared<laid::Paragraph>();
    box1->addParagraph(para);

    auto para2 = std::make_shared<laid::Paragraph>();
    box3->addParagraph(para2);
    std::string text = "Hello world. this is going to overflow into the next box so let's get ready! Keep adding more text and we should hit it hard! We need to add even more text in the hope that it will also overflow into another spread and not just into a single but but we need a whole spread when the food happens we all need some epic fucking bacon in our lives let it go let it go. Let's keep pushing this back into the motherfucking stratosphere and then we'lll see who is talking about who on the big stage, not the listtle stage";
    std::string styleName = "p";
    std::string header = "Millwall F.C is one of the greatest clubs since they love to fight";
    para2->addText(header, styleName);
    
    para->addText(text, styleName);

    doc->addStyle(*style);
    doc->addSpread(spread);
    std::cout << doc->page_count << std::endl;

    BuildPDF PDFBuilder(doc, "output4.pdf", true);
    PDFBuilder.Build();

    return 0;
}