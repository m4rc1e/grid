/*
<!-- Root element -->
<document> 
  <!-- Just like html, we include metadata in the head element, such as masterpages, colors etc -->
  <head> 
    
    <masterpages>
      <!-- Master Page Element:
      Attributes:
        name: The name of the masterpage. References in Page and Spread elements (required)
        height: Height in pt (required)
        width: Width in pt (required)
        margins: comma seperated positions for left,right,up,down
        cols: Amount of grid columns
        rows: Amount of grid rows
        gutter: Space in pts between grid rows and cols
      -->
      <masterpage name="dflt" height="842" width="595" cols="4" rows="4" margins=30,30,30,30/>
    </masterpages>
    
    <colors>
      <!-- Color Element:
      Attributes:
        name: The name of the color. Is references in style and boxstyle elements (required)
        rgba: Red, green, blue alpha values. Value range 0-255 (required)
     -->
     <color name="white" rgba="255, 255, 255, 255"/>
    </colors>
    
    <styles>
      <!-- Style Element:
      Attributes:
        name: Style Name (required)
        inherit: Inherit from another style. Properties defined in this style will override the inherited style.
        fontname: Font Name
        fontsize: Font Size (required)
        leading: Leading
        weight: Font Weight (100-900) (default 400)
        width: Font Width (1-9) (default 5)
        slant: Font Slant (0-1) (default 0)
        color: Font Color (default builtin black)
      -->
      <style name="p" fontname="Inter" fontsize="10" leading="12"/>
    </styles>
    
    <!-- Boxes have their own style element -->
    <boxstyles>
      <!-- Box Style Element:
      Attributes:
        name: Box style name (required)
        color: Font Color (default None)
      -->
    </boxstyles>
    
  </head>

  <body> <!-- Looks familiar. Body element contains the actual content for our document -->
    <!-- Page Element:
    Attributes:
      masterpage: Master Page Element name (required)
      overflow: if true, create a duplicate page with the overflowing content (default false)
    -->
    <page masterpage="dflt" overflow="true">
      
      <!-- Box Element:
      Attributes:
        x: Starting x position of box in pt (Required if gx not set)
        y: Starting y position of box in pt (Required if gy not set)
	      gx: Starting x grid position of box (If both "gx" and "x" are set, "x" takes presedence)
        gy: Starting y grid position of box (If both "gy" and "y" are set, "y" takes presedence)
        
        width: Width of box in pt (Required if "cols" not set)
        height: Height of box in pt (Required if "rows not set)
        cols: Width of box in grid columns (Required if "width" not set)
        rows: Height of box in grid rows (Required if "height" not set)
        
        zindex: if value greater than 0, make colliding boxes flow around this box (default 0)
        
        style: Box Style name (default none)
        -->
        <box gx="1" gy="1" cols="4" rows="4">
        
          <!-- Para Element:
          Attributes:
            style: Name of style (required)
          
          Notes:
            This element can include 
          -->
          <para style="p">Hello World</para>
        </box>
    </page>
      
      <!-- Spread Element:
      Attributes:
        leftmaster: Left master page (Required)
        rightmaster: Right master page (Required)
      -->
      <spread leftmaster="dflt" rightmaster="dflt">
        <box gx="1" gy="1" cols="4" rows="4">
          <para style="p">Hello World</para>
        </box>
      </spread>
  </body>
</document>
*/

#include <iostream>
#include "pugixml.hpp"
#include "models.h"
#include <stdexcept>
#include <cstring>
#include <filesystem>
#include <regex>


namespace laid {

std::unordered_set<std::string> masterPageAttribs = {
    "name",
    "height",
    "width",
    "cols",
    "rows",
    "baseline",
    "marginleft",
    "marginright",
    "margintop",
    "marginbottom",
    "gap"
};

std::unordered_set<std::string> colorAttribs = {
    "name",
    "rgba"
};

std::unordered_set<std::string> styleAttribs = {
    "name",
    "inherit",
    "fontname",
    "fontsize",
    "leading",
    "weight",
    "width",
    "slant",
    "color"
};

std::unordered_set<std::string> boxStyleAttribs = {
    "name",
    "color"
};

std::unordered_set<std::string> pageAttribs = {
    "masterpage",
    "overflow"
};

std::unordered_set<std::string> spreadAttribs = {
    "leftmaster",
    "rightmaster",
    "overflow"
};

std::unordered_set<std::string> boxAttribs = {
    "name",
    "next",
    "x",
    "y",
    "gx",
    "gy",
    "width",
    "gwidth",
    "height",
    "gheight",
    "zindex",
    "style"
};

std::unordered_set<std::string> paraAttribs = {
    "style"
};


bool unkownAttribs(pugi::xml_node node, std::unordered_set<std::string> attribs) {
    auto node_name = node.name();
    for (pugi::xml_attribute attr: node.attributes()) {
        if (attribs.find(attr.name()) == attribs.end()) {;
            throw std::invalid_argument("Unknown attribute in '" + std::string(node_name) + "' element called '" + std::string(attr.name()) + "'");
        }
    }
    return false;
}


bool parseMargins(const char* input) {
    // Updated pattern to ensure numbers are always positive
    std::regex pattern("^(\\d+),(\\d+),(\\d+),(\\d+)$");
    
    if (std::regex_match(input, pattern) == false) {
        throw std::invalid_argument(std::string("Invalid margins format, \"") + input + "\". Must be comma separated integers (left,right,top,bottom) e.g \"32,32,48,48\"");
    }
}

void parseParagraph(pugi::xml_node node, std::shared_ptr<laid::Paragraph> paragraph) {
    pugi::xpath_node_set nodes = node.select_nodes(".//text()");
    for (pugi::xpath_node_set::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        pugi::xml_node node = it->node();
        unkownAttribs(node, paraAttribs);
        auto text = node.text().as_string();
        std::string style = node.parent().attribute("style").as_string();
        paragraph->addText(text, style);
    }
}

void parseColor(pugi::xml_node node, std::shared_ptr<laid::Document> doc) {
    unkownAttribs(node, colorAttribs);
    auto color = laid::RGBColor();
    color.name = node.attribute("name").as_string();
    auto colString = node.attribute("rgba").as_string();
        std::istringstream ss(colString);
        char ch; // to discard the '-' character
        int r, g, b, a;
        ss >> r >> ch >> g >> ch >> b >> ch >> a;
        color.r = r;
        color.g = g;
        color.b = b;
        color.a = a;
    doc->addColor(color);
}

void parsePage(pugi::xml_node node, std::shared_ptr<laid::Document> doc) {
    unkownAttribs(node, pageAttribs);
    auto masterName = node.attribute("masterpage").as_string();
    auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
    page->overflow = node.attribute("overflow").as_bool();
    doc->addPage(page);
}

std::shared_ptr<laid::Box> parseBox(pugi::xml_node box_node, std::shared_ptr<laid::PageObject> basePage, std::map<std::string, std::shared_ptr<laid::Box>> &boxes, std::map<std::string, std::string> &boxMap) {
    unkownAttribs(box_node, boxAttribs);
    auto name = box_node.attribute("name").as_string();
    if (name == "") {
        throw std::invalid_argument("Box must have a name!");
    }
    auto next = box_node.attribute("next").as_string();

    float x, y, width, height;
    // x pos
    auto gxpos = box_node.attribute("gx");
    auto xpos = box_node.attribute("x");
    if (xpos.empty() == false) {
        x = xpos.as_float();
    } else if (gxpos.empty() == false) {
        auto rect = basePage->getRect(gxpos.as_float(), 1);
    x = rect.startX;
    } else {
        throw std::invalid_argument("Box must have a 'x' or 'gx' attribute!"); 
    }

    // y pos
    auto gypos = box_node.attribute("gy");
    auto ypos = box_node.attribute("y");
    if (ypos.empty() == false) {
        y = ypos.as_float();
    } else if (gypos.empty() == false) {
        if (gypos.as_string() == std::string("*")) {
            y = -1;
        } else {
            auto rect = basePage->getRect(1, gypos.as_float());
            y = rect.startY;
        }
    } else {
        throw std::invalid_argument("Box must have an 'y' or 'gy' attribute!"); 
    }

    // width
    auto gwidth = box_node.attribute("gwidth");
    auto widthlen = box_node.attribute("width");
    if (gwidth.empty() == false) {
        auto start = basePage->getRect(1, 1);
        auto end = basePage->getRect(gwidth.as_float(), 1);
        width = end.endX - start.startX;
    } else if (widthlen.empty() == false) {
        width = widthlen.as_float();
    } else {
        throw std::invalid_argument("Box must have a 'width' or 'gwidth' attribute!"); 
    }

    // height
    auto gheight = box_node.attribute("gheight");
    auto heightlen = box_node.attribute("height");
    if (gheight.empty() == false) {
        auto start = basePage->getRect(1, 1);
        auto end = basePage->getRect(1, gheight.as_float());
        height = end.endY - start.startY;
    } else if (heightlen.empty() == false) {
        height = heightlen.as_float();
    } else {
        throw std::invalid_argument("Box must have a 'height' or 'gheight' attribute!"); 
    }

    int zIndex = box_node.attribute("zindex").as_int();
    auto box = std::make_shared<laid::Box>(x, y, width, height);
    auto style = box_node.attribute("style");
    if (style.empty() == false) {
        box->style = std::string(style.as_string());
    }
    box->zIndex = zIndex;
    boxes[name] = box;
    boxMap[name] = next;
    int paragraphCount = 0;
    for (pugi::xml_node sub_node: box_node.children()) {
        if (std::strcmp(sub_node.name(), "para") == 0) {
            auto paragraph = std::make_shared<laid::Paragraph>();
            paragraph->style = sub_node.attribute("style").as_string();
            parseParagraph(sub_node, paragraph);
            box->addParagraph(paragraph);
        } else if (std::strcmp(sub_node.name(), "box") == 0) {
            box->addChild(paragraphCount, parseBox(sub_node, basePage, boxes, boxMap));
            std::cout << "getting box!"; 
        }
        paragraphCount++;
    }
    return box;
}

std::shared_ptr<laid::Document> load_file(const char* filename) {
    if (!std::filesystem::exists(filename)) {
        throw std::invalid_argument("File does not exist!");
    }
    pugi::xml_document xml_doc;
    pugi::xml_parse_result result = xml_doc.load_file(filename);
    auto xml_doc_start = xml_doc.child("document");
    auto doc = std::make_shared<laid::Document>();

    // build master pages
    for (pugi::xml_node node: xml_doc_start.child("head").child("masterpages").children("masterpage")) {
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
        
        auto margins = node.attribute("margins").as_string();
        if (margins != "") {
            std::istringstream ss(margins);
            parseMargins(margins);
            int left, right, top, bottom;
            char ch; // to discard the ',' character
            ss >> left >> ch >> right >> ch >> top >> ch >> bottom;
            masterPage.marginLeft = left;
            masterPage.marginRight = right;
            masterPage.marginTop = top;
            masterPage.marginBottom = bottom;
        } else {
            masterPage.marginLeft = node.attribute("marginleft").as_int();
            masterPage.marginRight = node.attribute("marginright").as_int();
            masterPage.marginTop = node.attribute("margintop").as_int();
            masterPage.marginBottom = node.attribute("marginbottom").as_int();
        }
        masterPage.gap = node.attribute("gap").as_int();

        doc->addMasterPage(masterPage);
    }

    // build swatches
    for (pugi::xml_node node: xml_doc_start.child("head").child("colors").children("color")) {
        parseColor(node, doc);
    }
    // build styles
    for (pugi::xml_node node: xml_doc_start.child("head").child("styles").children("style")) {
        unkownAttribs(node, styleAttribs);
        laid::Style style;
        style.name = std::string(node.attribute("name").as_string());
        if (style.name == "") {
            throw std::invalid_argument("Style must have a name!");
        }
        auto inherit = node.attribute("inherit").as_string();
        if (inherit != "") {
            style.inherit = inherit;
        }
        style.fontName = std::string(node.attribute("fontname").as_string());
        if (style.fontName == "") {
            style.fontName = "Arial";
        }
        style.fontSize = node.attribute("fontsize").as_int();
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

        std::cout << "weight: " << style.weight << std::endl;

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

    // build boxStyles
    for (pugi::xml_node node: xml_doc_start.child("head").child("boxstyles").children("boxstyle")) {
        unkownAttribs(node, boxStyleAttribs);
        laid::BoxStyle boxStyle;
        boxStyle.name = std::string(node.attribute("name").as_string());
        auto color = node.attribute("color").as_string();
        if (color != "") {
            boxStyle.color = color;
        }
        doc->addBoxStyle(boxStyle);
        
    }

    // maps used to link boxes in the whole document
    std::map<std::string, std::shared_ptr<laid::Box>> boxes;
    std::map<std::string, std::string> boxMap;

    // build pages
    bool isSpread = false;
    for (pugi::xml_node node: xml_doc_start.child("body").child("pages").children()) {
        auto basePage = std::make_shared<laid::PageObject>();
        if (std::strcmp(node.name(), "page") == 0) {
            unkownAttribs(node, pageAttribs);
            auto masterName = std::string(node.attribute("masterpage").as_string());
            auto page = std::make_shared<laid::Page>(*doc->masterPages[masterName]);
            page->overflow = node.attribute("overflow").as_bool();
            // Set other Page-specific properties
            basePage = page; // Assuming basePage is meant to hold any PageObject
        } else if (std::strcmp(node.name(), "spread") == 0) {
            unkownAttribs(node, spreadAttribs);
            isSpread = true;
            auto leftMaster = std::string(node.attribute("leftmaster").as_string());
            auto rightMaster = std::string(node.attribute("rightmaster").as_string());
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
            auto box = parseBox(box_node, basePage, boxes, boxMap);
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
        boxes[pair.second]->prev = boxes[pair.first];
    }
    return doc;
}
}
