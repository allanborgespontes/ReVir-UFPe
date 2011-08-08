#include "tlp_list.h"

namespace protlib {
	using namespace log;

TLP_list::TLP_list(const TLP_list& tlp) : obj2pos_map() {
	TLP_list::objpos_map_const_iter it = tlp.get_map().begin();
	for(;it != tlp.get_map().end(); it++) {
		position_list_t* pos_list = new position_list_t(*(it->second));
		obj2pos_map.insert(std::make_pair(it->first, pos_list));
	}
}

const TLP_list::position_list_t * TLP_list::get_list(uint16 type, uint16 subtype) const {
	uint32 key = ((uint32)type << 16) | subtype;
	objpos_map_const_iter it = obj2pos_map.find(key);
	if (it != obj2pos_map.end()) {
		return it->second;
	} else return NULL;
}

void TLP_list::insert(uint16 type, uint16 subtype, uint32 position){
	uint32 key = ((uint32)type << 16) | subtype;
	objpos_map_iter it_map = obj2pos_map.find(key);
	position_list_t* list = NULL;
	if (it_map != obj2pos_map.end()) {
		list = it_map->second;
	}
	if( list ) {
		position_list_t::iterator list_it = list->begin();
		while(list_it != list->end() && *list_it != position) list_it++;
		if( list_it != list->end() && *list_it == position){
			ERRLog("TLP_list","Same Object (type="<<type<<") at position "<<
				position<<" was already in list, but tried to insert.");
		} else list->push_back(position);
	} else {
		position_list_t* pos_list = new position_list_t(1, position);
		obj2pos_map.insert(std::make_pair(key, pos_list));
	}
}

}
