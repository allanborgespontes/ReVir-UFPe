#include <iostream>
#include "address.h"

using namespace protlib;
using namespace protlib::log;

logfile commonlog;
protlib::log::logfile& protlib::log::DefaultLog(commonlog);

int main() {
/*
    netaddress n0, n1;
    int pos, cmp;
    string addr;

    n0.set_pref_len(120);
    n1.set_pref_len(112);
    do {
        cout << "Addr1: ";
        cin >> addr;
        n0.set_ipv6(addr.c_str());
        cout << " " << n0 << endl;
        cout << "Addr2: ";
        cin >> addr;
        n1.set_ipv6(addr.c_str());
        cout << " " << n1 << endl;
        pos = 105;
        while (pos >= 0) {
            cmp = n0.rdx_cmp(&n1, &pos);
            cout << n0 << " x " << n1 << " cmp:" << cmp << " pos:" << pos << endl;
        }
    } while  (true);
*/

    RadixTrie<netaddress> test;
    hostaddress ha("192.168.64.1");
    netaddress na(ha), *nap;
    na.set_pref_len(8);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();
    na.set_pref_len(16);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();
    na.set_pref_len(24);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();

/*
    na.set_ipv4("0.0.0.0");
    na.set_pref_len(8);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();
    na.set_pref_len(16);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();
    na.set_pref_len(24);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();
 */

    na.set_pref_len(32);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();
    na.set_pref_len(17);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();

    na.set_ip("10.0.0.1");
    na.set_pref_len(16);
    for (int i = 1; i < 5; i++) {
    nap = new netaddress(na);
    test.insert(na, *nap);
    }
    test.print();

    na.set_pref_len(25);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();

    na.set_ip("100.0.0.1");
    na.set_pref_len(16);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();

    na.set_ip("10.0.255.255");
    na.set_pref_len(16);
    nap = new netaddress(na);
    test.insert(na, *nap);
    test.print();

    for (int i = 0; i < 33; i++) {
        na.set_pref_len(i);
        RadixTrie<netaddress>::node *n;
        n = test.lookup_node(na);
        if (n != NULL && n->data)
            cout << na << " -> " << *n->key << "=>" << *n->data << endl;
        n = test.lookup_node(na, false);
        if (n != NULL && n->data)
            cout << "XACT: " << na << " -> " << *n->key << "=>" << *n->data << endl;
    }


    na.set_ip("10.0.0.1");
    for (int i = 0; i < 33; i++) {
        na.set_pref_len(i);
        RadixTrie<netaddress>::node *n;
        n = test.lookup_node(na);
        if (n != NULL && n->data)
            cout << na << " -> " << *n->key << "=>" << *n->data << endl;
        n = test.lookup_node(na, false);
        if (n != NULL && n->data)
            cout << "XACT: " << na << " -> " << *n->key << "=>" << *n->data << endl;
    }
    na.set_ip("192.168.4.1");
    for (int i = 0; i < 33; i++) {
        na.set_pref_len(i);
        RadixTrie<netaddress>::node *n;
        n = test.lookup_node(na);
        if (n != NULL && n->data)
            cout << na << " -> " << *n->key << "=>" << *n->data << endl;
        n = test.lookup_node(na, false);
        if (n != NULL && n->data)
            cout << "XACT: " << na << " -> " << *n->key << "=>" << *n->data << endl;
    }
    na.set_ip("192.168.64.1");
    for (int i = 0; i < 33; i++) {
        na.set_pref_len(i);
        RadixTrie<netaddress>::node *n;
        n = test.lookup_node(na);
        if (n != NULL && n->data)
            cout << na << " -> " << *n->key << "=>" << *n->data << endl;
        n = test.lookup_node(na, false);
        if (n != NULL && n->data)
            cout << "XACT: " << na << " -> " << *n->key << "=>" << *n->data << endl;
    }
    na.set_ip("192.2.5.3");
    for (int i = 0; i < 33; i++) {
        na.set_pref_len(i);
        RadixTrie<netaddress>::node *n;
        n = test.lookup_node(na);
        if (n != NULL && n->data)
            cout << na << " -> " << *n->key << "=>" << *n->data << endl;
        n = test.lookup_node(na, false);
        if (n != NULL && n->data)
            cout << "XACT: " << na << " -> " << *n->key << "=>" << *n->data << endl;
    }

    test.print();
    na.set_ip("192.168.64.1");
    na.set_pref_len(16);
    cout << test.remove(na) << endl;
    test.print();
    nap = test.lookup(na, true);
    if (nap)
        cout << *nap << endl;
    cout << test.remove_all(na) << endl;
    test.print();
    nap = test.lookup(na, true);
    if (nap)
        cout << *nap << endl;

    RadixTrie<int> foobar;
    int *ip;
    for (int i = 0; i < 1000000; i++) {
        struct in_addr foo;
        foo.s_addr = htonl(rand());
        na.set_ip(foo);
        na.set_pref_len((rand() % 32) + 33);
        ip = foobar.lookup(na);
        ip = new int(i);
        foobar.insert(na, *ip);
//        foobar.print();
//        cout << endl;
    }
    foobar.print();
    

    return 0;
}
