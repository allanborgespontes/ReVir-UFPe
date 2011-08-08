#include "logfile.h"

#include "auth_attr.h"
#include "ie.h"
#include "benchmark_journal.h"

namespace nslp_auth {
using namespace protlib::log;

#ifdef BENCHMARK
  extern benchmark_journal journal;
#endif

/**
 * Length of a header in bytes.
 */
const uint16 auth_attr::HEADER_LENGTH = 4;


/*
 * Standard Constructor
 */
auth_attr::auth_attr(): IE(cat_auth_attr), x_type(0), sub_type(0){
 // nothing to do
}




/**
 * Parse a auth_attr header.
 * @param msg the NetMsg to read from
 * @param coding encoding to use (not checked!)
 * @param body_length the length of the body in bytes (according to the header)
 * @param errorlist errors
 * @param bytes_read the number of bytes read
 * @param skip skip the header in case of parse errors
 * @return true on success, false if there's a parse error
 */

bool auth_attr::deserialize_header(NetMsg &msg, coding_t coding,uint16 &body_length, IEErrorList &errorlist, uint32 &bytes_read, bool skip) {
	MP(benchmark_journal::DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_HEADER_ENTRY);
	
	uint32 start_pos = msg.get_pos();

	uint32 header_raw;

	try {
		header_raw = msg.decode32();
	}
	catch ( NetMsgError ) {
		catch_bad_alloc( errorlist.put(new IEMsgTooShort(coding, get_category(), start_pos)) );
		return false;
	}

 	x_type = extract_xtype(header_raw);
	sub_type = extract_subtype(header_raw);

 	// The body's value is in Bytes, no padding included.
	body_length = extract_length(header_raw)-HEADER_LENGTH;
	
	MP(benchmark_journal::DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_HEADER_MIDDLE);
	
	// Error: Message is shorter than the length field makes us believe.
	if ( msg.get_bytes_left() < round_up4(body_length) ) {
		catch_bad_alloc( errorlist.put(new IEMsgTooShort(coding, category, start_pos)) );
 		msg.set_pos(start_pos);
		return false;
	}
	
	bytes_read = msg.get_pos() - start_pos;
	MP(benchmark_journal::DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_HEADER_END);
	return true;
}

IE *auth_attr::deserialize(NetMsg &msg, coding_t coding, IEErrorList &err, uint32 &bytes_read, bool skip) {

	bytes_read = 0;
	uint32 start_pos = msg.get_pos();

	// check if coding is supported
	uint32 tmp;
	if ( ! check_deser_args(coding, err, tmp) )
		return NULL;

	MP(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_HEADER);
	// Parse header
	uint16 body_length = 0;
	if ( ! (dynamic_cast<auth_attr*>(this)->deserialize_header(msg, coding, body_length, err, bytes_read, skip)) )
		return NULL;
	MP(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_HEADER);


	MP(benchmark_journal::PRE_DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_BODY);
	// Parse body
	uint32 body_bytes_read = 0;
	if ( ! (deserialize_body(msg, coding, body_length, err, body_bytes_read, skip)) )
		return NULL;
	MP(benchmark_journal::POST_DESERIALIZE_SESSIONAUTH_DESERIALIZE_ATTR_BODY);

	bytes_read += body_bytes_read;

	//cerr << "** AUTH_ATTR ** read " << get_ie_name() << " " <<  bytes_read << "  bytes" << endl;

	// Check if deserialize_body() returned the correct length.
	if ( bytes_read != msg.get_pos() - start_pos )
		Log(ERROR_LOG, LOG_CRIT, "auth_attr",	"deserialize(): byte count mismatch");

	return this;	// success
}


/**
 * Write a Auth_Attr header.
 *
 * @param msg the NetMsg to write to
 * @param coding encoding to use (not checked!)
 * @param bytes_written the number of bytes written
 * @param body_length the length of the body in bytes(!)
 */

void auth_attr::serialize_header(NetMsg &msg, coding_t coding, uint32 &bytes_written, uint16 body_length) const {

	
	uint16 length = body_length + HEADER_LENGTH;
	
	uint32 header = (length << 16)| (x_type << 8 )| sub_type;

	msg.encode32(header);

	bytes_written = 4;
}

void auth_attr::serialize(NetMsg &msg, coding_t coding, uint32 &bytes_written) const throw (IEError) {

	bytes_written = 0;

	// Check if the object is in a valid state. Throw an exception if not.
	uint32 tmp;
	check_ser_args(coding, tmp);

	/*
	 * Write header
	 */
	uint16 body_length = get_serialized_size_nopadding(coding) - HEADER_LENGTH;

	try {
		uint32 header_bytes_written;
		serialize_header(msg, coding, header_bytes_written, body_length);

		bytes_written += header_bytes_written;
	}
	catch (NetMsgError) {
		throw IEMsgTooShort(coding, get_category(), msg.get_pos());
	}


	/*
	 * Write body
	 */
	try {
		uint32 body_bytes_written;
		serialize_body(msg, coding, body_bytes_written);
		
 		// check padding for all
		if( body_bytes_written % 4 ){
			ERRLog("auth_attr","** ALIGNMENT ERROR ** " << get_ie_name() << " written: " << body_bytes_written);
			throw IEError(IEError::ERROR_UNALIGNED_OBJECT);
		}
		bytes_written += body_bytes_written;
	}
	catch (NetMsgError) {
		throw IEError(IEError::ERROR_MSG_TOO_SHORT);
	}
}

/**
 * Check the state of this object.
 *
 * This method can only check the header, so it relies on check_body()
 * in child classes.
 *
 * @return true if the object is in a valid state and false otherwise
 */
bool auth_attr::check() const {
	return check_body();
}

bool auth_attr::operator==(const IE &ie) const {
	const auth_attr *attr = dynamic_cast<const auth_attr*>(&ie);
	
	if( attr == NULL ) return false;
	
	return x_type==attr->get_xtype() && sub_type==attr->get_subtype() && equals_body(*attr); 
}

}

