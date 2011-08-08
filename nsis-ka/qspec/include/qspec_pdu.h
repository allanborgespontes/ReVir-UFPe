/*
 * qspec_pdu.h - A QSPEC Template PDU
 *
 * $Id: qspec_pdu.h 4593 2009-10-20 14:30:51Z roehricht $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qspec/trunk/include/qspec_pdu.h $
 *
 */
#ifndef QSPEC__QSPEC_PDU_H
#define QSPEC__QSPEC_PDU_H

#include "ie.h"

#include "ie_store.h"
#include "qspec_object.h"


namespace qspec {
  using namespace protlib;


/**
 * QSPEC Message Sequence Types.
 *
 * All valid message sequence types available in QSPEC-T. The message sequence
 * is the first part of the QSPEC Procedure Identifier.
 */
enum msg_sequence_t {
	ms_two_way_transactions		   = 0,
	ms_sender_initiated_reservations   = 0,
	ms_three_way_transactions	   = 1,
	ms_receiver_initiated_reservations = 1,
	ms_resource_queries		   = 2
};


/**
 * A QSPEC Template PDU.
 *
 * This class is an implementation of QoS-NSLP QSPEC Template (QSPEC-T) and
 * represents a Quality of Service Specification (QSPEC). It can read and
 * initialize itself from or write itself into a NetMsg object using
 * deserialize()/serialize() respectively.
 *
 * It is possible to use this PDU as part of another PDU (see serialize()).
 */
class qspec_pdu : public IE {

  public:
	static const uint16 HEADER_LENGTH;
	static const uint8 QSPEC_VERSION;

	qspec_pdu();
	explicit qspec_pdu(qspec::msg_sequence_t msg_seq, uint8 obj_comb, uint8 qspec_type = 0, bool localQSPEC = false);
	explicit qspec_pdu(const qspec_pdu &other);

	virtual ~qspec_pdu();


	/*
	 * Inherited from IE:
	 */
	virtual qspec_pdu *new_instance() const;
	virtual qspec_pdu *copy() const;

	virtual qspec_pdu *deserialize(NetMsg &msg, coding_t coding,
			IEErrorList &errorlist, uint32 &bytes_read, bool skip);

	virtual void serialize(NetMsg &msg, coding_t coding,
			uint32 &bytes_written) const throw (IEError);

	virtual bool check() const;
	virtual bool supports_coding(coding_t c) const;
	virtual size_t get_serialized_size(coding_t coding) const;

	virtual bool operator==(const IE &ie) const;
	virtual const char *get_ie_name() const;
	virtual ostream &print(ostream &os, uint32 level, const uint32 indent,
			const char *name = NULL) const;

	virtual void register_ie(IEManager *iem) const;


	/*
	 * New methods:
	 */
	virtual uint8 get_version() const;

	virtual uint8 get_qspec_type() const;
	virtual void set_qspec_type(uint8 id);

	virtual uint8 get_msg_sequence() const;
	virtual void set_msg_sequence(uint8 seq);

	virtual uint8 get_obj_combination() const;
	virtual void set_obj_combination(uint8 comb);

	bool is_local_qspec() const { return local_QSPEC; };
	void set_local_qspec() { local_QSPEC= true; };

	bool is_initiator_qspec() const { return !local_QSPEC; };
	void set_initiator_qspec() { local_QSPEC= false; };

	virtual size_t get_num_objects() const;
	virtual qspec_object *get_object(uint16 object_id) const;
	virtual void set_object(qspec_object *obj);
	virtual qspec_object *remove_object(uint16 object_id);

	static uint16 extract_qspec_type(uint32 header_raw) throw ();

  private:
	void set_version(uint8 ver);

	static const char *const ie_name;

	/**
	 * QSPEC header fields.
	 */
	uint8 version;         // 4bit
	uint8 qspec_type;     // 5bit
	uint8 msg_sequence;    // 4bit
	uint8 obj_combination; // 4bit

	bool local_QSPEC; // flag: local QSPEC or initiator QSPEC

	/**
	 * Map QSPEC object ID to qspec_object
	 */
	ie_store objects;
	typedef ie_store::const_iterator obj_iter;

#ifdef BIG_HACK
	uint32 hack_len;
	uint8 *hack_buf;
#endif
};


} // namespace qspec

#endif // QSPEC__QSPEC_PDU_H
