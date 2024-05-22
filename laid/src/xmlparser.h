#include <iostream>
#include "pugixml.hpp"
#include "models.h"
#include <stdexcept>

namespace laid {

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

    // build styles
    for (pugi::xml_node node: xml_doc_start.child("head").child("styles").children("style")) {
        laid::Style style;
        style.name = std::string(node.attribute("name").as_string());
        if (style.name == "") {
            throw std::invalid_argument("Style must have a name!");
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
        doc->addStyle(style);
    }

    // build pages
    bool overflowNext = false;
    for (pugi::xml_node node: xml_doc_start.child("body").child("pages").children("page")) {
        auto masterName = node.attribute("masterPage").as_string();
        auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
        page->overflow = node.attribute("overflow").as_bool();
        std::shared_ptr<laid::Box> prev;
        for (pugi::xml_node box_node: node.children("box")) {
            auto gx = box_node.attribute("gX").as_int();
            auto gy = box_node.attribute("gY").as_int();
            auto gWidth = box_node.attribute("gWidth").as_int();
            auto gHeight = box_node.attribute("gHeight").as_int();

            std::cout << gx << " " << gy << " " << gWidth << " " << gHeight << std::endl;

            auto start = page->masterPage.getRect(gx, gy);
            auto end = page->masterPage.getRect(gx + gWidth, gy + gHeight);

            auto x = start.startX;
            auto y = start.startY;
            auto width = end.endX - start.startX;
            auto height = end.endY - start.startY;
            
            
            if (box_node.attribute("x").as_int() != 0) {
                x = box_node.attribute("x").as_int();
            }
            if (box_node.attribute("y").as_int() != 0) {
                y = box_node.attribute("y").as_int();
            }
            if (box_node.attribute("width").as_int() != 0) {
                width = box_node.attribute("width").as_int();
            }
            if (box_node.attribute("height").as_int() != 0) {
                height = box_node.attribute("height").as_int();
            }

            auto box = std::make_shared<laid::Box>(x, y, width, height);
            if (overflowNext) {
                prev->addNext(box);
            }
            overflowNext = box_node.attribute("overflowNext").as_bool();
            for (pugi::xml_node text_node: box_node.children("text")) {
                auto styleName = text_node.attribute("style").as_string();
                auto style = doc->paragraph_styles[styleName];
                auto text = text_node.text().as_string();
                box->addText(text, style);
            }
            page->addBox(box);
            prev = box;
        }
        doc->addPage(page);
    }
    return doc;
}
}
