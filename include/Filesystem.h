#pragma once

#include <string>
#include <optional>
#include <variant>
#include <memory>
#include "Filemap.h"
#include <PATypes/HashMap.h>

namespace LabFS {
	enum NodeType {
		NODE_FILE,
		NODE_DIRECTORY
	};
	class Node {
		std::shared_ptr<File> file;
		std::string directoryName;
		std::shared_ptr<PATypes::HashMap<std::string, std::shared_ptr<Node>>> subnodes; 
	public:
		Node() {
		}
		Node(File file) {
			std::make_shared<File>(file);
		}
		Node(std::string directoryName) : directoryName(directoryName) {
			subnodes = std::make_shared<PATypes::HashMap<std::string, std::shared_ptr<Node>>>();
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
				throw std::logic_error("попытка получения имени директории файла");
			return directoryName;
		}
		void AddFile(std::shared_ptr<File> file) {
			file = file;
		}
		void AddDirectorySubnode(std::string name) {
			if (file != nullptr) {
				throw std::logic_error("попытка добавления директории файлу");
			}
			subnodes->Add(name, std::make_shared<Node>(name));
		}
		std::shared_ptr<PATypes::Sequence<std::shared_ptr<Node>>> GetChildren() {
			return subnodes->GetAll();
		}
	};
};