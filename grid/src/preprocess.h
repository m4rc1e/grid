#include "pugixml.hpp"
#include <iostream>


struct doc_flatten: pugi::xml_tree_walker
{
    doc_flatten(pugi::xml_document& doc): flattened(doc) {
    }
    pugi::xml_document& flattened;
    virtual bool for_each(pugi::xml_node& node) {
        if (std::strcmp(node.name(), "import") == 0) {
            const char* file_path = node.attribute("file").value();
            pugi::xml_document import_doc;
            pugi::xml_parse_result result = import_doc.load_file(file_path);

            if (result) {
                for (pugi::xml_node import_node = import_doc.document_element().first_child(); import_node; import_node = import_node.next_sibling()) {
                    node.parent().insert_copy_after(import_node, node);
                }
                node.parent().remove_child(node);
            }
            else {
                std::cerr << "Failed to load file: " << file_path << " Error: " << result.description() << std::endl;
            }
        }
        return true; // continue traversal
    }
};

// may want to heap alloc this eventually
pugi::xml_document preprocessXML(const char* input_file) {
    pugi::xml_document xml_doc;
    pugi::xml_parse_result result = xml_doc.load_file(input_file);

    if (!result) {
        throw std::runtime_error("Failed to load file: " + std::string(result.description()));
    }

    doc_flatten walker(xml_doc);
    xml_doc.traverse(walker);
    return xml_doc;
}
