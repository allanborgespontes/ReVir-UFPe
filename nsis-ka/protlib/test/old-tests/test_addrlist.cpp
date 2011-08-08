#include <iostream>
#include "address.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() {
    AddressList test;
    netaddress na;

    // initalize netdb
    protlib::tsdb::init();

    cout << "linklocal test: ";
    na.set_ip("127.0.0.1");
    na.set_pref_len(32);
    if (!test.addr_is(na, AddressList::LocalAddr_P)) {
        cout << "Huh?  " << na << " is not local!" << endl;
        return 1;
    }
    na.set_ip("::1");
    na.set_pref_len(128);
    if (!test.addr_is(na, AddressList::LocalAddr_P)) {
        cout << "Huh?  " << na << " is not local!" << endl;
        return 1;
    }
    cout << "PASSED" << endl;

    cout << "unlikely local test: ";
    na.set_ip("66.249.93.104");
    na.set_pref_len(32);
    if (test.addr_is(na, AddressList::LocalAddr_P)) {
        cout << "Huh?  Running at google.de" << endl;
        return 1;
    }
    na.set_ip("2001:200:0:8002:203:47ff:fea5:3085");
    na.set_pref_len(128);
    if (test.addr_is(na, AddressList::LocalAddr_P)) {
        cout << "Huh?  Running at www.kame.net" << endl;
        return 1;
    }
    cout << "PASSED" << endl;

    cout << "property test:" << endl;
    AddressList::AddrProperty LanAddr("LAN");
    na.set_ip("10.0.0.1");
    na.set_pref_len(8);
    cout << "adding " << LanAddr << " to " << na << endl;
    test.add_property(na, &LanAddr);
    if (!test.addr_is(na, &LanAddr)) {
        cout << "didn't stick to " << na << endl;
        return 1;
    }
    na.set_pref_len(32);
    if (!test.addr_is_in(na, &LanAddr)) {
        cout << "didn't stick to " << na << endl;
        return 1;
    }
    na.set_pref_len(16);
    cout << "removing " << LanAddr << " from " << na << endl;
    test.del_property(na, &LanAddr, true);
    na.set_pref_len(8);
    if (!test.addr_is_in(na, &LanAddr)) {
        cout << "was deleted from " << na << endl;
        return 1;
    }
    na.set_pref_len(32);
    if (test.addr_is_in(na, &LanAddr)) {
        cout << "was not deleted from " << na << endl;
        return 1;
    }
    cout << "PASSED" << endl;
    

    AddressList::iflist_t *iflist = test.get_interfaces();
    if (iflist != 0) {
        cout << "Interface list: ";
        AddressList::iflist_t::iterator it;
        for (it = iflist->begin(); it != iflist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete iflist;

    AddressList::addrlist_t *alist = test.get_addrs();
    if (alist != 0) {
        cout << "Address list: ";
        AddressList::addrlist_t::iterator it;
        for (it = alist->begin(); it != alist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete alist;

    cout << "Selecting interfaces: eth0, lo" << endl;
    test.by_interface();
    test.add_interface("eth0");
    test.add_interface("lo");

    iflist = test.get_interfaces();
    if (iflist != 0) {
        cout << "Interface list: ";
        AddressList::iflist_t::iterator it;
        for (it = iflist->begin(); it != iflist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete iflist;

    alist = test.get_addrs();
    if (alist != 0) {
        cout << "Address list: ";
        AddressList::addrlist_t::iterator it;
        for (it = alist->begin(); it != alist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete alist;

    test.ignore_loopback();
    alist = test.get_addrs();
    if (alist != 0) {
        cout << "Address list (w/o loopback): ";
        AddressList::addrlist_t::iterator it;
        for (it = alist->begin(); it != alist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete alist;

    na.set_ip("192.168.0.1");
    na.set_pref_len(32);
    test.add_property(na, AddressList::LocalAddr_P);    
    if (!test.addr_is(na, AddressList::LocalAddr_P)) {
        cout << "Huh?  " << na << " is not local!" << endl;
        return 1;
    }

    alist = test.get_addrs();
    if (alist != 0) {
        cout << "Address list: ";
        AddressList::addrlist_t::iterator it;
        for (it = alist->begin(); it != alist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete alist;

    test.add_host_prop(NULL, AddressList::ConfiguredAddr_P);
    alist = test.get_addrs(AddressList::ConfiguredAddr_P);
    if (alist != 0) {
        cout << "Address list for this host: ";
        AddressList::addrlist_t::iterator it;
        for (it = alist->begin(); it != alist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete alist;

    AddressList::AddrProperty TestAddr("TEST");
    test.add_host_prop("www.tm.uka.de", &TestAddr);
    alist = test.get_addrs(&TestAddr);
    if (alist != 0) {
        cout << "Address list for www.tm.uka.de: ";
        AddressList::addrlist_t::iterator it;
        for (it = alist->begin(); it != alist->end(); it++)
            cout << *it << ", ";
        cout << endl;
    }
    delete alist;

    netaddress *na1;
    na1 = test.get_first(&TestAddr, true);
    cout << "first IPv4 address for www.tm.uka.de: " << *na1 << endl;
    na1 = test.get_first(&TestAddr, false);
    cout << "first IPv4 address for www.tm.uka.de: " << *na1 << endl;

    na1 = test.get_first(AddressList::LocalAddr_P, true);
    cout << "first local IPv4 address: " << *na1 << endl;
    na1 = test.get_first(AddressList::LocalAddr_P, false);
    cout << "first local IPv4 address: " << *na1 << endl;

    string dest;
    na.set_pref_len(128);
    cout << "Find src for: ";
    while (cin >> dest) {
        hostaddress *ha;
        na.set_ip(dest.c_str());
        ha = test.get_src_addr(na, NULL);
        cout << *ha << endl;
        cout << "Find src for: ";    
    }
}
