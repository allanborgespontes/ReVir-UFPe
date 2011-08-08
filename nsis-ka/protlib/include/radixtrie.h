#ifndef _RADIX_TRIE_H
#define _RADIX_TRIE_H

#include "address.h"

namespace protlib {

template <typename _dT>
class RadixTrie {
public:
	typedef _dT	data_type;

	struct node {
		node(netaddress *k, data_type *d) : key(k), data(d) {
			left = right = this;
			index = 0;
		}
		~node() {
			if (data)
				delete data;
			if (key)
				delete key;
			if (left != 0 && left->index > index)
				delete left;
			if (right != 0 && right->index > index)
				delete right;
		}
		node			*left;
		node			*right;
		netaddress		*key;
		data_type		*data;
		int			 index;
	};

	RadixTrie() {
		netaddress *def;
		def = new netaddress("0.0.0.0", (prefix_length_t)0);
		v4head = new node(def, 0);
		def = new netaddress("::", (prefix_length_t)0);
		v6head = new node(def, 0);
	}

	~RadixTrie() {
		delete v4head;
		delete v6head;
	}

	node *insert(const netaddress &key, data_type &dat) {
		node *a, *b, *c, *n, *m;
		int cmp, pos = 0;

		c = a = key.is_ipv4() ? v4head : v6head;

		// search the tree as long as there are bits left in key
		while (key.get_pref_len() > a->index) {

			// compare key to key in node a from position a->index
			pos = a->index - 1;
			cmp = key.rdx_cmp(a->key, &pos);
			if (pos < 0)
				abort();

			// in case of a perfect match
			if ((cmp == 0) &&
			    (a->key->get_pref_len() == key.get_pref_len())) {
			    // replace data in node
				if (a->data)
					delete a->data;
				a->data = &dat;
				return a;
			}

			if (cmp == 0)
				break;

			// select node to continue the search based on the
			// first different bit between a and key
			b = cmp < 0 ? a->left : a->right;

			// we reached a dead end
			if (b->index <= a->index)
				break;

			// the first difference was before a's bitmask ended
			// we must not make any more progress
			if (pos <= a->key->get_pref_len())
				break;

			c = a;
			a = b;
		}

		// first check if a and key share a common prefix
		if ((key.get_pref_len() == a->key->get_pref_len()) ||
		   (pos > a->index && pos <= a->key->get_pref_len())) {
		   	int opos = pos;

		   	// make sure we didn't just miss the perfect match
			pos = a->index;
			cmp = key.rdx_cmp(a->key, &pos);
			if (cmp == 0 &&
			    (a->key->get_pref_len() == key.get_pref_len())) {
			    // replace data in node
				if (a->data)
					delete a->data;
				a->data = &dat;
				return a;
			}

		   	// create a node with that prefix
		   	pos = opos;
			n = new node(new netaddress(key), 0);
			n->key->set_pref_len(pos - 1);

			// hook it to the previous node(c)
			pos = c->index;
			cmp = n->key->rdx_cmp(c->key, &pos);
			n->index = pos;
			if (n->index <= c->index) {
				cout << "DEAD NODE INSERTION!!!" << endl;
				abort();
			}
			if (cmp < 0) {
				if (c->left != a) {
					cout << "TREE CORRUPTION!!!" << endl;
					abort();
				}
				c->left = n;
			} else {
				if (c->right != a) {
					cout << "TREE CORRUPTION!!!" << endl;
					abort();
				}
				c->right = n;
			}

			// hook the current node(a) to the common prefix
			// node(n)
			pos = n->index;
			cmp = a->key->rdx_cmp(n->key, &pos);
			a->index = pos;
			if (a->index <= n->index) {
				cout << "DEAD NODE INSERTION!!!" << endl;
				abort();
			}
			if (cmp < 0)
				n->left = a;
			else
				n->right = a;

			// create a new node(m) for the insert
			m = new node(new netaddress(key), &dat);			
			// hook node(m) to the common prefix node(n)
			pos = n->index;
			cmp = m->key->rdx_cmp(n->key, &pos);
			m->index = pos;
			if (cmp < 0) {
				if (n->left == a) {
					cout << "OVERWRITE!!!" << endl;
					abort();
				}
				n->left = m;
			} else {
				if (n->right == a) {
					cout << "OVERWRITE!!!" << endl;
					abort();
				}
				n->right = m;
			}

			return m;
		}

		// c is a prefix of key, key is a prefix of a
		if (a->index >= pos) {
			// create a new node for the key
			n = new node(new netaddress(key), &dat);
			// hook it to the previous node(c)
			n->index = pos;
			if (n->index <= c->index) {
				cout << "DEAD NODE INSERTION!!!" << endl;
				abort();
			}
			if (cmp < 0) {
				if (c->left != a) {
					cout << "TREE CORRUPTION!!!" << endl;
					abort();
				}
				c->left = n;
			} else {
				if (c->right != a) {
					cout << "TREE CORRUPTION!!!" << endl;
					abort();
				}
				c->right = n;
			}

			// hook the current node(a) to the newly created
			// node(n)
			pos = n->index;
			cmp = a->key->rdx_cmp(n->key, &pos);
			a->index = pos;
			if (a->index <= c->index) {
				cout << "DEAD NODE INSERTION!!!" << endl;
				abort();
			}
			if (cmp < 0)
				n->left = a;
			else
				n->right = a;

			return n;
		}

		// reached a deadend, simply add a new node
		n = new node(new netaddress(key), &dat);
		n->index = pos;
		if (n->index <= a->index) {
			cout << "DEAD NODE INSERTION!!!" << endl;
			abort();
		}
		if (b->index <= a->index) {
			if (cmp < 0)
				a->left = n;
			else
				a->right = n;
		} else {
			cout << "TREE CORRUPTION!!!" << endl;
			abort();
		}

		return n;
	}

	node *lookup_node(const netaddress &key, bool lpfm = true,
	    bool with_data = true) {
		node *a, *b, *c, *lpfn;
		int cmp, pos = 0;

		lpfn = 0;
		c = b = a = key.is_ipv4() ? v4head : v6head;
		if (lpfm) {
			if (!with_data)
				lpfn = a;
			else if (a->data)
				lpfn = a;
		}

		// search the tree as long as there are bits left in key
		while (key.get_pref_len() > a->index) {

			// compare key to key in node a from pos
			pos--;
			cmp = key.rdx_cmp(a->key, &pos);

			// all of key consumed
			if (cmp == 0) {
				// key is less specific than a
				if (key.get_pref_len() <
				    a->key->get_pref_len())
					return lpfm ? lpfn : NULL;
				
				// key is an exact match for a
				if (key.get_pref_len() >=
				    a->key->get_pref_len()) {
					if (!with_data)
						return a;
					if (a->data)
						return a;
					return lpfm ? lpfn : NULL;
				}
			}

			// all of a consumed -> a is a prefix of key
			if (pos > a->key->get_pref_len()) {
				if (!with_data)
					lpfn = a;
				else if (a->data)
					lpfn = a;
			}

			// select node to continue the search based on the
			// first different bit between a and key
			b = cmp < 0 ? a->left : a->right;

			// we reached a dead end
			if (b->index <= a->index)
				break;

			c = a;
			a = b;
		}

		return lpfm ? lpfn : NULL;
	}

	data_type *lookup(const netaddress &key, bool lpfm = true) {
		node *n = lookup_node(key, lpfm);

		return n ? n->data : NULL;
	}

	bool remove(const netaddress &key) {
		node *n = lookup_node(key);

		if (n && n->data) {
			delete n->data;
			n->data = NULL;
		}

		return (n != 0);
	}

	bool remove_all(netaddress &key) {
		node *n = lookup_node(key, false, false);

		if (n == 0)
			return false;

		if (n->data) {
			delete n->data;
			n->data = NULL;
		}

		if (n->left->index > n->index) {
			delete n->left;
			n->left = n;
		}
		if (n->right->index > n->index) {
			delete n->right;
			n->right = n;
		}

		return true;
	}

	void print() {
		cout << "v4_TREE: " << endl;
		print_node(v4head);
		cout << "v6_TREE: " << endl;
		print_node(v6head);
	}

	void print_node(node *x, bool decent = true) {
		if (x && x->key) {
			cout << "node: " << x << " key: " <<  *x->key;
			if (x->data != 0)
				cout << " data: " << x->data;
			else
				cout << " data: NULL";
			cout << " index: " << x->index << endl;
			cout << "\tleft: " << x->left << " right: " << x->right << endl;
			if (decent) {
				if (x->left->index > x->index)
					print_node(x->left);
				if (x->right->index > x->index)
					print_node(x->right);
			}
		}
	}

private:
	struct node *v4head;
	struct node *v6head;
};


} // end namespace
#endif

