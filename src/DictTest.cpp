#include <iostream>
#include <cassert>
#include <string>

#include <PATypes/Map.h>
#include <PATypes/HashMap.h>

int main() {

	PATypes::Map<int, int> intDictionary; 
	intDictionary.Add(1, 1);
	assert(intDictionary.Get(1) == 1);

	PATypes::Map<std::string, std::string> stringDictionary;
	stringDictionary.Add("asdf", "fdsa");
	stringDictionary.Add("fdsa", "bebebe");
	assert(stringDictionary.Get("asdf") == "fdsa");
	assert(stringDictionary.Get("fdsa") == "bebebe");

	PATypes::HashMap<int, int> intHashMap;
	intHashMap.Add(1, 1);
	bool caught = false;
	assert(intHashMap.Get(1) == 1);
	try {
		intHashMap.Get(2);
	} catch (std::out_of_range& err) {
		caught = true;
	}
	assert(caught);
	return 0;
}