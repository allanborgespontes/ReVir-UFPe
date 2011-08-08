#ifndef _PROTLIB__TLP_LIST_H_
#define _PROTLIB__TLP_LIST_H_

#include <netinet/in.h>
#include <string>
#include <ostream>
#include <list>
#include "hashmap"

#include "logfile.h"
#include "protlib_types.h"
#include <ostream>

namespace protlib {

/** @class TLP_list
 * 
 *  It is used in NetMsg class, to store the position of the corresponding object types
 *  @note It`s a pointer in NetMsg, but can be not allocated (= NULL)
 */
class TLP_list {

public:
    // uint32 is the position relative to netmsg::msg_buf begin
	typedef list<uint32> position_list_t;
	// uint32 is the type (first (high order) 16 bit) and subtype (last (low order) 16 bit)
  	typedef hashmap_t<uint32, position_list_t*> objpos_map_t;  	
  	typedef objpos_map_t::const_iterator objpos_map_const_iter;
    // constructor
 	inline TLP_list(): obj2pos_map() {}
    TLP_list(const TLP_list& tlp);
    //destructor
	~TLP_list()
	{
		TLP_list::objpos_map_const_iter it = obj2pos_map.begin();
		for(;it != obj2pos_map.end(); it++) {
			delete it->second;
		}
		obj2pos_map.clear();
	}
	
	void insert(uint16 type, uint16 subtype, uint32 position);
//	uint32 get_position(uint16 object_type, uint32& position) const;
  	const position_list_t* get_list(uint16 type, uint16 subtype) const;

	const objpos_map_t& get_map() const { return obj2pos_map; }
  	
private:
/// uint16 is the obj_type, the linked list assign the object type to linked list entry.  
  	typedef objpos_map_t::iterator objpos_map_iter;

	// a map that stores netmsg buffer positions of object types
	objpos_map_t obj2pos_map;
};

inline std::ostream& operator<<(std::ostream& os, const TLP_list& tlp) {
	TLP_list::objpos_map_const_iter it = tlp.get_map().begin();
	os<<"TLP_list : {\n";
	uint16 old_type = (uint16)(it->first>>16)+1;
	uint16 type = (uint16)(it->first>>16);
	while(it != tlp.get_map().end()) {
		if( type != old_type ) 
			os<<"\ttype : "<< type <<" {\n";
		os<<"\t\tsubtype : "<<(uint16)(it->first)<<" {\n"<<"\t\t\tpos : { ";
		list<uint32>::const_iterator list_it = it->second->begin();
		while(list_it!=it->second->end()) {
			os<<*list_it;
			list_it++;
			if(list_it!=it->second->end()) {
				os<<", ";
			} else {
				os<<" }\n\t\t}\n";
			}
		}
		it++;
		old_type = type;
		if( it != tlp.get_map().end() ) {
			type = (uint16)(it->first>>16);
			if( type != old_type ) 
				os<<"\t}\n";
		} else os<<"\t}\n";
	}
	os<<"}\n";
	return os;
}

}


#endif // _PROTLIB__TLP_LIST_H_
