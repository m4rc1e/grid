#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <tuple>


namespace laid {


class Style {
    public:
        std::string name;
        std::string fontName;
        int fontSize;
        int leading;
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
        float baseline;
    
        Rect getRect(int col, int row) {
            // columns
            auto workingWidth = width - marginLeft - marginRight - (gap*(cols-1));
            auto colWidth = (workingWidth / cols);
            float startX, endX;
            if (col == 0) {
                startX = marginLeft;
                endX = startX + colWidth;
            } else {
                startX = marginLeft + ((colWidth * col)) + (gap*col);
                endX = startX + colWidth;
            }
            // rows
            auto workingHeight = height - marginTop - marginBottom - (gap*(rows-1));
            auto rowHeight = workingHeight / rows;
            float startY, endY;
            if (row == 0) {
                startY = marginTop;
                endY = startY + rowHeight;
            } else {
                startY = marginTop + (rowHeight * row) + (gap*row);
                endY = startY + rowHeight;
            }
            return Rect{startX, startY, endX, endY};
        }

};


class TextRun {
    public:
        std::string text;
        Style style;
};


class Box {
    public:
        float x;
        float y;
        float width;
        float height;
        int pageIdx;
        std::vector<TextRun> text_runs;
        std::shared_ptr<Box> next;
        std::shared_ptr<Box> prev;
        std::string image_path;
        std::map<int, std::vector<std::shared_ptr<Box>>> children;

        Box(int x, int y, int width, int height) : x(x), y(y), width(width), height(height) {}
        void addText(const std::string& text, Style style) {
            text_runs.push_back(TextRun{text, style});
        }

        void addImage(const std::string& path) {
            image_path = path;
        }

        void addNext(std::shared_ptr<Box> box) {
            next = box;
            box->prev = std::make_shared<Box>(*this);
        }

        std::shared_ptr<Box> getFirst() {
            auto current = std::make_shared<Box>(*this);
            while (current->prev != nullptr) {
                current = current->prev;
            }
            return current;
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
        bool overflow;
        std::shared_ptr<Page> next;
        int boxIdx = 0;

        void addBox(std::shared_ptr<Box>& box) {
            box->pageIdx = boxIdx;
            boxes.push_back(box);
            boxIdx += 1;
        }


};

class Document {
    public:
        std::map<std::string, Style> paragraph_styles;
        std::map<std::string, MasterPage*> masterPages;
        int page_count;
        std::shared_ptr<Page> pages;
        std::shared_ptr<Page> lastPage;

        void addStyle(Style paragraphStyle) {
            paragraph_styles[paragraphStyle.name] = paragraphStyle;
        }

        void addMasterPage(MasterPage& masterPage) {
            masterPages[masterPage.name] = &masterPage;
        }

        void addPage(std::shared_ptr<Page> page) {
            page_count++;
            if (this->pages == nullptr) {
                this->pages = page;
                return;
            }
            auto current = this->pages;
            while (current->next != nullptr) {
                current = current->next;
            }
            std::cout << current << std::endl;
            current->next = page;
        }
        std::shared_ptr<Page> overflowPage(std::shared_ptr<Page> page) {
            auto newPage = std::make_shared<Page>(page->masterPage);
            newPage->overflow = true;
            
            for (auto& box : page->boxes) {
                auto newBox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                newPage->addBox(newBox);
            }
            auto tail = page->next;
            page->next = newPage;
            newPage->next = tail;
            return newPage;
        }
};

} // namespace laid
#endif