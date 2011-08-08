/*
 * t_mod.cpp - The QSPEC Token Bucket Parameters
 *
 * $Id: t_mod.cpp 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/src/t_mod.cpp $
 */
#include <limits>

#include "qspec_ie.h"
#include "t_mod.h"
#include "logfile.h"


using namespace qspec;



/**
 * The two officially assigned parameter ID
 */
const uint16 t_mod::PARAM_ID1 = 1; // TMOD-1
const uint16 t_mod::PARAM_ID2 = 2; // TMOD-2

const char *const t_mod::ie_name = "TMOD";

/**
 * Default constructor.
 */
t_mod::t_mod()
		: qspec_param(PARAM_ID1), rate(0.0), bucket_depth(0.0),
		  peak_data_rate(0.0), min_policed_unit(0) {

	// nothing to do, PARAM_ID1 is mandatory
}


/**
 * Constructor for manual use.
 *
 * Note that the parameter ID defaults to the first parameter ID assigned
 * in QSPEC-T.
 *
 * @param rate the Token Bucket rate
 * @param bucket_depth the Token Bucket bucket_depth
 * @param peak_data_rate the peak data rate
 * @param min_policed_unit the minimum policed unit
 */
t_mod::t_mod(float rate, float bucket_depth, float peak_data_rate, uint32 min_policed_unit)
		: qspec_param(PARAM_ID1), rate(rate), bucket_depth(bucket_depth),
		  peak_data_rate(peak_data_rate),
		  min_policed_unit(min_policed_unit) {

	// nothing to do, PARAM_ID1 is mandatory
}


/**
 * Constructor for manual use.
 *
 * Allows to set the parameter ID.
 *
 * @param param_id the QSPEC Parameter ID
 * @param rate the Token Bucket rate
 * @param bucket_depth the Token Bucket size
 * @param peak_data_rate the peak data rate
 * @param min_policed_unit the minimum policed unit
 */
t_mod::t_mod(uint16 param_id, float rate, float bucket_depth,
		float peak_data_rate, uint32 min_policed_unit)
		: qspec_param(param_id), rate(rate), bucket_depth(bucket_depth),
		  peak_data_rate(peak_data_rate),
		  min_policed_unit(min_policed_unit) {

	if ( get_parameter_id() == PARAM_ID2 )
		set_mandatory(false);
}


t_mod::~t_mod() {
	// nothing to do
}


t_mod *t_mod::new_instance() const {
	t_mod *q = NULL;
	catch_bad_alloc( q = new t_mod() );
	return q;
}


t_mod *t_mod::copy() const {
	t_mod *q = NULL;
	catch_bad_alloc( q = new t_mod(*this) );
	return q;
}


bool t_mod::deserialize_body(NetMsg &msg, coding_t coding,
		uint16 body_length, IEErrorList &err, uint32 &bytes_read,
		bool skip) {

	bytes_read = 0;

	set_rate(decode_float(msg));
	bytes_read += 4;

	set_bucket_depth(decode_float(msg));
	bytes_read += 4;

	set_peak_data_rate(decode_float(msg));
	bytes_read += 4;

	set_min_policed_unit(msg.decode32());
	bytes_read += 4;

	// Check for invalid values.
	if ( ! check() ) {
		catch_bad_alloc( err.put( 
			new PDUSyntaxError(coding, get_category(),
				get_parameter_id(), 0, msg.get_pos()-4)) );

		if ( ! skip )
			return false;
	}

	return true; // success
}


void t_mod::serialize_body(NetMsg &msg, coding_t coding,
		uint32 &bytes_written) const {

	bytes_written = 0;

	encode(msg, get_rate());
	bytes_written += 4;

	encode(msg, get_bucket_depth());
	bytes_written += 4;

	encode(msg, get_peak_data_rate());
	bytes_written += 4;

	msg.encode32(get_min_policed_unit());
	bytes_written += 4;

}


// Note: It is not possible to detect a negative zero representation (-0.0).
bool t_mod::check_body() const {
  //cout << "t_mod::check_body() rate:" << is_non_negative_finite(get_rate())
  //     << " bucket depth:" << is_non_negative_finite(get_bucket_depth())
  //     << " peak data rate:" << is_non_negative_finite(get_peak_data_rate()) << endl;

	// the other attributes can't be invalid (syntactically)
	return is_non_negative_finite(get_rate())
		&& is_non_negative_finite(get_bucket_depth())
	        && (is_non_negative_finite(get_peak_data_rate()) || 
	      get_peak_data_rate() == std::numeric_limits<float>::infinity());
}


// Note: Comparing floating point values is a tricky business. Using a
//       different definition for == might be necessary.
bool t_mod::equals_body(const qspec_param &param) const {

	const t_mod *other
		= dynamic_cast<const t_mod *>(&param);

	return other != NULL && get_rate() == other->get_rate()
		&& get_bucket_depth() == other->get_bucket_depth()
		&& get_peak_data_rate() == other->get_peak_data_rate()
		&& get_min_policed_unit() == other->get_min_policed_unit() ;
}


const char *t_mod::get_ie_name() const {
	return ie_name;
}


size_t t_mod::get_serialized_size(coding_t coding) const {
	// header, three 32bit floats, one 32bit unsigned integer
	return qspec_param::HEADER_LENGTH + 3*4 + 1*4;
}


void t_mod::register_ie(IEManager *iem) const {
	iem->register_ie(cat_qspec_param, PARAM_ID1, 0, this);
	iem->register_ie(cat_qspec_param, PARAM_ID2, 0, this->copy());
}


ostream &t_mod::print_attributes(ostream &os) const {
	return os << ", rate=" << get_rate()
		<< ", bucket_depth=" << get_bucket_depth()
		<< ", peak_data_rate=" << get_peak_data_rate()
		  << ", min_policed_unit=" << get_min_policed_unit();
}



// EOF
