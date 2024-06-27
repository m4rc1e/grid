#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <tuple>
#include <iostream>
#include <sstream>


namespace laid {


class Style {
    public:
        std::string name;
        std::string fontName;
        std::string inherit;
        std::string swatch;
        int weight;
        int width;
        int slant;
        int fontSize;
        int leading;

        int fontSizeFromLeading(int leading) {
            return leading * 0.8333;
        }
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
        std::string style;
};

class Paragraph {
    public:
        std::vector<TextRun> text_runs;
        std::string style;

        void addText(const std::string& text, std::string& style) {
            text_runs.push_back(TextRun{text, style});
        }
};


class RGBColor {
    public:
        int r;
        int g;
        int b;
};
class Swatch {
    public:
        std::string name;
        std::string color;

    RGBColor parseRGB() {
        std::istringstream ss(color); // " " added at end due to delimiter for std::getline
        char ch; // to discard the '-' character
        int r, g, b;
        ss >> r >> ch >> g >> ch >> b;
        return RGBColor{r, g, b};
    } 
};


class Box {
    public:
        float x;
        float y;
        float width;
        float height;
        int pageIdx;
        int zIndex;
        std::vector<std::shared_ptr<Paragraph>> paragraphs;
        std::shared_ptr<Box> next;
        laid::Box* prev;
        std::shared_ptr<Box> prev2;
        std::string image_path;
        std::map<int, std::vector<std::shared_ptr<Box>>> children;

        Box(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}

        void addParagraph(std::shared_ptr<Paragraph> paragraph) {
            paragraphs.push_back(paragraph);
        }

        void addImage(const std::string& path) {
            image_path = path;
        }

        void addNext(std::shared_ptr<Box> box) {
            next = box;
            box->prev = this;
        }

        int getFirst() {
            auto current = this;
            while (current->prev != nullptr) {
                std::cout << "looping" << std::endl;
                current = current->prev;
            }
            std::cout << current->pageIdx << " got" << std::endl;
            return current->pageIdx;
        }

        std::shared_ptr<Box> getFirst2() {
            if (prev2 == nullptr) {
                return nullptr;
            }
            auto current = prev2;
            while (current->prev2 != nullptr) {
                current = current->prev2;
            }
            return current;
        }

        void addChild(int idx, std::shared_ptr<Box> box) {
            children[idx].push_back(box);
        }
};


class Page {
    public: 
        enum class PageType {
            Single,
            Left,
            Right
        };
        Page(MasterPage& masterPage) {
            this->masterPage = masterPage;
        }
        MasterPage masterPage;
        std::vector<std::shared_ptr<Box>> boxes;
        bool overflow;
        std::shared_ptr<Page> next;
        std::shared_ptr<Page> prev;
        int boxIdx = 0;
        PageType type = PageType::Single;

        void addBox(std::shared_ptr<Box>& box) {
            box->pageIdx = boxIdx;
            boxes.push_back(box);
            boxIdx += 1;
        }
};

class Spread {
    public:
        Spread(MasterPage& leftMaster, MasterPage& rightMaster) : leftMaster(leftMaster), rightMaster(rightMaster) {}
        MasterPage leftMaster;
        MasterPage rightMaster;
        std::vector<std::shared_ptr<Box>> boxes;
        bool overflow;

        void addBox(std::shared_ptr<Box>& box) {
            boxes.push_back(box);
        }

        std::shared_ptr<Page> leftPage() {
            auto page = std::make_shared<Page>(leftMaster);
            page->type = Page::PageType::Left;
            for (auto& box : boxes) {
                if (box->x > leftMaster.width) {
                    continue;
                }
                page->addBox(box);
            }
            page->overflow = overflow;
            return page;
        }

        std::shared_ptr<Page> rightPage() {
            auto page = std::make_shared<Page>(rightMaster);
            page->type = Page::PageType::Right;
            for (auto& box : boxes) {
                if (box->x > leftMaster.width) {
                    box->x -= leftMaster.width;
                    page->addBox(box);
                }
                // duplicate the box if it sits between both pages
                if (box->x + box->width > leftMaster.width) {
                    auto newbox = std::make_shared<Box>(*box);
                    newbox->x -= leftMaster.width;
                    page->addBox(newbox);
                }
            }
            page->overflow = overflow;
            return page;
        }

};

class Document {
    public:
        std::map<std::string, Swatch> swatches;
        std::map<std::string, Style> paragraph_styles;
        std::map<std::string, MasterPage*> masterPages;
        int page_count;
        std::shared_ptr<Page> pages;
        std::shared_ptr<Page> lastPage;

        void addSwatch(Swatch& swatch) {
            swatches[swatch.name] = swatch;
        }

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
        void addSpread(std::shared_ptr<Spread> spread) {
            auto leftPage = spread->leftPage();
            auto rightPage = spread->rightPage();
            rightPage->prev = leftPage;
            addPage(leftPage);
            addPage(rightPage);
        }
        std::shared_ptr<Page> overflowPage(std::shared_ptr<Page> page) {
            auto newPage = std::make_shared<Page>(page->masterPage);
            newPage->overflow = true;
            
            std::shared_ptr<Page> prev;
            for (auto& box : page->boxes) {
                auto newBox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                newPage->addBox(newBox);
            }
            // link boxes
            for (size_t i = 0; i < page->boxes.size(); i++) {
                auto oldBox = page->boxes[i];
                auto newBox = newPage->boxes[i];
                if (oldBox->next != nullptr) {
                    newBox->next = newPage->boxes[oldBox->next->pageIdx];
                }
                if (oldBox->prev != nullptr) {
                    newBox->prev = newPage->boxes[oldBox->prev->pageIdx].get();
                }
            }
            // link first box on this page
            for (size_t i = 0; i < page->boxes.size(); i++) {
                auto oldBox = page->boxes[i];
                auto newBox = newPage->boxes[i];                
                if (oldBox->next == nullptr) {
                    oldBox->next = newPage->boxes[oldBox->getFirst()];
                }
            }
            auto tail = page->next;
            page->next = newPage;
            newPage->next = tail;
            return newPage;
        }
        void overflowSpread(std::shared_ptr<Page> leftPage, std::shared_ptr<Page> rightPage) {
            std::cout << "overflow spread" << '\n';
            std::cout << "overflow spread2" << '\n';
            auto leftBoxes = leftPage->boxes;
            auto rightBoxes = rightPage->boxes;

            auto boxMap = std::map<std::shared_ptr<laid::Box>, std::shared_ptr<laid::Box>>();
            // create new left page
            auto newLeftPage = std::make_shared<Page>(leftPage->masterPage);
            for (auto& box : leftBoxes) {
                auto newbox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                boxMap[box] = newbox;
                newLeftPage->addBox(newbox);
            }

            // create new right page
            auto newRightPage = std::make_shared<Page>(rightPage->masterPage);
            for (auto& box : rightBoxes) {
                auto newbox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                boxMap[box] = newbox;
                newRightPage->addBox(newbox);
            }
            
            // link boxes
            for (auto& box : leftBoxes) {
                boxMap[box]->next = boxMap[box->next];
            }
            for (auto& box : rightBoxes) {
                boxMap[box]->next = boxMap[box->next];
            }
            // link first to last
            for (auto& box : leftBoxes) {
                if (box->next == nullptr) {
                    box->next = boxMap[box->getFirst2()];
                }
            }
            for (auto& box : rightBoxes) {
                if (box->next == nullptr) {
                    box->next = boxMap[box->getFirst2()];
                }
            }
            rightPage->next = newLeftPage;
            newLeftPage->next = newRightPage;
        }
};

} // namespace laid
#endif