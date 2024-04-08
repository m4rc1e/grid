#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <map>
#include <vector>
#include <iostream>

namespace laid {


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


class TextRun {
    public:
        std::string text;
        ParagraphStyle style;
};


class Box {
    public:
        int x;
        int y;
        int width;
        int height;
        std::vector<TextRun> text_runs;

        Box(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
        void addText(std::string text, ParagraphStyle& style) {
            text_runs.push_back(TextRun{text, style});
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

} // namespace laid
#endif