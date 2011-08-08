/*
 * Custom assertions to make testing easier.
 *
 * $Id: test_suite.cpp 2209 2006-10-18 09:22:37Z stud-matfried $
 * $HeadURL: https://svn.tm.kit.edu/nsis/ntlp/trunk/test/test_suite.cpp $
 */
#include <sstream>

#include "ie.h"
#include "test_suite.h"

using namespace CppUnit;
using namespace protlib;

static const IE::coding_t CODING = IE::protocol_v1;


// Format: "str: num"
static std::string mk_str(const std::string &str, uint32 num) {
	std::stringstream ss;

	ss << str << ": " << num;

	return ss.str();
}


/**
 * Test the IE::new_instance() method.
 */
void test_is_instantiable(const protlib::IE &ie, CppUnit::SourceLine line) {
	IE *ie2 = ie.new_instance();

	Asserter::failIf(
		ie2 == NULL,
		Message("IE::new_instance() returned NULL"), line);

	// Objects must not be the same.
	Asserter::failIf(
		&ie == ie2,
		Message("IE::new_instance() returned the same object"), line);

	delete ie2;
}


/**
 * Test the IE::copy() method.
 *
 * This depends on a working == operator.
 */
void test_is_copyable(const protlib::IE &ie, CppUnit::SourceLine line) {
	IE *ie2 = ie.copy();

	Asserter::failIf(
		ie2 == NULL,
		Message("IE::copy() returned NULL"), line);

	// Objects should be equal, but not the same.
	Asserter::failIf(
		&ie == ie2,
		Message("IE::copy() returned the same object"), line);

	Asserter::failIf(
		ie != *ie2,
		Message("IE::copy(): the returned object is not equal"), line);

	delete ie2;
}


/**
 * Test if the given IE is serializable.
 *
 * This checks if exceptions are thrown and if the reported sizes match the
 * actual written bytes to the NetMsg. If the test passes, IE::serialize()
 * and IE::get_serialized_size() are known to be consistent.
 */
void test_is_serializable(const IE &ie, SourceLine line) {
	NetMsg msg(NetMsg::max_size);
	uint32 bytes_written;

	try {
		ie.serialize(msg, CODING, bytes_written);
	}
	catch ( std::exception &e ) {
		std::stringstream ss;

		ss << "serialize() threw an exception: " << e.what();
		Asserter::fail(ss.str(), line);

		return;
	}

	/*
	 * Check if sizes match.
	 */
	Asserter::failIf(
		msg.get_pos() != bytes_written,
		Message("IE::serialize(): wrong number of written bytes "
			"returned"),
		line);

	Asserter::failIf(
		bytes_written != ie.get_serialized_size(CODING),
		Message("IE::get_serialized_size() and number of written bytes "
			"doesn't match"),
		line);
}


/**
 * Test a complete serialize/deserialize cycle.
 *
 * This depends on a correctly implemented IE::serialize(), as tested by the
 * test_is_serializeable() function above, a working IE::deserialize() and
 * a working IE::new_instance() implementation.
 */
void test_is_deserializable(const IE &ie, SourceLine line) {
	test_is_serializable(ie, line);

	NetMsg msg(ie.get_serialized_size(CODING));
	uint32 bytes_written;

	ie.serialize(msg, CODING, bytes_written);

	msg.set_pos(0);


	/*
	 * The NetMsg contains a serialized message. Now try to deserialize.
	 *
	 * A "blank" object is required to call deserialize() on. Otherwise
	 * the IEManager would be needed.
	 */

	IEErrorList errlist;
	uint32 bytes_read;

	IE *blank = ie.new_instance();
	IE *ret = NULL;

	try {
		ret = blank->deserialize(
			msg, CODING, errlist, bytes_read, false);
	}
	catch ( std::exception &e ) {
		std::stringstream ss;

		ss << "deserialize() threw an exception: " << e.what();
		Asserter::fail(ss.str(), line);

		return;
	}

	Asserter::failIf(
		ret == NULL,
		Message("IE::deserialize(): returned NULL"),
		line);

	Asserter::failIf(
		msg.get_pos() != bytes_read,
		Message("IE::deserialize(): wrong number of read bytes",
			mk_str("according to NetMsg", msg.get_pos()),
			mk_str("according to bytes_read", bytes_read)),
		line);

	Asserter::failIf(
		bytes_read < bytes_written,
		Message("IE::deserialize(): fewer bytes read than written "
			"by IE::serialize()"),
		line);

	Asserter::failIf(
		bytes_read > bytes_written,
		Message("IE::deserialize(): more bytes read than written "
			"by IE::serialize()"),
		line);


	delete blank;
}


/**
 * Tests basic capabilities of the given IE instance.
 *
 * This function runs formal tests on the following IE methods:
 *	- new_instance()
 *	- copy()
 *	- serialize()
 *	- deserialize()
 *
 * Note that a working == operator is required for the tests to work.
 */
void test_basics_work(const protlib::IE &ie, CppUnit::SourceLine line) {
	test_is_instantiable(ie, line);
	test_is_copyable(ie, line);
	test_is_serializable(ie, line);
	test_is_deserializable(ie, line);
}

// EOF
