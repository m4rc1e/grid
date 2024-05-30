#include <iostream>
#include "pugixml.hpp"
#include "models.h"
#include <stdexcept>

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

        doc->addStyle(style);
    }

    // maps used to link boxes in the whole document
    std::map<std::string, std::shared_ptr<laid::Box>> boxes;
    std::map<std::string, std::string> boxMap;

    // build pages
    for (pugi::xml_node node: xml_doc_start.child("body").child("pages").children("page")) {
        auto masterName = node.attribute("masterPage").as_string();
        auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
        page->overflow = node.attribute("overflow").as_bool();
        std::shared_ptr<laid::Box> prev;
        for (pugi::xml_node box_node: node.children("box")) {
            auto name = box_node.attribute("name").as_string();
            if (name == "") {
                throw std::invalid_argument("Box must have a name!");
            }
            auto next = box_node.attribute("next").as_string();
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
            boxes[name] = box;
            boxMap[name] = next;
            for (pugi::xml_node paragraph_node: box_node.children("para")) {
                auto paragraph = std::make_shared<laid::Paragraph>();
                paragraph->style = paragraph_node.attribute("style").as_string();
                parseParagraph(paragraph_node, paragraph);
                box->addParagraph(paragraph);
            }
            page->addBox(box);
            prev = box;
        }
        doc->addPage(page);
    }
    // link boxes
    for(const auto &pair : boxMap) {
        if (pair.second == "" || pair.first == "") {
            continue;
        }
        boxes[pair.first]->next = boxes[pair.second];
        boxes[pair.second]->prev = boxes[pair.first].get();
    }
    return doc;
}
}
