#include "auth_nslp_object_list.h"
#include "logfile.h"
#include "benchmark_journal.h"

namespace nslp_auth {
using namespace protlib::log;

#ifdef BENCHMARK
  extern benchmark_journal journal;
#endif

/*auth_nslp_object_list::auth_nslp_object_list(uint8 xtype, uint8 subtype): auth_attr(xtype, subtype){
}*/

/*
*Destructor
*/
auth_nslp_object_list::~auth_nslp_object_list(){
	obj_type_list.clear();
}
auth_nslp_object_list *auth_nslp_object_list::new_instance() const {
	auth_nslp_object_list *a = NULL;
	catch_bad_alloc( a = new auth_nslp_object_list() );
	return a;
}


auth_nslp_object_list *auth_nslp_object_list::copy() const {
	auth_nslp_object_list *a = NULL;
	catch_bad_alloc( a = new auth_nslp_object_list(*this) );
	return a;
}

bool auth_nslp_object_list::equals_body(const auth_attr &attr) const {
  int i;
  bool bo=false;
  const auth_nslp_object_list *other = dynamic_cast<const auth_nslp_object_list*>(&attr);
  if ( other==NULL || other->getListSize()!=obj_type_list.size() ) return false;
  vector<uint16>::const_iterator iter;
  for (iter= obj_type_list.begin(); iter<obj_type_list.end(); iter++) {
    bo=false;
    for(i=0; i< other->getListSize(); i++)
      if(other->get_list().at(i)==*iter) { bo= true; break; } 
    if(!bo) return false;
  }
  return true;
}

size_t auth_nslp_object_list::get_serialized_size_nopadding(coding_t coding) const{
	return ((getListSize()+1)*2 + HEADER_LENGTH);
}

size_t auth_nslp_object_list::get_serialized_size(coding_t coding) const{
	return round_up4((getListSize()+1)*2 + HEADER_LENGTH);
}

bool auth_nslp_object_list::deserialize_body(NetMsg &msg, coding_t coding, uint16 body_length, IEErrorList &err, uint32 &bytes_read, bool skip){
	bytes_read = 0;
	uint32 start_pos = msg.get_pos();
	
	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < body_length ) {
		catch_bad_alloc( err.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}

	//MP(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH_ATTR_NSLP_OBJ_LIST_INSERT);	
	obj_type_list.clear();
	uint16 count_obj_type=msg.decode16();
	uint32 i;	
	for(i=0; i<count_obj_type; i++){
		obj_type_list.push_back(msg.decode16()&0xFFF);	
	}
	bytes_read = (count_obj_type+1)*2;
	//MP(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH_ATTR_NSLP_OBJ_LIST_INSERT);
	
	// Error: We expected the length field to be different.
	if ((body_length + HEADER_LENGTH) !=(uint16) get_serialized_size_nopadding(coding) || body_length!= bytes_read) {

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

void auth_nslp_object_list::serialize_body(NetMsg &msg, coding_t coding, uint32 &bytes_written) const{
	uint32 i;
	msg.encode16(obj_type_list.size());		
	for(i=0; i< obj_type_list.size(); i++){
		msg.encode16((uint16)obj_type_list.at(i) & 0xFFF);
	}
	bytes_written = 2*(obj_type_list.size()+1);
	// add padding if necessary
	if (bytes_written % 4)
	{
		msg.padding(round_up4(bytes_written) - bytes_written);
		bytes_written= round_up4(bytes_written);
	}
}


void auth_nslp_object_list::insert(uint16 ot) {
  obj_type_list.push_back(ot);
}


bool auth_nslp_object_list::isObjTypeIn(uint16 ot) const {
  bool return_val=false;

  for (uint32 i=0; i< obj_type_list.size(); i++) {
    if (obj_type_list.at(i)==ot) {
      return_val=true;
      break;
    }
  } // end for
  return return_val;
}

ostream &auth_nslp_object_list::print_attributes(ostream &os) const {
	os << "object type list: { ";
  	for (uint32 i=0; i< obj_type_list.size(); i++) os<<obj_type_list[i]<<(i==obj_type_list.size()-1?" }":" , ");
	return os; 
}

ostream &auth_nslp_object_list::print(ostream &os, uint32 level, const uint32 indent, const char *name ) const {
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

bool auth_nslp_object_list::check() const {
	return check_body();
}

bool auth_nslp_object_list::check_body() const {
	// other conditions are checked by session_auth_object::check_hmac_cond 
	if( obj_type_list.empty() ) return false;
	else return true;
}

}

