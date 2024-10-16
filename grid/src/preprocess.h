#include "pugixml.hpp"
#include <iostream>
#include <filesystem>


struct doc_flatten: pugi::xml_tree_walker
{
    doc_flatten(pugi::xml_document& doc, const char* root_doc): flattened(doc), root_doc(root_doc) {
    }
    pugi::xml_document& flattened;
    const char* root_doc;
    virtual bool for_each(pugi::xml_node& node) {
        if (std::strcmp(node.name(), "import") == 0) {
            const char* file_path = node.attribute("file").value();
            auto parent_path = std::filesystem::path(root_doc).parent_path();
            auto to_path = std::filesystem::path(file_path);
            auto full_path = parent_path / to_path;
            if (!std::filesystem::exists(full_path)) {
                std::cerr << "File does not exist: " << full_path << std::endl;
            }
            pugi::xml_document import_doc;
            pugi::xml_parse_result result = import_doc.load_file(full_path.c_str());

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

    doc_flatten walker(xml_doc, input_file);
    xml_doc.traverse(walker);
    return xml_doc;
}
