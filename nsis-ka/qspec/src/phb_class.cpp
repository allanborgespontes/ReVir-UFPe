/*
 * phb_class.cpp - The QSPEC PHB Class Parameter
 *
 * $Id: phb_class.cpp 6281 2011-06-17 13:26:53Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/phb_class.cpp $
 */
#include "phb_class.h"
#include "logfile.h"


using namespace qspec;



/**
 * The officially assigned parameter ID.
 */
const uint16 phb_class::PARAM_ID = 12;

const char *const phb_class::ie_name = "phb_class";


/**
 * Default constructor.
 */
phb_class::phb_class()
		: qspec_param(PARAM_ID), phb_id_field(0) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param phb_id_code a PHB ID value according to RFC 3140 (12 bit)
 */
phb_class::phb_class(uint16 phb_id_code)
  : qspec_param(PARAM_ID), phb_id_field( ((phb_id_code & 0xFFF) << 4) | 0x01 ) {

	// nothing to do
}


/**
 * Constructor for manual use.
 *
 * @param dscp_value a single standard DSCP value of a standard PHB 
 */
phb_class::phb_class(uint8 dscp_value)
  : qspec_param(PARAM_ID), phb_id_field( (dscp_value & 0x3F) << 10 ) {

	// nothing to do
}

/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter Id
 * @param dscp_value a DiffServ Code Point (6 bit)
 */
phb_class::phb_class(uint16 param_id, uint8 dscp_value)
  : qspec_param(param_id), phb_id_field( (dscp_value & 0x3F) << 10 ) {

	// nothing to do
}

/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID. This is usually not required and should
 * be used with care if QSPEC-T compliant output is desired.
 *
 * @param param_id the parameter Id
 * @param value a DiffServ Code Point (6 bit)
 */
phb_class::phb_class(uint16 param_id, uint16 phb_id)
  : qspec_param(param_id), phb_id_field( ((phb_id & 0xFFF) << 4) | 0x01 ) {

	// nothing to do
}


phb_class::~phb_class() {
	// nothing to do
}


phb_class *phb_class::new_instance() const {
	phb_class *q = NULL;
	catch_bad_alloc( q = new phb_class() );
	return q;
}


phb_class *phb_class::copy() const {
	phb_class *q = NULL;
	catch_bad_alloc( q = new phb_class(*this) );
	return q;
}


bool phb_class::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	uint16 body_raw = msg.decode16();
	msg.set_pos_r(2); // skip 2 bytes
	bytes_read = 4;

	phb_id_field = body_raw; // set PHB ID

	// Error: The part after the PHB-ID is not zero.
	if ( (body_raw & 0x0C) != 0 ) {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void phb_class::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	msg.encode16( phb_id_field ); // write PHB_ID
	msg.encode16(0); // reserved part

	bytes_written = 4;
}


bool phb_class::check_body() const {
  // The PHB ID is 12 bit
	return (((phb_id_field & 0x01) == 0) && ((phb_id_field & 0x3FD) == 0) )  // single DSCP or DSCP set
	  ||  ( ((phb_id_field & 0x01) == 1) && ((phb_id_field & 0x0C) == 0) ); // single PHB ID or PHB ID set
}


bool phb_class::equals_body(const qspec_param &param) const {

	const phb_class *other
		= dynamic_cast<const phb_class *>(&param);

	return other != NULL && phb_id_field == other->phb_id_field;
}


const char *phb_class::get_ie_name() const {
	return ie_name;
}


ostream &phb_class::print_attributes(ostream &os) const {
  return os << ", " << ((phb_id_field & 0x1) ? "PHB-ID" : "DSCP") 
	    << ((phb_id_field &0x2) ? " set = " : "= ") 
	    << ((phb_id_field & 0x1) ? (phb_id_field >> 4) : (phb_id_field >> 10));
}


/**
 * Returns the value of the DSCP field
 *
 * @return the value of the DSCP field (6 bit)
 */
uint8 phb_class::get_dscp_value() const {
  return (phb_id_field >> 10);
}


/**
 * Set the value of the DSCP field.
 *
 * @param val the new value of the DSCP field (6 bit)
 */
void 
phb_class::set_dscp_value(uint8 val) {
  phb_id_field = ((val & 0x3F) << 10);
}

/**
 * Set the value of the DSCP field as DSCP set.
 *
 * @param val the new value of the DSCP field (6 bit)
 */
void 
phb_class::set_dscp_set(uint8 val) {
  phb_id_field = ((val & 0x3F) << 10) | 0x02;
}


/**
 * is the  DSCP value a representative of a DSCP set?
 *
 * @return true if DSCP field is a set (6 bit)
 */
bool
phb_class::is_dscp_set() const {
  return (phb_id_field & 0x2) && ( (phb_id_field & 0x1) == 0 );
}

/**
 * Returns the value of the PHB ID
 *
 * @return the value of the PHB ID (12 bit)
 */
uint16 
phb_class::get_phb_id() const {
  return (phb_id_field >> 4);
}


/**
 * Set the value of a single PHB ID
 *
 * @param val the new value of the PHB ID
 */
void 
phb_class::set_phb_id(uint16 val) {
  phb_id_field = ((val & 0xFFF) << 4) | 0x01;
}


/**
 * Set the value of the PHB ID as PHB ID set.
 *
 * @param val the new value of the PHB ID set
 */
void 
phb_class::set_phb_id_set(uint16 val) {
  phb_id_field = ((val & 0xFFF) << 4) | 0x03;
}


/**
 * is the PHB ID value a representative of a PHB ID set?
 *
 * @return true if PHB ID field is a set (12 bit)
 */
bool
phb_class::is_phb_id_set() const {
  return (phb_id_field & 0x03) == 0x03;
}


// EOF
