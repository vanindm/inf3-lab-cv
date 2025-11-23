#pragma once

#include "Set.h"
#include "IMap.h"
#include <functional>

namespace PATypes {
	template<class K, class V>
	class Map : IMap<K, V> {
		class MapNode {
			size_t index;
			V val;

		public:
			MapNode(K index, V val) : index(std::hash<K>{}(index)), val(val) {}
			MapNode(size_t indexHash, V val) : index(indexHash), val(val) {}
			size_t getIndex() { return index; }
			V getValue() { return val; }
			int operator<(const MapNode &b) const { return index < b.index; }
			int operator<=(const MapNode &b) const { return index <= b.index; }
			int operator==(const MapNode &b) const { return index == b.index; }
    	};
		Set<MapNode> storage;
		public:
			Map() {}
			virtual V Get(K index) const {
				try {
					return storage.getByItem({std::hash<K>{}(index), {}}).getValue();
				} catch (std::out_of_range& err) {
					throw std::out_of_range("попытка найти элемент, не лежащий в ассоциативном массиве");
				}
			}
			virtual void Add(K index, V value) {
				try {
					storage.erase({std::hash<K>{}(index), {}});
				} catch (std::logic_error&) {
				}
				storage.insert({std::hash<K>{}(index), value});
			}
			virtual void Clear() {
				storage = Set<MapNode>();
			}
	};

};