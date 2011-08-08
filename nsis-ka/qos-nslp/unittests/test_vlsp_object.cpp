/*
 * Test for VLSP objects
 *
 * $Id:$
 * $HeadURL:$
 */
#include "test_suite.h"

#include "vlsp_object.h"

using namespace qos_nslp;


class test_vlsp_object : public CppUnit::TestCase {

	CPPUNIT_TEST_SUITE( test_vlsp_object );

	CPPUNIT_TEST( test_constructors );
	CPPUNIT_TEST( test_comparison );
	CPPUNIT_TEST( test_set_n_get );

	CPPUNIT_TEST_SUITE_END();

	public:
		void test_constructors() {
			vlsp_object vlsp_object_one, vlsp_object_two;
			CPPUNIT_ASSERT( vlsp_object_one.is_vlsp_object() && vlsp_object_two.is_vlsp_object() );
			CPPUNIT_ASSERT( vlsp_object_one.get_vnet_id() == vlsp_object_two.get_vnet_id() );
			CPPUNIT_ASSERT( vlsp_object_one.get_vnode_id_src() == vlsp_object_two.get_vnode_id_src() );
			CPPUNIT_ASSERT( vlsp_object_one.get_vnode_id_dst() == vlsp_object_two.get_vnode_id_dst() );
			CPPUNIT_ASSERT( vlsp_object_one.get_vif_id_src() == vlsp_object_two.get_vif_id_src() );
			CPPUNIT_ASSERT( vlsp_object_one.get_vif_id_dst() == vlsp_object_two.get_vif_id_dst() );
			CPPUNIT_ASSERT( vlsp_object_one.get_vlink_id() == vlsp_object_two.get_vlink_id() );
			CPPUNIT_ASSERT( vlsp_object_one.get_tunnel_type() == vlsp_object_two.get_tunnel_type() );

			ASSERT_BASICS_WORK( vlsp_object_one );
			ASSERT_BASICS_WORK( vlsp_object_two );
		}

		
		void test_comparison() {
			uint128 zero(0,0,0,0);
			uint128 one(0,0,0,1);
			uint128 two(0,0,0,1);
			vlsp_object vlsp_object_zero(zero,zero,zero,0,0,0,vlsp_object::tunnel_undefined),
				vlsp_object_one(one,one,two,1,2,1, vlsp_object::tunnel_IP_in_IP);

			vlsp_object vlsp_object_one_clone(vlsp_object_one);
			
			CPPUNIT_ASSERT( vlsp_object_one == vlsp_object_one_clone);
			CPPUNIT_ASSERT( vlsp_object_zero != vlsp_object_one ) ;			

		}

		void test_set_n_get() {
			uint128 zero(0,0,0,0);
			uint128 one(0,0,0,1);
			uint128 two(0,0,0,1);

			vlsp_object vlsp_object_one(one,one,two,1,2,1, vlsp_object::tunnel_IP_in_IP),
				vlsp_object_two(one,one,two,1,2,1, vlsp_object::tunnel_IP_in_IP);

			uint128 id_x(1,2,3,0xfffe5432L);
			uint128 id_y(1,2,3,0xfffe45ffL);
			CPPUNIT_ASSERT( vlsp_object_one == vlsp_object_two);
			vlsp_object_one.set_vnet_id(id_x); 
			CPPUNIT_ASSERT( vlsp_object_one != vlsp_object_two );
			CPPUNIT_ASSERT( vlsp_object_one.get_vnet_id() == id_x );

			vlsp_object_two.set_vnet_id(id_x);
			CPPUNIT_ASSERT( vlsp_object_one == vlsp_object_two);
			vlsp_object_two.set_vnode_id_src(id_x);
			CPPUNIT_ASSERT( vlsp_object_one != vlsp_object_two );
			CPPUNIT_ASSERT( vlsp_object_two.get_vnode_id_src() == id_x); 
			vlsp_object_two.set_vnode_id_dst(id_y);
			CPPUNIT_ASSERT( vlsp_object_two.get_vnode_id_dst() == id_y);

			vlsp_object_two.set_vif_id_src(42);
			CPPUNIT_ASSERT( vlsp_object_two.get_vif_id_src() == 42);

			vlsp_object_two.set_vif_id_dst(911);
			CPPUNIT_ASSERT( vlsp_object_two.get_vif_id_dst() == 911);

			vlsp_object_two.set_vlink_id(0x1234567);
			CPPUNIT_ASSERT( vlsp_object_two.get_vlink_id() == 0x1234567);


			vlsp_object_two.set_tunnel_type(vlsp_object::tunnel_VLAN);
			CPPUNIT_ASSERT( vlsp_object_two.get_tunnel_type() == vlsp_object::tunnel_VLAN);

		}

};

CPPUNIT_TEST_SUITE_REGISTRATION( test_vlsp_object );
