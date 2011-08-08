#include "auth_attr_portlist.h"
#include "logfile.h"


namespace nslp_auth {



auth_attr_portlist:: auth_attr_portlist(uint8 xtype, uint8 subtype): auth_attr(xtype, subtype), ie_name(subtype == UDP_PORT_LIST?"UDP_PORT_LIST":"TCP_PORT_LIST") {
}

/*
*Destruktor
*/
auth_attr_portlist::~auth_attr_portlist() {
	list.clear();
}

auth_attr_portlist *auth_attr_portlist::new_instance() const {
	auth_attr_portlist *a = NULL;
	catch_bad_alloc( a = new auth_attr_portlist() );
	return a;
}


auth_attr_portlist *auth_attr_portlist::copy() const {
	auth_attr_portlist *a = NULL;
	catch_bad_alloc( a = new auth_attr_portlist(*this) );
	return a;
}

bool auth_attr_portlist::equals_body(const auth_attr &attr) const{
	const auth_attr_portlist *other = dynamic_cast<const auth_attr_portlist*>(&attr);
	if( other==NULL || other->getListSize()!=list.size() ) 
		return false;
	return list == other->get_list();
}

size_t auth_attr_portlist::get_serialized_size_nopadding(coding_t coding) const{
	// 4 bytes attr header
	return list.size()*2+4;
}


size_t auth_attr_portlist::get_serialized_size(coding_t coding) const{
	// 4 bytes attr header + padding
	return round_up4(list.size()*2)+4 ;
}



bool auth_attr_portlist::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip){
	bytes_read = 0;
	uint32 start_pos = msg.get_pos();
	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < round_up4(body_length) ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}
	
	uint32 port_count= body_length/2;
	list.clear();
	uint32 i;	
	for(i=0; i<port_count; i++){
		list.push_back(msg.decode16());	
	}
	bytes_read = port_count*2;

	// Error: We expected the length field to be different.
	if ( (body_length + HEADER_LENGTH) !=(uint16) get_serialized_size_nopadding(coding) || body_length!= bytes_read) {

		catch_bad_alloc( err.put( new IEWrongLength(coding, get_category(), get_xtype(), get_subtype(), msg.get_pos())) );
		msg.set_pos(start_pos);
		return false;
	}
	
	// jump over padding
	if( bytes_read%4 ) {
		msg.set_pos_r(4 - (bytes_read % 4));
		bytes_read += (4 - (bytes_read % 4));
	}
	
	return true;
}

void auth_attr_portlist::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const{

	for(vector<port_t>::const_iterator port_list_iter(list.begin()); port_list_iter != list.end(); port_list_iter++) {
		msg.encode16(*port_list_iter);
	}
	bytes_written = 2*list.size();

	// handle padding
	if ( bytes_written % 4 )
		msg.padding(4 - ( bytes_written % 4));

	bytes_written += 4 - ( bytes_written % 4);
}

const vector<port_t>& auth_attr_portlist::get_list() const{
	return list;
}

uint16 auth_attr_portlist::getListSize()const{
	return list.size();
}

void auth_attr_portlist::insert(uint16 p){
	list.push_back(p);
}

bool auth_attr_portlist::isPortIn(uint16 p)const{
	bool bo=false;
	for(uint32 i=0; i< list.size(); i++)
		if (list.at(i)==p) {
			bo=true;
			break;
		}
	return bo;
}

ostream &auth_attr_portlist::print_attributes(ostream &os) const {
	os << "ports: { ";
	for(unsigned int i=0;i<list.size();i++) os<<list[i]<<(i==list.size()-1?" }":" , ");
	return os; 
}

ostream &auth_attr_portlist::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
	os << setw(level*indent) << "";

	if ( name )
	  os << name <<  " = ";

	os << "[" << ie_name << ": xtype = "<<(int)x_type<<", subtype = "<<(int)sub_type<<", attribute_body : { \n";
	os << setw((level+1)*indent) << "";

	print_attributes(os);
	
	os <<"\n"<<setw(level*indent) << "";	 
	os << "}]";

	return os;	
}

bool auth_attr_portlist::check() const {
	return check_body();
}

bool auth_attr_portlist::check_body() const {
	return !list.empty();
}


}

