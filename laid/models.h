#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <tuple>


namespace laid {


class ParagraphStyle {
    public:
        std::string name;
};

class Rect {
    public:
        float startX;
        float startY;
        float endX;
        float endY;
};

class MasterPage {
    public:
        std::string name;
        float width;
        float height;
        int cols;
        int rows;
        float gap;
        float marginTop;
        float marginBottom;
        float marginLeft;
        float marginRight;
    
        Rect getRect(int col, int row) {
            auto colWidth = (width - marginLeft - marginRight - gap) / cols;
            auto rowHeight = (height - marginTop - marginBottom - gap) / rows;

            auto startX = marginLeft + (colWidth * col) + ((gap/2) * col);
            auto endX = startX + colWidth - (gap/2);
            auto startY = marginTop + (rowHeight * row) + ((gap/2) * row);
            auto endY = startY + rowHeight - (gap/2);
            return Rect{startX, startY, endX, endY};
        }

};


class TextRun {
    public:
        std::string text;
        ParagraphStyle style;
};


class Box {
    public:
        float x;
        float y;
        float width;
        float height;
        std::vector<TextRun> text_runs;
        std::shared_ptr<Box> next;
        std::string image_path;
        std::map<int, std::vector<std::shared_ptr<Box>>> children;

        Box(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
        void addText(std::string text, ParagraphStyle& style) {
            text_runs.push_back(TextRun{text, style});
        }

        void addImage(std::string path) {
            image_path = path;
        }

        void addChild(int idx, std::shared_ptr<Box> box) {
            children[idx].push_back(box);
        }
};


class Page {
    public: 
        Page(MasterPage& masterPage) {
            this->masterPage = masterPage;
        }
        MasterPage masterPage;
        std::vector<std::shared_ptr<Box>> boxes;

        void addBox(std::shared_ptr<Box> box) {
            boxes.push_back(std::shared_ptr<Box>(box));
        }
};


class Document {
    public:
        std::map<std::string, ParagraphStyle*> paragraph_styles;
        std::map<std::string, MasterPage*> masterPages;
        int page_count;
        std::vector<std::shared_ptr<Page>> pages;

        void addParagraphStyle(ParagraphStyle& paragraphStyle) {
            paragraph_styles[paragraphStyle.name] = &paragraphStyle;
        }

        void addMasterPage(MasterPage& masterPage) {
            masterPages[masterPage.name] = &masterPage;
        }

        void addPage(std::shared_ptr<Page> page) {
            pages.push_back(page);
        }
};

} // namespace laid
#endif