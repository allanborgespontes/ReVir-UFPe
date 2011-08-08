/*
 * Test for RSNs
 *
 * $Id: test_rsn.cpp 6159 2011-05-18 15:24:52Z bless $
 * $HeadURL: https://svn.tm.kit.edu/nsis/qos-nslp/trunk/unittests/test_rsn.cpp $
 */
#include "test_suite.h"

#include "rsn.h"

using namespace qos_nslp;


class test_rsn : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( test_rsn );

	CPPUNIT_TEST( test_constructors );
	CPPUNIT_TEST( test_comparison );
	CPPUNIT_TEST( test_set_n_get );

	CPPUNIT_TEST_SUITE_END();

	public:
		void test_constructors() {
			rsn rsn_one, rsn_two;
			CPPUNIT_ASSERT( rsn_one.is_rsn() && rsn_two.is_rsn() );
			CPPUNIT_ASSERT( rsn_one.get_epoch_id() == rsn_two.get_epoch_id() );
			CPPUNIT_ASSERT( rsn_one.get() == rsn_two.get() );

			ASSERT_BASICS_WORK( rsn_one );
			ASSERT_BASICS_WORK( rsn_two );
		}

		
		void test_comparison() {
			rsn rsn_zero(0,0),
				rsn_one(1,1), 
				rsn_two(4711,42), 
				rsn_three(2,2), 
				rsn_four (1,2), 
				rsn_five (1,0x7FFFFFFFUL), // lower window (2^31-1) 
				rsn_six  (1,0x7FFFFFFEUL), // lower window (2^31-2)
				rsn_seven(1,0xFFFFFFFFUL), // upper window (2^32-1)
				rsn_eight(2,0x3FFFFFFFUL), // lower window (2^30-1)
				rsn_nine (1,0x80000000UL), // upper window (2^31)
				rsn_ten  (1,0xFF000000UL); // upper window (2^31+2^30+...+2^24)

			rsn rsn_one_clone(rsn_one);
			
			CPPUNIT_ASSERT( rsn_one == rsn_one_clone);
			CPPUNIT_ASSERT( rsn_zero != rsn_one ) ;

			CPPUNIT_ASSERT( (rsn_one != rsn_two) );
			CPPUNIT_ASSERT( (rsn_one != rsn_four)  );
			CPPUNIT_ASSERT( (rsn_two != rsn_three) );
			CPPUNIT_ASSERT( (rsn_one != rsn_three) );
			CPPUNIT_ASSERT( (rsn_four == rsn_three) == false );
			CPPUNIT_ASSERT( (rsn_four != rsn_three) );

			
			CPPUNIT_ASSERT( rsn_zero < rsn_one );
			CPPUNIT_ASSERT( rsn_one < rsn_two );
			CPPUNIT_ASSERT( rsn_one < rsn_three );
			CPPUNIT_ASSERT( rsn_three < rsn_two );
			CPPUNIT_ASSERT( rsn_one < rsn_four );
			CPPUNIT_ASSERT( rsn_four < rsn_two );
			CPPUNIT_ASSERT( rsn_one < rsn_five );
			CPPUNIT_ASSERT( rsn_one < rsn_six );
			CPPUNIT_ASSERT( rsn_seven < rsn_one );

			// one cannot compare 2^31-1 and 2^32-1 with < and >
			CPPUNIT_ASSERT( ((rsn_seven < rsn_five) == false) && (rsn_seven > rsn_five) == false );
			CPPUNIT_ASSERT( rsn_six < rsn_five );
			CPPUNIT_ASSERT( rsn_six > rsn_seven );
			CPPUNIT_ASSERT( rsn_one < rsn_eight );
			CPPUNIT_ASSERT( rsn_two < rsn_eight );
			CPPUNIT_ASSERT( rsn_three < rsn_eight );
			CPPUNIT_ASSERT( rsn_eight < rsn_five );
			CPPUNIT_ASSERT( rsn_eight < rsn_six );
			CPPUNIT_ASSERT( rsn_one < rsn_nine );

			CPPUNIT_ASSERT( rsn_nine < rsn_ten );
			CPPUNIT_ASSERT( rsn_five < rsn_ten );
			CPPUNIT_ASSERT( rsn_ten < rsn_one );
			CPPUNIT_ASSERT( rsn_ten < rsn_two );
			CPPUNIT_ASSERT( rsn_ten < rsn_three );
			CPPUNIT_ASSERT( rsn_ten < rsn_four );
			CPPUNIT_ASSERT( rsn_ten < rsn_seven );
			CPPUNIT_ASSERT( rsn_ten < rsn_eight );

			rsn rsn_tmp(0,0); 
			rsn rsn_tmpinc;
			for (uint32 start= 0; start < 0xFC000000UL; start+= 0x4000000UL)
			{
				rsn_tmp.set(start);
				rsn_tmpinc= rsn_tmp;
				rsn_tmpinc++;
				CPPUNIT_ASSERT(rsn_tmp < rsn_tmpinc);
			}
		}

		void test_set_n_get() {
			rsn rsn_one(1,1), rsn_two(1,1);

			CPPUNIT_ASSERT( rsn_one == rsn_two);
			rsn_one.set(2); 
			CPPUNIT_ASSERT( rsn_one > rsn_two );
			rsn_two.set(2);
			CPPUNIT_ASSERT( rsn_one == rsn_two);
			rsn_one.set(rsn_two.get() + 1);
			CPPUNIT_ASSERT( rsn_one > rsn_two);
			rsn_two.set(3);
			CPPUNIT_ASSERT( rsn_one == rsn_two);
			rsn_two.increment();
			CPPUNIT_ASSERT( rsn_two > rsn_one);
			rsn_one.increment();
			CPPUNIT_ASSERT( rsn_one == rsn_two);
			rsn_two++;
			CPPUNIT_ASSERT( rsn_two > rsn_one);
			rsn_one++;
			CPPUNIT_ASSERT( rsn_one == rsn_two);
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION( test_rsn );
