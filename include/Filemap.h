#pragma once

#include <string>
#include <PATypes/Sequence.h>

namespace LabFS {

	const std::string HOMEDIR = "/";

	class Path {
		PATypes::LinkedList<std::string> storage;
	public:
		Path(const std::string& path) {
			std::string rpath = path;
			if (rpath[0] != '/') {
				rpath = HOMEDIR + path;
			}
			std::string current_node = "/";
			for (char c : rpath) {
				if (c == '/') {
					if (current_node == "..") {
						storage.removeAt(storage.getLength() - 1);
						if (storage.getLength() != 1)
							storage.removeAt(storage.getLength() - 1);
					}
					storage.append("/");
					storage.append(current_node);
					current_node = "";
				}
				current_node += c;
			}
		}
		void append() {

		}
	};

	class File {
		size_t hash;
		Path path;
	};
}