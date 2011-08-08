#! /bin/sh
#
# Clears the network setup for a testbed node.
#
# This script has to be run with root priviledges on each testbed node before
# a new test setup is loaded.
#

# Stop DHCP deamons for each device. SuSE-specific.
#
echo "shutting down DHCP daemons ..."
ifdown-dhcp eth1
ifdown-dhcp eth2
ifdown-dhcp eth3

# Shut down the network interfaces. This also deletes the automatic routes
# entered by the kernel for each locally attached network.
#
echo "bringing down network interfaces ..."
ifconfig eth1 down
ifconfig eth2 down
ifconfig eth3 down

# Delete all manually set routes. Not all routes may be set on all hosts,
# so error messages will be printed.
#
echo "deleting routes, this may print (mostly harmless) error messages ..."
route del -net 10.38.01.0/24
route del -net 10.38.02.0/24
route del -net 10.38.03.0/24


# Stop IP forwarding, this node will no longer act as a router.
#
echo "deactivating IPv4 forwarding ..."
echo 0 > /proc/sys/net/ipv4/ip_forward

# EOF
