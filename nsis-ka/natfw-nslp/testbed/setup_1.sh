#! /bin/sh
#
# Network setup for the testbed nodes.
#
# Required switch configuration:
#     set vlan 3801 3/38,3/40
#     set vlan 3802 4/39,4/41
#
# usage: setup_1.sh hostname
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
	;;
    tb20)
	ifconfig eth1 10.38.01.201/24 broadcast 10.38.01.255 up
	ifconfig eth2 10.38.02.202/24 broadcast 10.38.02.255 up
	echo 1 > /proc/sys/net/ipv4/ip_forward
	;;
    tb21)
	ifconfig eth2 10.38.02.212/24 broadcast 10.38.02.255 up
	route add -net 10.38.01.0/24 gw 10.38.02.202
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
