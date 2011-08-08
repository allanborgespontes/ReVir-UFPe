/*
 * t_mod.h - The QSPEC Traffic Model (TMOD) Parameters (TMOD-1,TMOD-2), expressed by a Token Bucket
 *
 * $Id: t_mod.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/t_mod.h $
 *
 */
#ifndef QSPEC__T_MOD_H
#define QSPEC__T_MOD_H

#include "qspec_param.h"


namespace qspec {
  using namespace protlib;


/**
 * The QSPEC Traffic Model (TMOD) Parameter.
 *
 * Note that QSPEC-T defines two TMOD parameters TMOD-1 and TMOD-2, each with a
 * different parameter ID. This class' register_ie() method registers
 * itself for both IDs. An object of this class represents one parameter.
 *
 *  If, for example, TMOD is set to specify bandwidth only, then set r = peak
 *  rate = p, b = large, m = large.  As another example if TMOD is set
 *  for TCP traffic, then set r = average rate, b = large, p = large.
 *
 * The TMOD corresponds to the TOKEN_BUCKET_TSPEC of the IntServ model
 * (RFC 2210, RFC 2215):
 * The token bucket specification includes an average or token rate [r]
 * and a bucket depth [b].  Both [r] and [b] must be positive.
 *
 * The token rate [r] is measured in bytes of IP datagrams per second.
 * Values of this parameter may range from 1 byte per second to 40
 * terabytes per second. In practice, only the first few digits of the
 * [r] and [p] parameters are significant, so the use of floating point
 * representations, accurate to at least 0.1% is encouraged.
 *
 * The bucket depth, [b], is measured in bytes. Values of this parameter
 * may range from 1 byte to 250 gigabytes. In practice, only the first
 * few digits of the [b] parameter are significant, so the use of
 * floating point representations, accurate to at least 0.1% is
 * encouraged.
 *
 * The peak traffic rate [p] is measured in bytes of IP datagrams per
 * second. Values of this parameter may range from 1 byte per second to
 * 40 terabytes per second. In practice, only the first few digits of
 * the [r] and [p] parameters are significant, so the use of floating
 * point representations, accurate to at least 0.1% is encouraged. The
 * peak rate value may be set to positive infinity, indicating that it
 * is unknown or unspecified.
 *
 * The range of values allowed for these parameters is intentionally
 * large to allow for future network technologies. A particular network
 * element is not expected to support the full range of values.
 *
 * The minimum policed unit, [m], is an integer measured in bytes.  This
 * size includes the application data and all protocol headers at or
 * above the IP level (IP, TCP, UDP, RTP, etc.). It does not include the
 * link-level header size, because these headers will change in size as
 * the packet crosses different portions of the internetwork.
 *
 * All IP datagrams less than size [m] are treated as being of size m
 * for purposes of resource allocation and policing. The purpose of this
 * parameter is to allow reasonable estimation of the per-packet
 * resources needed to process a flow's packets (maximum packet rate can
 * be computed from the [b] and [m] terms) and to reasonably bound the
 * bandwidth overhead consumed by the flow's link-level packet headers.
 * The maximum bandwidth overhead consumed by link-level headers when
 * carrying a flow's packets is bounded by the ratio of the link-level
 * header size to [m]. Without the [m] term, it would be necessary to
 * compute this bandwidth overhead assuming that every flow was always
 * sending minimum-sized packets, which is unacceptable.
 *
 *
 */
class t_mod : public qspec_param {

  public:
	static const uint16 PARAM_ID1;
	static const uint16 PARAM_ID2;

	t_mod();
	explicit t_mod(float rate, float bucket_depth, float peak_data_rate,
			uint32 min_policed_unit);
	explicit t_mod(uint16 param_id, float rate, float bucket_depth,
			float peak_data_rate, uint32 min_policed_unit);

	virtual ~t_mod();

	virtual t_mod *new_instance() const;
	virtual t_mod *copy() const;

	virtual size_t get_serialized_size(coding_t coding) const;
	virtual void register_ie(IEManager *iem) const;

	virtual bool check_body() const;
	virtual bool equals_body(const qspec_param &other) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print_attributes(ostream &os) const;


	virtual bool deserialize_body(NetMsg &msg, coding_t coding,
			uint16 body_length, IEErrorList &err,
			uint32 &bytes_read, bool skip);

	virtual void serialize_body(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const;


	/*
	 * New methods
	 */
	float get_rate() const { return rate; }
	void set_rate(float value) { rate = value; }

	float get_bucket_depth() const { return bucket_depth; }
	void set_bucket_depth(float value) { bucket_depth = value; }

	float get_peak_data_rate() const { return peak_data_rate; }
	void set_peak_data_rate(float value) { peak_data_rate = value; }

	uint32 get_min_policed_unit() const { return min_policed_unit; }
	void set_min_policed_unit(uint32 value) { min_policed_unit = value; }

  private:
	// Disallow assignment for now.
	t_mod &operator=(const t_mod &other);

	static const char *const ie_name;

	float rate;              ///< token rate [r], measured in bytes of IP datagrams per second
	float bucket_depth;      ///< bucket depth, [b], measured in bytes
	float peak_data_rate;    ///< measured in bytes of IP datagrams per second.
	uint32 min_policed_unit; ///< minimum policed unit, [m], is an integer measured in bytes.  
	                         ///  This size includes the application data and all protocol headers at or
	                         ///  above the IP level (IP, TCP, UDP, RTP, etc.).
};


} // namespace qspec

#endif // QSPEC__T_MOD_H
