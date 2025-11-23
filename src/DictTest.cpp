#include <cassert>
#include <iostream>
#include <string>

#include <PATypes/HashMap.h>
#include <PATypes/Map.h>

int main() {

    PATypes::Map<int, int> intDictionary;
    intDictionary.Add(1, 1);
    assert(intDictionary.Get(1) == 1);

    PATypes::Map<std::string, std::string> stringDictionary;
    stringDictionary.Add("asdf", "fdsa");
    stringDictionary.Add("fdsa", "bebebe");
    assert(stringDictionary.Get("asdf") == "fdsa");
    assert(stringDictionary.Get("fdsa") == "bebebe");
    stringDictionary.Delete("fdsa");
    bool caught = false;
    try {
        stringDictionary.Get("fdsa");
    } catch (std::out_of_range &err) {
        caught = true;
    }
    assert(caught);
    caught = false;

    PATypes::HashMap<int, int> intHashMap;
    intHashMap.Add(1, 1);
    assert(intHashMap.Get(1) == 1);
    try {
        intHashMap.Get(2);
    } catch (std::out_of_range &err) {
        caught = true;
    }
    assert(caught);
    caught = false;
    return 0;
}