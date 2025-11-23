#pragma once

#include "Filemap.h"
#include <PATypes/HashMap.h>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace LabFS {
class Filesystem : std::enable_shared_from_this<Filesystem> {
    enum NodeType { NODE_FILE, NODE_DIRECTORY };
    class Node : std::enable_shared_from_this<Node> {
        std::shared_ptr<File> file;
        std::string directoryName;
        std::shared_ptr<PATypes::HashMap<std::string, std::shared_ptr<Node>>>
            subnodes;
        std::weak_ptr<Filesystem> fs;
		std::weak_ptr<Node> parent;

      public:
        Node(std::weak_ptr<Filesystem> fs = std::weak_ptr<Filesystem>())
            : fs(fs) {}
        Node(File file,
             std::weak_ptr<Filesystem> fs = std::weak_ptr<Filesystem>())
            : fs(fs) {
            std::make_shared<File>(file);
        }
        Node(std::string directoryName,
             std::weak_ptr<Filesystem> fs = std::weak_ptr<Filesystem>(),
             std::weak_ptr<Node> parent = std::weak_ptr<Node>())
            : directoryName(directoryName), fs(fs), parent(parent) {
            subnodes = std::make_shared<
                PATypes::HashMap<std::string, std::shared_ptr<Node>>>();
        }
        NodeType GetType() {
            return (file == nullptr ? NODE_DIRECTORY : NODE_FILE);
        }
        std::shared_ptr<Node> GetSubnode(std::string name) {
            if (GetType() == NODE_FILE)
                throw std::logic_error("попытка получения директории файла");
            return subnodes->Get(name);
        }
        std::string GetName() {
            if (file != nullptr)
                throw std::logic_error(
                    "попытка получения имени директории файла");
            return directoryName;
        }
        void AddFile(std::shared_ptr<File> file) { file = file; }
        void AddDirectorySubnode(std::string name) {
            if (file != nullptr) {
                throw std::logic_error("попытка добавления директории файлу");
            }
            subnodes->Add(name,
                          std::make_shared<Node>(name, fs, weak_from_this()));
        }
        std::shared_ptr<PATypes::Sequence<std::shared_ptr<Node>>>
        GetChildren() {
            return subnodes->GetAll();
        }
    };
    Node root;
    PATypes::HashMap<size_t, std::shared_ptr<File>> fileStorage;

  public:
    Filesystem() : root(std::string("/"), weak_from_this()) {}
    std::shared_ptr<Node> getRootDirectory() {
        return std::make_shared<Node>(root);
    }
};
} // namespace LabFS