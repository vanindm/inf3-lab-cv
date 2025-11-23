#include <cassert>
#include <iostream>
#include <string>
#include "Filesystem.h"

int main() {
	LabFS::Node root(std::string("/"));
	root.AddDirectorySubnode("test");
	auto children = root.GetChildren();
	auto iterDirectories = children->getEnumerator();
	while (iterDirectories->moveNext()) {
		assert(iterDirectories->current()->GetName() == std::string("test"));
	}
	assert(root.GetSubnode("test")->GetName() == "test");
	return 0;
}