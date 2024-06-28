#include <iostream>
#include "pugixml.hpp"
#include "models.h"
#include <stdexcept>
#include <cstring>

namespace laid {


void parseParagraph(pugi::xml_node node, std::shared_ptr<laid::Paragraph> paragraph) {
    pugi::xpath_node_set nodes = node.select_nodes(".//text()");
    for (pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        pugi::xml_node node = it->node();
        auto text = node.text().as_string();
        std::string style = node.parent().attribute("style").as_string();
        paragraph->addText(text, style);
    }
}

void parseColor(pugi::xml_node node, std::shared_ptr<laid::Document> doc) {
    auto color = laid::RGBColor();
    color.name = node.attribute("name").as_string();
    auto colString = node.attribute("rgb").as_string();
        std::istringstream ss(colString);
        char ch; // to discard the '-' character
        int r, g, b;
        ss >> r >> ch >> g >> ch >> b;
        color.r = r;
        color.g = g;
        color.b = b;
    doc->addColor(color);
}

void parsePage(pugi::xml_node node, std::shared_ptr<laid::Document> doc) {
    auto masterName = node.attribute("masterPage").as_string();
    auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
    page->overflow = node.attribute("overflow").as_bool();
    doc->addPage(page);
}

std::shared_ptr<laid::Document> load_file(const char* filename) {
    pugi::xml_document xml_doc;
    pugi::xml_parse_result result = xml_doc.load_file(filename);
    auto xml_doc_start = xml_doc.child("document");
    auto doc = std::make_shared<laid::Document>();

    // build master pages
    for (pugi::xml_node node: xml_doc_start.child("head").child("masterPages").children("masterPage")) {
        laid::MasterPage masterPage;
        masterPage.name = std::string(node.attribute("name").as_string());
        if (masterPage.name == "") {
            throw std::invalid_argument("Master page must have a name!");
        }
        masterPage.height = node.attribute("height").as_int();
        masterPage.width = node.attribute("width").as_int();
        masterPage.cols = node.attribute("columns").as_int();
        masterPage.rows = node.attribute("rows").as_int();
        masterPage.baseline = node.attribute("baseline").as_int();
        masterPage.marginLeft = node.attribute("marginLeft").as_int();
        masterPage.marginRight = node.attribute("marginRight").as_int();
        masterPage.marginTop = node.attribute("marginTop").as_int();
        masterPage.marginBottom = node.attribute("marginBottom").as_int();
        masterPage.gap = node.attribute("gap").as_int();

        doc->addMasterPage(masterPage);
    }

    // build swatches
    for (pugi::xml_node node: xml_doc_start.child("head").child("colors").children("color")) {
        parseColor(node, doc);
    }
    // build styles
    for (pugi::xml_node node: xml_doc_start.child("head").child("styles").children("style")) {
        laid::Style style;
        style.name = std::string(node.attribute("name").as_string());
        if (style.name == "") {
            throw std::invalid_argument("Style must have a name!");
        }
        auto inherit = node.attribute("inherit").as_string();
        if (inherit != "") {
            style.inherit = inherit;
        }
        style.fontName = std::string(node.attribute("fontName").as_string());
        if (style.fontName == "") {
            style.fontName = "Arial";
        }
        style.fontSize = node.attribute("fontSize").as_int();
        if (style.fontSize == 0) {
            style.fontSize = 10;
        }
        style.leading = node.attribute("leading").as_int();
        if (style.leading == 0) {
            if (style.fontSize != 0) {
                style.leading = style.fontSize * 1.2;
            } else {
                style.leading = 12;
            }
        }
        auto weight = node.attribute("weight").as_int();
        // TODO we can only support multiples of 100
        if (weight == 0) {
            style.weight = 400;
        } else {
            style.weight = weight;
        }

        auto width = node.attribute("width").as_int();
        // TODO supported range is 1-9
        if (width == 0) {
            // normal width
            style.width = 5;
        } else {
            style.width = width;
        }

        auto slant = node.attribute("slant").as_int();
        // TODO supported range is 1-3
        if (slant == 0) {
            // normal slant
            style.slant = 0;
        } else {
            style.slant = slant;
        }

        auto color = node.attribute("color").as_string();
        if (color != "") {
            style.color = color;
        }

        doc->addStyle(style);
    }

    // maps used to link boxes in the whole document
    std::map<std::string, std::shared_ptr<laid::Box>> boxes;
    std::map<std::string, std::string> boxMap;

    // build pages
    bool isSpread = false;
    for (pugi::xml_node node: xml_doc_start.child("body").child("pages").children()) {
        auto basePage = std::make_shared<laid::PageObject>();
        if (std::strcmp(node.name(), "page") == 0) {
            auto masterName = std::string(node.attribute("masterPage").as_string());
            auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
            page->overflow = node.attribute("overflow").as_bool();
            // Set other Page-specific properties
            basePage = page; // Assuming basePage is meant to hold any PageObject
        } else if (std::strcmp(node.name(), "spread") == 0) {
            isSpread = true;
            auto leftMaster = std::string(node.attribute("leftMaster").as_string());
            auto rightMaster = std::string(node.attribute("rightMaster").as_string());
            auto spread = std::make_shared<laid::Spread>(
                *doc->masterPages[leftMaster],
                *doc->masterPages[rightMaster]
            );
            spread->overflow = node.attribute("overflow").as_bool();
            // Set other Spread-specific properties
            basePage = spread; // Assuming basePage is meant to hold any PageObject
        }

        std::shared_ptr<laid::Box> prev;
        for (pugi::xml_node box_node: node.children("box")) {
            auto name = box_node.attribute("name").as_string();
            if (name == "") {
                throw std::invalid_argument("Box must have a name!");
            }
            auto next = box_node.attribute("next").as_string();
            
            float x, y, width, height;
            // x pos
            auto gxpos = box_node.attribute("gX");
            auto xpos = box_node.attribute("x");
            if (xpos.empty() == false) {
                x = xpos.as_float();
            } else if (gxpos.empty() == false) {
                auto rect = basePage->getRect(gxpos.as_float(), 1);
                x = rect.startX;
            } else {
                throw std::invalid_argument("Box must have an x or gX attribute!"); 
            }

            // y pos
            auto gypos = box_node.attribute("gY");
            auto ypos = box_node.attribute("y");
            if (ypos.empty() == false) {
                y = ypos.as_float();
            } else if (gypos.empty() == false) {
                auto rect = basePage->getRect(1, gypos.as_float());
                y = rect.startX;
            } else {
                throw std::invalid_argument("Box must have an x or gX attribute!"); 
            }

            // width
            auto gwidth = box_node.attribute("gWidth");
            auto widthlen = box_node.attribute("width");
            if (gwidth.empty() == false) {
                auto start = basePage->getRect(1, 1);
                auto end = basePage->getRect(gwidth.as_float(), 1);
                width = end.endX - start.startX;
            } else if (widthlen.empty() == false) {
                width = widthlen.as_float();
            } else {
                throw std::invalid_argument("Box must have a width or gWidth attribute!"); 
            }

            // height
            auto gheight = box_node.attribute("gHeight");
            auto heightlen = box_node.attribute("height");
            if (gheight.empty() == false) {
                auto start = basePage->getRect(1, 1);
                auto end = basePage->getRect(1, gheight.as_float());
                height = end.endY - start.startY;
            } else if (heightlen.empty() == false) {
                height = heightlen.as_float();
            } else {
                throw std::invalid_argument("Box must have a height or gHeight attribute!"); 
            }

            int zIndex = box_node.attribute("zIndex").as_int();
            auto box = std::make_shared<laid::Box>(x, y, width, height);
            box->zIndex = zIndex;
            boxes[name] = box;
            boxMap[name] = next;
            for (pugi::xml_node paragraph_node: box_node.children("para")) {
                auto paragraph = std::make_shared<laid::Paragraph>();
                paragraph->style = paragraph_node.attribute("style").as_string();
                parseParagraph(paragraph_node, paragraph);
                box->addParagraph(paragraph);
            }
            basePage->addBox(box);
            prev = box;
        }
        if (isSpread == true) {
            auto spread = std::dynamic_pointer_cast<laid::Spread>(basePage);
            doc->addSpread(spread);
        }
        else {
            doc->addPage(std::dynamic_pointer_cast<laid::Page>(basePage));
        }
    }
    // link boxes
    for(const auto &pair : boxMap) {
        if (pair.second == "" || pair.first == "") {
            continue;
        }
        boxes[pair.first]->next = boxes[pair.second];
        boxes[pair.second]->prev = boxes[pair.first].get();
        boxes[pair.second]->prev2 = boxes[pair.first];
    }
    return doc;
}
}
