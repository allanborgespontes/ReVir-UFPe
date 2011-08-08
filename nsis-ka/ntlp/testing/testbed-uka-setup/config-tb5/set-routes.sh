#!/bin/bash
ip r add 10.2.4.0/24 via 10.4.5.4
ip r add 10.2.3.0/24 via 10.3.5.3
ip r add 10.1.2.0/24 via 10.3.5.3
echo 1 >/proc/sys/net/ipv4/conf/eth1/forwarding
echo 1 >/proc/sys/net/ipv4/conf/eth2/forwarding
echo 1 >/proc/sys/net/ipv4/conf/eth3/forwarding
echo 0 >/proc/sys/net/ipv4/conf/eth1/rp_filter
echo 0 >/proc/sys/net/ipv4/conf/eth2/rp_filter
echo 0 >/proc/sys/net/ipv4/conf/eth3/rp_filter
