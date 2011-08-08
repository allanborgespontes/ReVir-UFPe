#! /bin/sh
#
# Network setup for the testbed nodes.
#
# Required switch configuration:
#     set vlan 3801 3/38,3/40
#     set vlan 3802 4/39,4/41
#     set vlan 3803 4/42,4/44
#
# usage: setup_2.sh hostname
#

if [ $# -ne 1 ]; then
	echo "usage: $0 hostname" >&2
	exit 1
fi

# clear existing configuration
. setup_clear.sh

HOSTNAME=$1

echo "setting network configuration for host $1 ..."

case $HOSTNAME in
    tb19)
	ifconfig eth1 10.38.01.191/24 broadcast 10.38.01.255 up
	route add -net 10.38.02.0/24 gw 10.38.01.201
	route add -net 10.38.03.0/24 gw 10.38.01.201
	;;
    tb20)
	ifconfig eth1 10.38.01.201/24 broadcast 10.38.01.255 up
	ifconfig eth2 10.38.02.202/24 broadcast 10.38.02.255 up
	route add -net 10.38.03.0/24 gw 10.38.02.212
	echo 1 > /proc/sys/net/ipv4/ip_forward
	;;
    tb21)
	ifconfig eth2 10.38.02.212/24 broadcast 10.38.02.255 up
	ifconfig eth3 10.38.03.213/24 broadcast 10.38.03.255 up
	route add -net 10.38.01.0/24 gw 10.38.02.202
	echo 1 > /proc/sys/net/ipv4/ip_forward
	;;
    tb22)
	ifconfig eth3 10.38.03.223/24 broadcast 10.38.03.255 up
	route add -net 10.38.01.0/24 gw 10.38.03.213
	route add -net 10.38.02.0/24 gw 10.38.03.213
	;;
    *)
	echo "error: this host is not part of the test setup." >&2
	;;
esac


echo "loading kernel modules ..."
modprobe iptable_filter
modprobe ip_queue
modprobe ip6table_filter
modprobe ip6_queue


echo "done."

# EOF
