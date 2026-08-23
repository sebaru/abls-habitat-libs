#!/bin/sh

SOCLE=`grep "^ID=" /etc/os-release | cut -f 2 -d '='`

if [ "$(whoami)" != "root" ]
 then
	 echo "Only user root can run this script (or sudo)."
	 exit 1
fi

groupadd abls

if [ "$SOCLE" = "fedora" ]
 then
	echo "Installing RPM-based dependencies"
	dnf install -y gcc glib2-devel json-glib-devel mosquitto-devel cmake rpm-build git pkg-config
fi

if [ "$SOCLE" = "debian" ] || [ "$SOCLE" = "raspbian" ] || [ "$SOCLE" = "ubuntu" ]
 then
	echo "Installing Debian-based dependencies"
	apt update -y
	apt install -y git cmake gcc pkg-config fakeroot dpkg-dev debhelper lintian
	apt install -y libglib2.0-dev libjson-glib-dev libmosquitto-dev
fi

if [ "$SOCLE" != "fedora" ] && [ "$SOCLE" != "debian" ] && [ "$SOCLE" != "raspbian" ] && [ "$SOCLE" != "ubuntu" ]
 then
	echo "Unsupported distribution: $SOCLE"
	exit 2
fi
