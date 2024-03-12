#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <memory>
#include <pugixml.hpp>
#include <axml/axml_file.h>
#include <axml/axml_parser.h>
#include <axml/chunk_xml.h>

int main(int argc, const char* argv[]) {
    if (argc != 3){
        std::cout << "Usage: " << argv[0] << " <input_file> <out_file>" << std::endl;
        return -1;
    }
    
    const char* input_path = argv[1];
    const char* out_path =  argv[2];
    
    std::ifstream input_file(input_path, std::ios::binary);
    if (!input_file.is_open()) {
        std::cout << "Failed to open file: " << input_path << std::endl;
        return -1;
    }
    
    input_file.seekg(0, std::ios::end);
    std::streampos file_size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    std::unique_ptr<char[]> buffer = std::make_unique<char[]>(file_size);
    input_file.read(buffer.get(), file_size);
    input_file.close();

    axml::AXMLFile file(buffer.get(), file_size);
    axml::AXMLParser parser(file);

    pugi::xml_document doc;
    pugi::xml_node declarationNode = doc.prepend_child(pugi::node_declaration);
    declarationNode.append_attribute("version") = "1.0";
    declarationNode.append_attribute("encoding") = "UTF-8";
    std::vector<pugi::xml_node> xml_stack;
    
    while (parser.next()) {
        if (parser.eventType() == axml::AXMLParser::START_NAMESPACE) {
            printf("START_NAMESPACE prefix=%s url=%s\n", parser.getNamespacePrefix().c_str(),
                parser.getNamespaceUrl().c_str());
            
        }
        else if (parser.eventType() == axml::AXMLParser::END_NAMESPACE) {
            printf("END_NAMESPACE prefix=%s url=%s\n", parser.getNamespacePrefix().c_str(),
                parser.getNamespaceUrl().c_str());
        }
        else if (parser.eventType() == axml::AXMLParser::START_ELEMENT) {
            auto name = parser.getElementName();
            printf("START_ELEMENT ns=%s name=%s\n", parser.getElementNs().c_str(), name.c_str());
            size_t attrCount = parser.getElementAttributeCount();
            pugi::xml_node node{};
            if (xml_stack.empty()) {
                node = doc.append_child(name.c_str());
            }
            else {
                node = xml_stack.back().append_child(name.c_str());
            }

            for (size_t i = 0; i < attrCount; i++) {
                auto& typedValue = parser.getElementAttributeTypedValue(i);
                printf(" ATTRIBUTE ns=%s name=%s ", parser.getElementAttributeNs(i).c_str(),
                    parser.getElementAttributeName(i).c_str());
                auto ns = parser.getElementAttributeNs(i);
                auto name = parser.getElementAttributeName(i);

                if (ns == "http://schemas.android.com/apk/res/android")
                    name = "android:" + name;

                if (typedValue.dataType == axml::ResValue::TYPE_STRING) {
                    printf("rawValue=%s\n", parser.getElementAttributeRawValue(i).c_str());
                    node.append_attribute(name.c_str()) = parser.getElementAttributeRawValue(i).c_str();
                }
                else {
                    printf("typedValue=%u\n", typedValue.data.get());
                    if (typedValue.dataType == axml::ResValue::TYPE_INT_BOOLEAN)
                        node.append_attribute(name.c_str()) = typedValue.data?"true":"false";
                    else if(typedValue.dataType == axml::ResValue::TYPE_REFERENCE){
                        std::stringstream ss;
                        ss << "0x" <<std::hex << typedValue.data; 
                        node.append_attribute(name.c_str()) = ss.str().c_str();
                    }
                    else
                        node.append_attribute(name.c_str()) = typedValue.data;
                }
            }
            xml_stack.emplace_back(node);
            
        }
        else if (parser.eventType() == axml::AXMLParser::END_ELEMENT) {
            auto name = parser.getElementName();
            printf("END_ELEMENT ns=%s name=%s\n", parser.getElementNs().c_str(), name.c_str());
            if (!xml_stack.empty()) {

                xml_stack.back().end();
                xml_stack.pop_back();
            }
        }
    }
    doc.save_file(out_path);
}