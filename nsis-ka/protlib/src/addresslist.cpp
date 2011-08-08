#include "addresslist.h"
#include "rfc5014_hack.h"

#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <err.h>

using namespace protlib;
using namespace std;

AddressList::AddrProperty *AddressList::LocalAddr_P;
AddressList::AddrProperty *AddressList::ConfiguredAddr_P;
AddressList::AddrProperty *AddressList::HomeAddr_P;
AddressList::AddrProperty *AddressList::IgnoreAddr_P;
AddressList::AddrProperty *AddressList::AnyAddr_P;
AddressList::AddrProperty *AddressList::HAAddr_P;  
AddressList::AddrProperty *AddressList::AltHAAddr_P;
AddressList::AddrProperty *AddressList::HomeNet_P;
                

AddressList::AddressList()
{
	if (LocalAddr_P == 0) {
		LocalAddr_P = new AddrProperty("local");
		ConfiguredAddr_P = new AddrProperty("configured");
		HomeAddr_P = new AddrProperty("HoA");
		HAAddr_P = new AddrProperty("HA");
		AltHAAddr_P = new AddrProperty("HA(alt)");
		HomeNet_P = new AddrProperty("HomeNet");
		IgnoreAddr_P = new AddrProperty("ignore");
		AnyAddr_P = new AddrProperty("wildcard");
	}
	interfaces = 0;
}

AddressList::~AddressList()
{
	// Refcount AddressLists in order to GC properties?
}

AddressList::iflist_t *
AddressList::get_interfaces()
{
	iflist_t *iflist;

	if (interfaces != 0)
		iflist = new iflist_t(*interfaces);
	else {
		iflist = new iflist_t();
		getifaddrs_iflist(*iflist);
	}

	return iflist;
}

bool
AddressList::by_interface(bool start_empty)
{
	if (interfaces != 0)
		return false;

	interfaces = new iflist_t();
	if (!start_empty)
		getifaddrs_iflist(*interfaces);

	return true;
}

bool
AddressList::add_interface(char *name)
{
	if (interfaces == 0)
		return false;

	return (interfaces->insert(name)).second;
}

bool
AddressList::del_interface(char *name)
{
	if (interfaces == 0)
		return false;

	return (interfaces->erase(name) > 0);
}

bool
AddressList::add_property(const netaddress &na, AddrProperty *p, bool propagate)
{
	propmap_t *props, *lpfm_props;
	propmap_t::iterator it;
	addr2prop_t::node *node;

	node = prop_trie.lookup_node(na, false, false);
	if (node != NULL) {
		props = node->data;
		if (props == NULL) {
			props = new propmap_t();
			node->data = props;
		}
		props->insert(pair<AddrProperty *, bool>(p, propagate));
		
	} else {
		props = new propmap_t();
		props->insert(pair<AddrProperty *, bool>(p, propagate));
		node = prop_trie.insert(na, *props);
	}

	if (propagate)
		bequeath(node, p, true);

	// copy lpfm properties
	lpfm_props = prop_trie.lookup(na, true);
	if (lpfm_props == NULL)
		return true;

	for (it = lpfm_props->begin(); it != lpfm_props->end(); it++) {
		if ((*it).second)
			props->insert((*it));
	}

	return true;
}

bool
AddressList::del_property(const netaddress &na, AddrProperty *p, bool propagate)
{
	propmap_t *props, *lpfm_props;
	propmap_t::iterator it;
	addr2prop_t::node *node;

	node = prop_trie.lookup_node(na, false, true);
	if (node == NULL) {
		// no exact match
		if (!propagate) {
			node = prop_trie.lookup_node(na, true, true);
			if (node == NULL) {
				// no lpfm either, we're done
				return false;
			}

			props = node->data;
			it = props->find(p);
			if (it == props->end()) {
				// lpfm doesn't have p set -> done
				return false;
			}
		}
		// insert an empty propmap
		props = new propmap_t();
		node = prop_trie.insert(na, *props);

		// copy other lpfm properties
		lpfm_props = prop_trie.lookup(na, true);
		if (p != AnyAddr_P && lpfm_props != NULL) {
			for (it = lpfm_props->begin(); it != lpfm_props->end();
			    it++) {
				if ((*it).first != p && (*it).second)
					props->insert((*it));
			}
		}
	} else {
		props = node->data;
		if (p == AnyAddr_P) {
			props->clear();
		} else {
			it = props->find(p);
			if (it == props->end() && !propagate)
				return false;

			props->erase(it);
		}
	}

	if (propagate)
		bequeath(node, p, false);

	return true;
}

bool
AddressList::add_host_prop(const char *name, AddrProperty *p)
{
	netaddress na;
	sockaddr_in *sin;
	sockaddr_in6 *sin6;
	struct addrinfo hints = {0}, *res, *cur;
	int error;
	char buf[1024];

	if (name == NULL) {
		name = buf;
		if (gethostname(buf, sizeof(buf)) != 0)
			return false;
		buf[sizeof(buf) - 1] = '\0';
	}
	hints.ai_flags = AI_ADDRCONFIG | AI_CANONNAME;
	hints.ai_family = AF_UNSPEC;
	error = getaddrinfo(name, NULL, &hints, &res);
	if (error != 0)
		return false;

	for(cur = res; cur != NULL && error == 0; cur = cur->ai_next) {
		if (cur->ai_family == AF_INET) {
			sin = (struct sockaddr_in *)cur->ai_addr;
			na.set_ip(sin->sin_addr);
			na.set_pref_len(32);
		} else if (cur->ai_family == AF_INET6) {
			sin6 = (struct sockaddr_in6 *)cur->ai_addr;
			na.set_ip(sin6->sin6_addr);
			na.set_pref_len(128);
		} else
			continue;

		// cout << ++i << "XXMOB: " << na << endl;

		error += add_property(na, p) ? 0 : 1;
		// XXXMOB: for some reason we need a 'reset' here
		//         if we want to use /etc/hosts
		na.set_ip("127.0.0.1");
	}
	freeaddrinfo(res);

	return (error == 0);
}

bool
AddressList::del_host_prop(const char *name, AddrProperty *p)
{
	netaddress na;
	sockaddr_in *sin;
	sockaddr_in6 *sin6;
	struct addrinfo hints = {0}, *res, *cur;
	int error;
	char buf[1024];

	if (name == NULL) {
		name = buf;
		if (gethostname(buf, sizeof(buf)) != 0)
			return false;
		buf[sizeof(buf) - 1] = '\0';
	}
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_family = AF_UNSPEC;
	error = getaddrinfo(name, NULL, &hints, &res);
	if (error != 0)
		return false;

	for(cur = res; cur != NULL && error == 0; cur = cur->ai_next) {
		if (cur->ai_family == AF_INET) {
			sin = (struct sockaddr_in *)cur->ai_addr;
			na.set_ip(sin->sin_addr);
			na.set_pref_len(32);
		} else if (cur->ai_family == AF_INET6) {
			sin6 = (struct sockaddr_in6 *)cur->ai_addr;
			na.set_ip(sin6->sin6_addr);
			na.set_pref_len(128);
		} else
			continue;

		error += del_property(na, p) ? 0 : 1;
	}
	freeaddrinfo(res);

	return (error == 0);
}

bool
AddressList::ignore(netaddress &na, bool propagate)
{
	del_property(na, AnyAddr_P, propagate);
	return add_property(na, IgnoreAddr_P);
}

bool
AddressList::unignore(netaddress &na, bool propagate)
{
	return del_property(na, IgnoreAddr_P, propagate);
}

bool
AddressList::ignore_bogons(void)
{
	netaddress na;

	// according to http://www.cymru.com/Documents/bogon-list.html 
	// 4.8 (30 Apr 2009)
	na.set_ip("0.0.0.0");na.set_pref_len(7);
	ignore(na);
	na.set_ip("2.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("5.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("10.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("14.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("23.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("27.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("31.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("36.0.0.0");na.set_pref_len(7);
	ignore(na);
	na.set_ip("39.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("42.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("46.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("49.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("50.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("100.0.0.0");na.set_pref_len(6);
	ignore(na);
	na.set_ip("104.0.0.0");na.set_pref_len(5);
	ignore(na);
	na.set_ip("169.254.0.0");na.set_pref_len(16);
	ignore(na);
	na.set_ip("175.0.0.0");na.set_pref_len(7);
	ignore(na);
	na.set_ip("176.0.0.0");na.set_pref_len(5);
	ignore(na);
	na.set_ip("179.0.0.0");na.set_pref_len(5);
	ignore(na);
	na.set_ip("181.0.0.0");na.set_pref_len(6);
	ignore(na);
	na.set_ip("182.0.0.0");na.set_pref_len(6);
	ignore(na);
	na.set_ip("185.0.0.0");na.set_pref_len(6);
	ignore(na);
	na.set_ip("191.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("192.0.2.0");na.set_pref_len(24);
	ignore(na);
	na.set_ip("197.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("198.18.0.0");na.set_pref_len(15);
	ignore(na);
	na.set_ip("223.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("240.0.0.0");na.set_pref_len(4);
	ignore(na);
	// according to http://www.cymru.com/Bogons/v6bogon.html (2008-05-20)
	na.set_ip("2001:db8::");na.set_pref_len(32);
	ignore(na);
	na.set_ip("0000::");na.set_pref_len(3);
	ignore(na);
	na.set_ip("2000::");na.set_pref_len(16);
	ignore(na);
	na.set_ip("2001:1::");na.set_pref_len(32);
	ignore(na);
	na.set_ip("2001:2::");na.set_pref_len(31);
	ignore(na);
	na.set_ip("2001:4::");na.set_pref_len(30);
	ignore(na);
	na.set_ip("2001:8::");na.set_pref_len(29);
	ignore(na);
	na.set_ip("2001:10::");na.set_pref_len(28);
	ignore(na);
	na.set_ip("2001:20::");na.set_pref_len(27);
	ignore(na);
	na.set_ip("2001:40::");na.set_pref_len(26);
	ignore(na);
	na.set_ip("2001:80::");na.set_pref_len(25);
	ignore(na);
	na.set_ip("2001:100::");na.set_pref_len(24);
	ignore(na);
	na.set_ip("2001:1000::");na.set_pref_len(23);
	ignore(na);
	na.set_ip("2001:4e00::");na.set_pref_len(23);
	ignore(na);
	na.set_ip("2001:6000::");na.set_pref_len(19);
	ignore(na);
	na.set_ip("2001:c000::");na.set_pref_len(18);
	ignore(na);
	na.set_ip("2003:4000::");na.set_pref_len(18);
	ignore(na);
	na.set_ip("2003:8000::");na.set_pref_len(17);
	ignore(na);
	na.set_ip("2004::");na.set_pref_len(14);
	ignore(na);
	na.set_ip("2008::");na.set_pref_len(13);
	ignore(na);
	na.set_ip("2010::");na.set_pref_len(12);
	ignore(na);
	na.set_ip("2020::");na.set_pref_len(11);
	ignore(na);
	na.set_ip("2040::");na.set_pref_len(10);
	ignore(na);
	na.set_ip("2080::");na.set_pref_len(9);
	ignore(na);
	na.set_ip("2100::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2200::");na.set_pref_len(7);
	ignore(na);
	na.set_ip("2410::");na.set_pref_len(12);
	ignore(na);
	na.set_ip("2420::");na.set_pref_len(11);
	ignore(na);
	na.set_ip("2440::");na.set_pref_len(10);
	ignore(na);
	na.set_ip("2480::");na.set_pref_len(9);
	ignore(na);
	na.set_ip("2500::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2610:200::");na.set_pref_len(23);
	ignore(na);
	na.set_ip("2610:400::");na.set_pref_len(22);
	ignore(na);
	na.set_ip("2610:800::");na.set_pref_len(21);
	ignore(na);
	na.set_ip("2610:1000::");na.set_pref_len(20);
	ignore(na);
	na.set_ip("2610:2000::");na.set_pref_len(19);
	ignore(na);
	na.set_ip("2610:4000::");na.set_pref_len(18);
	ignore(na);
	na.set_ip("2610:8000::");na.set_pref_len(17);
	ignore(na);
	na.set_ip("2611::");na.set_pref_len(16);
	ignore(na);
	na.set_ip("2612::");na.set_pref_len(15);
	ignore(na);
	na.set_ip("2614::");na.set_pref_len(14);
	ignore(na);
	na.set_ip("2618::");na.set_pref_len(13);
	ignore(na);
	na.set_ip("2620:200::");na.set_pref_len(23);
	ignore(na);
	na.set_ip("2620:400::");na.set_pref_len(22);
	ignore(na);
	na.set_ip("2620:800::");na.set_pref_len(21);
	ignore(na);
	na.set_ip("2620:1000::");na.set_pref_len(20);
	ignore(na);
	na.set_ip("2620:2000::");na.set_pref_len(19);
	ignore(na);
	na.set_ip("2620:4000::");na.set_pref_len(18);
	ignore(na);
	na.set_ip("2620:8000::");na.set_pref_len(17);
	ignore(na);
	na.set_ip("2621::");na.set_pref_len(16);
	ignore(na);
	na.set_ip("2622::");na.set_pref_len(15);
	ignore(na);
	na.set_ip("2624::");na.set_pref_len(14);
	ignore(na);
	na.set_ip("2628::");na.set_pref_len(13);
	ignore(na);
	na.set_ip("2630::");na.set_pref_len(12);
	ignore(na);
	na.set_ip("2640::");na.set_pref_len(10);
	ignore(na);
	na.set_ip("2680::");na.set_pref_len(9);
	ignore(na);
	na.set_ip("2700::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2810::");na.set_pref_len(12);
	ignore(na);
	na.set_ip("2820::");na.set_pref_len(11);
	ignore(na);
	na.set_ip("2840::");na.set_pref_len(10);
	ignore(na);
	na.set_ip("2880::");na.set_pref_len(9);
	ignore(na);
	na.set_ip("2900::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2a10::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2a20::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2a40::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2a80::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2b00::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2c10::");na.set_pref_len(12);
	ignore(na);
	na.set_ip("2c20::");na.set_pref_len(11);
	ignore(na);
	na.set_ip("2c40::");na.set_pref_len(10);
	ignore(na);
	na.set_ip("2c80::");na.set_pref_len(9);
	ignore(na);
	na.set_ip("2d00::");na.set_pref_len(8);
	ignore(na);
	na.set_ip("2e00::");na.set_pref_len(7);
	ignore(na);
	na.set_ip("3000::");na.set_pref_len(4);
	ignore(na);
	na.set_ip("4000::");na.set_pref_len(2);
	ignore(na);
	na.set_ip("8000::");na.set_pref_len(1);
	ignore(na);
	na.set_ip("F000::");na.set_pref_len(5);
	ignore(na);
	na.set_ip("F800::");na.set_pref_len(6);
	ignore(na);
	na.set_ip("FC00::");na.set_pref_len(7);
	ignore(na);
	na.set_ip("FE00::");na.set_pref_len(9);
	ignore(na);

	return true;
}

bool
AddressList::ignore_locals(void)
{
	netaddress na;

	na.set_ip("10.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("172.16.0.0");na.set_pref_len(12);
	ignore(na);
	na.set_ip("192.168.0.0");na.set_pref_len(16);
	ignore(na);
	na.set_ip("FE80::");na.set_pref_len(10);
	ignore(na);
	na.set_ip("FEC0::");na.set_pref_len(10);
	ignore(na);

	return true;
}

bool
AddressList::ignore_loopback(void)
{
	netaddress na;

	na.set_ip("127.0.0.0");na.set_pref_len(8);
	ignore(na);
	na.set_ip("::1");na.set_pref_len(128);
	ignore(na);

	return true;
}

bool
AddressList::addr_is(netaddress &na, AddrProperty *prop)
{
	propmap_t *props;
	propmap_t::iterator it;

	if (addr_is_in(na, IgnoreAddr_P))
		return false;

	props = prop_trie.lookup(na, false);
	if (props != NULL) {
		it = props->find(prop);
		if (it != props->end()) {
			return true;
		}
	}

	if (prop != LocalAddr_P)
		return false;

	return getifaddrs_is_local(na);
}

bool
AddressList::addr_is_in(netaddress &na, AddrProperty *prop)
{
	addr2prop_t::node *node;
	propmap_t *props;
	propmap_t::iterator it;

	node = prop_trie.lookup_node(na, true, true);
	if (node == NULL)
		return false;

	props = node->data;
	it = props->find(prop);
	if (it == props->end())
		return false;

	if (!(*it).second && props != prop_trie.lookup(na, false))
			return false;

	return true;
}

AddressList::addrlist_t *
AddressList::get_addrs(AddrProperty *prop)
{
	addr2prop_t::node *node;
	netaddress na;
	addrlist_t *res = new addrlist_t();

	if (res == 0)
		return res;

	if (prop == LocalAddr_P || prop == AnyAddr_P)
		getifaddrs_get_addrs(*res);

	na.set_ip("0.0.0.0");
	na.set_pref_len(0);
	node = prop_trie.lookup_node(na, true, false);
	collect(node, prop, *res);

	na.set_ip("::");
	node = prop_trie.lookup_node(na, true, false);
	collect(node, prop, *res);

	return res;
}

netaddress *
AddressList::get_first(AddrProperty *p, bool IPv4)
{
	addr2prop_t::node *node;
	netaddress na;
	addrlist_t list;
	addrlist_t::iterator it;

	if (IPv4) {
		na.set_ip("0.0.0.0");
		na.set_pref_len(0);
	} else {
		na.set_ip("::");
		na.set_pref_len(0);
	}

	node = prop_trie.lookup_node(na, true, false);
	node = collect_first(node, p);
	if (node != NULL)
		return new netaddress(*node->key);

	if (p == LocalAddr_P) {
		getifaddrs_get_addrs(list);
		for (it = list.begin(); it != list.end(); it++)
			if ((*it).is_ipv4() == IPv4)
				return new netaddress(*it);
	}

	return NULL;
}


netaddress *
AddressList::get_src_addr(const netaddress &dest, uint32_t *prefs)
{
	netaddress *res;
	int sfd;

	netaddress canonical_dest(dest);
	
	// convert to canonical representation (do not use IPv4 mapped addresses!)
	// either plain IPv4 or plain IPv6
	canonical_dest.convert_to_ipv4();

	sfd = socket(canonical_dest.is_ipv4() ? AF_INET : AF_INET6, SOCK_DGRAM, 0);
	if (sfd == -1) {
		err(1, "socket");
		return NULL;
	}

#ifdef IPV6_ADDR_PREFERENCES
	/* XXX: IPV6_PREFER_SRC_COA does not work */
	if (prefs != NULL && (*prefs & IPV6_PREFER_SRC_COA)) {
		res = get_first(HomeAddr_P, canonical_dest.is_ipv4());
		if (res != NULL) {
			addrlist_t *alist;
			alist = get_addrs(LocalAddr_P);
			if (alist != NULL) {
				addrlist_t::iterator it;
				for (it = alist->begin(); it != alist->end();
				    it++) {
					if ((*it).is_ipv4() == canonical_dest.is_ipv4() &&
					    !addr_is(*it, HomeAddr_P)) {
						res = new netaddress(*it);
						return res;
					}
				}
				delete alist;
			}
		}
	}
#endif
	if (canonical_dest.is_ipv4()) {
		struct sockaddr_in sin = {0};
		socklen_t slen = sizeof(sin);
		sin.sin_family = AF_INET;
		sin.sin_port = htons(4);
		canonical_dest.get_ip(sin.sin_addr);
		if (connect(sfd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
			err(1, "connect");
			close(sfd);
			return NULL;
		}
		if (getsockname(sfd, (struct sockaddr *)&sin, &slen) == -1) {
			err(1, "getsockname");
			close(sfd);
			return NULL;
		}
		close(sfd);
		res = new netaddress();
		res->set_ip(sin.sin_addr);
		res->set_pref_len(32);
		return (res);
	} else {
		struct sockaddr_in6 sin6 = {0};
		socklen_t slen = sizeof(sin6);
		sin6.sin6_family = AF_INET6;
		sin6.sin6_port = htons(4);
		canonical_dest.get_ip(sin6.sin6_addr);
		if (connect(sfd, (struct sockaddr *)&sin6,
		    sizeof(sin6)) == -1) {
			err(1, "connect");
			close(sfd);
			return NULL;
		}
		if (getsockname(sfd, (struct sockaddr *)&sin6, &slen) == -1) {
			err(1, "getsockname");
			close(sfd);
			return NULL;
		}
		close(sfd);
		res = new netaddress();
		res->set_ip(sin6.sin6_addr);
		res->set_pref_len(128);
		return (res);
	}
}

void
AddressList::getifaddrs_iflist(iflist_t &list)
{
	struct ifaddrs *ifap, *cifa;

	if (::getifaddrs(&ifap) != 0)
		return;

	for (cifa = ifap; cifa != NULL; cifa = cifa->ifa_next) {
		list.insert(cifa->ifa_name);
	}

	freeifaddrs(ifap);
}

bool
AddressList::getifaddrs_is_local(netaddress &na)
{
	struct ifaddrs *ifap, *cifa;

	if (::getifaddrs(&ifap) != 0)
		return false;

	for (cifa = ifap; cifa != NULL; cifa = cifa->ifa_next) {
		hostaddress ha;

		if (cifa->ifa_addr == NULL)
			continue;

		if (cifa->ifa_addr->sa_family == AF_INET) {
			ha.set_ip(
			    ((struct sockaddr_in *)cifa->ifa_addr)->sin_addr);
		} else if (cifa->ifa_addr->sa_family == AF_INET6) {
			ha.set_ip(
			    ((struct sockaddr_in6 *)cifa->ifa_addr)->sin6_addr);
		} else {
			continue;
		}

		if (interfaces &&
		    interfaces->find(cifa->ifa_name) == interfaces->end())
			continue;

		if (ha.match_against(na) >= na.get_pref_len()) {
			freeifaddrs(ifap);
			return true;
		}
	}

	freeifaddrs(ifap);

	return false;
}

void
AddressList::getifaddrs_get_addrs(addrlist_t &list)
{
	struct ifaddrs *ifap, *cifa;

	if (::getifaddrs(&ifap) != 0)
		return;

	for (cifa = ifap; cifa != NULL; cifa = cifa->ifa_next) {
		hostaddress *ha;
		netaddress na;

		if (cifa->ifa_addr == NULL)
			continue;

		if (interfaces &&
		    interfaces->find(cifa->ifa_name) == interfaces->end())
			continue;

		if (cifa->ifa_addr->sa_family == AF_INET) {
			ha = new hostaddress;
			ha->set_ip(
			    ((struct sockaddr_in *)cifa->ifa_addr)->sin_addr);
			na.set_pref_len(32);
		} else if (cifa->ifa_addr->sa_family == AF_INET6) {
			ha = new hostaddress;
			ha->set_ip(
			    ((struct sockaddr_in6 *)cifa->ifa_addr)->sin6_addr);
			na.set_pref_len(128);
		} else {
			continue;
		}

		na.set_ip(*ha);
		if (!addr_is_in(na, IgnoreAddr_P))
			list.insert(*ha);
	}

	freeifaddrs(ifap);
}

void
AddressList::bequeath(addr2prop_t::node *head, AddrProperty *p, bool add)
{
	propmap_t *props;
	propmap_t::iterator it;

	if (p == AnyAddr_P && add)
		return;

	props = head->data;
	if (props != NULL) {
		if (p == AnyAddr_P) {
			props->clear();
		} else {
			if (add) {
				props->insert(pair<AddrProperty *, bool>
				    (p, true));
			} else {
				it = props->find(p);
				if (it != props->end())
					props->erase(it);
			}
		}
	}

	if (head->left->index > head->index)
		bequeath(head->left, p, add);
	if (head->right->index > head->index)
		bequeath(head->right, p, add);
}

void
AddressList::collect(addr2prop_t::node *head, AddrProperty *p,
    addrlist_t &list)
{
	propmap_t *props;
	propmap_t::iterator it;

	props = head->data;
	if (props != NULL) {
		if (p == AnyAddr_P) {
			it = props->begin();
		} else {
			it = props->find(p);
		}
		if (it != props->end()) {
			list.insert(*(new netaddress(*head->key)));
		}
	}

	if (head->left->index > head->index)
		collect(head->left, p, list);
	if (head->right->index > head->index)
		collect(head->right, p, list);
	
}

AddressList::addr2prop_t::node *
AddressList::collect_first(addr2prop_t::node *head, AddrProperty *p)
{
	addr2prop_t::node *res = NULL;
	propmap_t *props;
	propmap_t::iterator it;

	props = head->data;
	if (props != NULL) {
		if (p == AnyAddr_P) {
			it = props->begin();
		} else {
			it = props->find(p);
		}
		if (it != props->end()) {
			return head;
		}
	}

	if (head->left->index > head->index) {
		res = collect_first(head->left, p);
		if (res != NULL)
			return res;
	}
	if (head->right->index > head->index) {
		res = collect_first(head->right, p);
		if (res != NULL)
			return res;
	}

	return NULL;
}
