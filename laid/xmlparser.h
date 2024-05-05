#include <iostream>
#include "pugixml.hpp"
#include "models.h"

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
        masterPage.height = node.attribute("height").as_int();
        masterPage.width = node.attribute("width").as_int();
        masterPage.cols = node.attribute("columns").as_int();
        masterPage.rows = node.attribute("rows").as_int();
        masterPage.baseline = node.attribute("baseline").as_int();

        doc->addMasterPage(masterPage);
    }

    // build styles
    for (pugi::xml_node node: xml_doc_start.child("head").child("styles").children("style")) {
        laid::Style style;
        auto nn = node.attribute("name").as_string();
        style.name = std::string(node.attribute("name").as_string());
        style.fontName = std::string(node.attribute("fontName").as_string());
        style.fontSize = node.attribute("fontSize").as_int();
        style.leading = node.attribute("leading").as_int();
        doc->addStyle(style);
    }

    // build pages
    for (pugi::xml_node node: xml_doc_start.child("body").child("pages").children("page")) {
        auto masterName = node.attribute("masterPage").as_string();
        auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
        for (pugi::xml_node box_node: node.children("box")) {
            auto x = box_node.attribute("x").as_int();
            auto y = box_node.attribute("y").as_int();
            auto width = box_node.attribute("width").as_int();
            auto height = box_node.attribute("height").as_int();
            auto box = std::make_shared<laid::Box>(x, y, width, height);
            for (pugi::xml_node text_node: box_node.children("text")) {
                auto style = doc->paragraph_styles["p"];
                auto text = text_node.text().as_string();
                box->addText(text, style);
            }
            page->addBox(box);
        }
        // TODO add linkedboxes
        doc->addPage(page);
    }
    return doc;
}


//int main() {
//    auto doc = load_file("sketches/sketch4.xml");
//    std::cout << "Master Pages: " << doc->masterPages.size() << std::endl;
//    std::cout << "Styles: " << doc->paragraph_styles.size() << std::endl;
//    std::cout << "Pages: " << doc->page_count << std::endl;
//}

} // namespace laid