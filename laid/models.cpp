#include <string>
#include <map>
#include <vector>
#include <iostream>

/*

Model test

Document():
    paragraph_styles <dict of para_styles>
    masterPages <dict of master_pages>
    page_count <int>
    pages <list of sections>
        boxes <list of text_boxes>

    addParagraphStyle(paragraphStyle)
    addMasterPage(masterPage)
    addPage(masterName)

MasterPagebuilder(name string):
       

Page(Document):
    addBox(x, y, width, height, text, style)
    addLinkedBoxes()

Box(Page):
    addText(text, style)


*/


class ParagraphStyle {
    public:
        std::string name;
};

class MasterPage {
    public:
        std::string name;
        int width;
        int height;
        int cols;
        int rows;
};


class Box {
    int x;
    int y;
    int width;
    int height;
    std::string text;
    ParagraphStyle style;

    void addText(std::string text, ParagraphStyle& style) {
        this->text = text;
        this->style = style;
    }
};


class Page {
    public: 
        Page(MasterPage masterPage) {
            this->masterPage = masterPage;
        }
        MasterPage masterPage;
        std::vector<Box> boxes;

        void addBox(Box box) {
            boxes.push_back(box);
        }
};


class Document {
    public:
        std::map<std::string, ParagraphStyle*> paragraph_styles;
        std::map<std::string, MasterPage*> masterPages;
        int page_count;
        std::vector<Page*> pages;

        void addParagraphStyle(ParagraphStyle& paragraphStyle) {
            paragraph_styles[paragraphStyle.name] = &paragraphStyle;
        }

        void addMasterPage(MasterPage& masterPage) {
            masterPages[masterPage.name] = &masterPage;
        }

        void addPage(Page& page) {
            pages.push_back(&page);
        }
};


// move this shit into another file
int main() {
    auto doc = Document();

    auto style = ParagraphStyle();
    style.name = "p1";
    doc.addParagraphStyle(style);

    auto masterPage = MasterPage();
    masterPage.width = 592;
    masterPage.height = 826;

    auto page = Page(masterPage);
    doc.addPage(page);
    
    std::cout << "done";

}