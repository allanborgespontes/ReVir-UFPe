#ifndef ADDRESS_LIST_H
#define ADDRESS_LIST_H

#include "radixtrie.h"

namespace protlib {
/*
 * AddressList
 */

class AddressList {
public:
	class AddrProperty {
		public:
			AddrProperty(const char *name) {
				pname = new string(name);
			}
			~AddrProperty() {
				delete pname;
			};

			string *pname;
	};

	// Default properties, N.B. Ignore will remove all other properties
	static AddrProperty *LocalAddr_P;        // any physically address assigned to an i/f
	static AddrProperty *ConfiguredAddr_P;   // address assigned by configuration
	static AddrProperty *HomeAddr_P;         // Home Address (MobileIP)
	static AddrProperty *HAAddr_P;           // Home Agent Address (MobileIP)
	static AddrProperty *AltHAAddr_P;        // Alternative Home Agent Address (MobileIP)
	static AddrProperty *HomeNet_P;          // Home Network Address

private:
	// This is special
	static AddrProperty *IgnoreAddr_P;
	static AddrProperty *AnyAddr_P;

public:
	struct ltstr {
		bool operator()(const char* s1, const char* s2) const
		{ return strcmp(s1, s2) < 0; }
	};
	struct ltna {
		bool operator()(const netaddress &s1, const netaddress &s2)
		    const {
		    	if (s1.is_ipv4() != s2.is_ipv4())
		    		return (s1.is_ipv4());
			int cmp, pos = 0;
			cmp = s1.rdx_cmp(&s2, &pos);
			return (cmp < 0);
		}
		
	};
	typedef set<char *, ltstr>	iflist_t;
	typedef set<netaddress, ltna>	addrlist_t;

	AddressList();
	~AddressList();

	// Configure by interfaces
	// Return a list of all local interfaces, which are monitored
	iflist_t *get_interfaces();
	// Begin interface list (otherwise the following have no effect)
	// If start_empty is true use an empty list, otherwise add all
	// currently configured interfaces
	bool by_interface(bool start_empty = true);
	// Add interface to monitoring
	bool add_interface(char *name);
	// Remove interface from monitoring
	bool del_interface(char *name);

	// Manage properties
	// If 'all' is true operate on all matching addresses as well
	bool add_property(const netaddress &na, AddrProperty *p = ConfiguredAddr_P,
	    bool propagate = true);
	bool del_property(const netaddress &na, AddrProperty *p = ConfiguredAddr_P,
	    bool propagate = true);
	inline bool purge_properties(netaddress &na, bool prop = true) {
		return del_property(na, AnyAddr_P, prop);
	};
	bool add_host_prop(const char *name, AddrProperty *p = ConfiguredAddr_P);
	bool del_host_prop(const char *name, AddrProperty *p = ConfiguredAddr_P);
	inline bool purge_host_prop(const char *name) {
		return del_host_prop(name, AnyAddr_P);
	}

	// Special property
	bool ignore(netaddress &na, bool propagate = true);
	bool unignore(netaddress &na, bool propagate = true);
	bool ignore_bogons(void);
	bool ignore_locals(void);
	bool ignore_loopback(void);
	
	// Check if the given address has the given property
	bool addr_is(netaddress &na, AddrProperty *prop);
	bool addr_is(const hostaddress &ha, AddrProperty *prop) {
		netaddress na(ha);
		return addr_is(na, prop);
	}
	// Check if the given address is in a network with the given property
	bool addr_is_in(netaddress &na, AddrProperty *prop);
	bool addr_is_in(const hostaddress &ha, AddrProperty *prop) {
		netaddress na(ha);
		return addr_is_in(na, prop);
	}

	addrlist_t *get_addrs(AddrProperty *prop = LocalAddr_P);
	netaddress *get_first(AddrProperty *p = LocalAddr_P, bool IPv4 = true);

	netaddress *get_src_addr(const netaddress &dest, uint32_t *prefs);
private:
	typedef map<AddrProperty *, bool>	propmap_t;
	typedef RadixTrie<propmap_t>		addr2prop_t;

	iflist_t *interfaces;
	addr2prop_t prop_trie;

	// Backends for public functions
	void getifaddrs_iflist(iflist_t &list);
	bool getifaddrs_is_local(netaddress &na);
	void getifaddrs_get_addrs(addrlist_t &list);

	void bequeath(addr2prop_t::node *head, AddrProperty *p,
	    bool add = true);
	void collect(addr2prop_t::node *head, AddrProperty *p,
	    addrlist_t &list);
	addr2prop_t::node *collect_first(addr2prop_t::node *head,
	    AddrProperty *p);
};

inline ostream &operator<<(ostream &out, const AddressList::AddrProperty &prop) {
	return out << *prop.pname;
}

} // end namespace
#endif
