#ifndef MODELS_H
#define MODELS_H

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <tuple>
#include <iostream>
#include <sstream>
#include "print.h"


namespace laid {



class Style {
    public:
        std::string name;
        std::string fontName;
        std::string inherit;
        std::string color;
        std::string ruleAbove;
        std::string ruleBelow;
        int weight;
        int width;
        int slant;
        int fontSize;
        int leading;
        float spaceBefore;
        float spaceAfter;

        int fontSizeFromLeading(int leading) {
            return leading * 0.8333;
        }
};


class StrokeStyle {
    public:
        std::string name;
        std::string color;
        float yOffset;
        float xOffset;
        float thickness;
};


class BoxStyle {
public:
    std::string name;
    std::string color;
};


class Rect {
    public:
        float startX;
        float startY;
        float endX;
        float endY;
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
        std::string name;
        int r;
        int g;
        int b;
        int a;
};


class Box {
    public:
        float x;
        float y;
        float width;
        float height;
        int zIndex;
        enum class VertAlignChoices {
            Top,
            Middle,
            Bottom
        };
        VertAlignChoices vertAlign = VertAlignChoices::Top;
        std::vector<std::shared_ptr<Paragraph>> paragraphs;

        std::shared_ptr<Box> next;
        std::shared_ptr<Box> prev;
        std::string image_path;
        std::string style;
        std::map<int, std::vector<std::shared_ptr<Box>>> children;
        std::vector<float> tabs;

        Box(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {
            // could use a const for default tab width
            for (int i = 1; i < width / 30; i++) {
                tabs.push_back(i * 30);
            }
        }
        // Copy constructor for deep copy
        Box(const Box& other)
            : x(other.x), y(other.y), width(other.width), height(other.height),
            vertAlign(other.vertAlign), image_path(other.image_path), style(other.style), tabs(other.tabs) {

            // Deep copy of paragraphs
            for (const auto& paragraph : other.paragraphs) {
                paragraphs.push_back(std::make_shared<Paragraph>(*paragraph));
            }

            // Deep copy of next and prev
            if (other.next) {
                next = std::make_shared<Box>(*other.next);
            }
            if (other.prev) {
                prev = std::make_shared<Box>(*other.prev);
            }

            // Deep copy of children
            for (const auto& [key, childVector] : other.children) {
                std::vector<std::shared_ptr<Box>> copiedChildVector;
                for (const auto& child : childVector) {
                    copiedChildVector.push_back(std::make_shared<Box>(*child));
                }
                children[key] = copiedChildVector;
            }
        }

        void addParagraph(std::shared_ptr<Paragraph> paragraph) {
            paragraphs.push_back(paragraph);
        }

        void addImage(const std::string& path) {
            image_path = path;
        }

        void addNext(std::shared_ptr<Box> box) {
            next = box;
        }

        float nextTab(float x) {
            for (auto& tab : tabs) {
                if (tab > x) {
                    return tab;
                }
            }
            return x;
        }

        void addTabs(std::vector<float> newTabs) {
            if (newTabs.size() <= tabs.size()) {
                for (int i = 0; i < newTabs.size(); i++) {
                    tabs[i] = newTabs[i];
                }
            return;
            }
            tabs = newTabs;
        }

        std::shared_ptr<Box> getFirst() {
            if (prev == nullptr) {
                return nullptr;
            }
            auto current = prev;
            while (current->prev != nullptr) {
                current = current->prev;
            }
            return current;
        }

        void addChild(int idx, std::shared_ptr<Box> box) {
            children[idx].push_back(box);
        }
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
        std::vector<std::shared_ptr<Box>> boxes;

        void addBox(std::shared_ptr<Box> box) {
            boxes.push_back(box);
        }
    
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



class PageObject {
    public:
        std::vector<std::shared_ptr<Box>> boxes;
        virtual void addBox(std::shared_ptr<Box>& box) {
            // Base implementation
        }
        virtual Rect getRect(int x, int y) {
            // Base implementation
        }
        bool overflow;
};


class Page : public PageObject {
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
        std::map<std::string, std::string> variables;
        std::string name;
        int number;

        void addBox(std::shared_ptr<Box>& box) {
            boxes.push_back(box);
            boxIdx += 1;
        }

        Rect getRect(int x, int y) {
            if (x <= 0 || x > masterPage.cols || y <= 0 || y > masterPage.rows) {
                throw std::invalid_argument("Invalid coordinates");
            }
            return masterPage.getRect(x-1, y-1);
        }
};

class Spread : public PageObject {
    public:
        Spread(MasterPage& leftMaster, MasterPage& rightMaster) : leftMaster(leftMaster), rightMaster(rightMaster) {}
        MasterPage leftMaster;
        MasterPage rightMaster;
        std::vector<std::shared_ptr<Box>> boxes;
        bool overflow;
        std::string leftName;
        std::string rightName;

        Rect getRect(int col, int row) {
            if (col <= 0 || col > leftMaster.cols + rightMaster.cols || row <= 0 || row > leftMaster.rows + rightMaster.rows) {
                throw std::invalid_argument("Invalid coordinates");
            }
            if (col > leftMaster.cols) {
                auto rect = rightMaster.getRect(col - leftMaster.cols - 1, row - 1);
                rect.startX += leftMaster.width;
                rect.endX += leftMaster.width;
                return rect;
            }
            return leftMaster.getRect(col-1, row-1);
        }

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
                for (auto& child : box->children) {
                    for (auto& childBox : child.second) {
                        if (childBox->x > leftMaster.width) {
                            continue;
                        }
                        page->addBox(childBox);
                    }
                }
            }
            page->overflow = overflow;
            page->name = leftName;
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
                for (auto& child : box->children) {
                    for (auto& childBox : child.second) {
                        auto newbox = std::make_shared<Box>(*childBox);
                        newbox->x -= leftMaster.width;
                        page->addBox(newbox);
                    }
                }
            }
            page->overflow = overflow;
            page->name = rightName;
            return page;
        }
};

class Document {
    public:
        // TODO what about CMYK colors?
        std::map<std::string, RGBColor> colors;
        std::map<std::string, Style> paragraph_styles;
        std::map<std::string, BoxStyle> boxStyles;
        std::map<std::string, StrokeStyle> strokeStyles;
        std::map<std::string, std::shared_ptr<MasterPage>> masterPages;
        int page_count;
        std::shared_ptr<Page> pages;
        std::shared_ptr<Page> lastPage;
        laid::PrintSettings printSettings;
        std::map<std::string, int> pageLinks;
        std::map<std::string, int> boxLinks;

        void addColor(RGBColor& color) {
            colors[color.name] = color;
        }

        void addStyle(Style paragraphStyle) {
            paragraph_styles[paragraphStyle.name] = paragraphStyle;
        }
        
        void addBoxStyle(BoxStyle boxStyle) {
            boxStyles[boxStyle.name] = boxStyle;
        }

        void addStrokeStyle(StrokeStyle strokeStyle) {
            strokeStyles[strokeStyle.name] = strokeStyle;
        }

        void addMasterPage(std::shared_ptr<MasterPage> masterPage) {
            masterPages[masterPage->name] = masterPage;
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
            newPage->name = page->name;
            
            auto boxMap = std::map<std::shared_ptr<laid::Box>, std::shared_ptr<laid::Box>>();
            
        
            std::shared_ptr<Page> prev;
            for (auto& box : page->boxes) {
                auto newBox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                boxMap[box] = newBox;
                newPage->addBox(newBox);
            }
            // link boxes
            for (auto& box : page->boxes) {
                boxMap[box]->next = boxMap[box->next];
                if (boxMap[box->next] != nullptr) {
                    boxMap[box->next]->prev = boxMap[box];
                }
            }
            // link first to last
            for (auto& box : page->boxes) {
                if (box->next == nullptr) {
                    if (box->getFirst() == nullptr) {
                        box->next = boxMap[box];
                    } else {
                        box->next = boxMap[box->getFirst()];
                    }
                }
            }

            auto tail = page->next;
            page->next = newPage;
            newPage->next = tail;
            return newPage;
        }
        void overflowSpread(std::shared_ptr<Page> leftPage, std::shared_ptr<Page> rightPage) {
            auto leftBoxes = leftPage->boxes;
            auto rightBoxes = rightPage->boxes;

            auto boxMap = std::map<std::shared_ptr<laid::Box>, std::shared_ptr<laid::Box>>();
            // create new left page
            auto newLeftPage = std::make_shared<Page>(leftPage->masterPage);
            newLeftPage->overflow = true;
            newLeftPage->type = Page::PageType::Left;
            newLeftPage->name = leftPage->name;
            for (auto& box : leftBoxes) {
                auto newbox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                boxMap[box] = newbox;
                newLeftPage->addBox(newbox);
            }

            // create new right page
            auto newRightPage = std::make_shared<Page>(rightPage->masterPage);
            newRightPage->overflow = true;
            newRightPage->type = Page::PageType::Right;
            newRightPage->name = rightPage->name;
            for (auto& box : rightBoxes) {
                auto newbox = std::make_shared<Box>(box->x, box->y, box->width, box->height);
                boxMap[box] = newbox;
                newRightPage->addBox(newbox);
            }
            
            // link boxes
            for (auto& box : leftBoxes) {
                boxMap[box]->next = boxMap[box->next];
                if (boxMap[box->next] != nullptr) {
                    boxMap[box->next]->prev = boxMap[box];
                }
            }
            for (auto& box : rightBoxes) {
                boxMap[box]->next = boxMap[box->next];
                if (boxMap[box->next] != nullptr) {
                    boxMap[box->next]->prev = boxMap[box];
                }
            }
            // link first to last
            for (auto& box : leftBoxes) {
                if (box->next == nullptr) {
                    box->next = boxMap[box->getFirst()];
                }
            }
            for (auto& box : rightBoxes) {
                if (box->next == nullptr) {
                    box->next = boxMap[box->getFirst()];
                }
            }
            auto tail = rightPage->next;
            rightPage->next = newLeftPage;
            newLeftPage->prev = rightPage;
            newLeftPage->next = newRightPage;
            newRightPage->prev = newLeftPage;
            newRightPage->next = tail;
            if (tail != nullptr) { 
                tail->prev = newRightPage;
            }
        }
};

} // namespace laid
#endif