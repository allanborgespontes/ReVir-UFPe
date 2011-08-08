/// ----------------------------------------*- mode: C++; -*--
/// @file main.cpp
/// main file for natfwd program
/// ----------------------------------------------------------
/// $Id: main.cpp 6161 2011-05-18 20:02:40Z bless $
/// $HeadURL: https://svn.tm.kit.edu/nsis/natfw-nslp/trunk/src/main.cpp $
// ===========================================================
//                      
// Copyright (C) 2005-2007, all rights reserved by
// - Institute of Telematics, Universitaet Karlsruhe (TH)
//
// More information and contact:
// https://projekte.tm.uka.de/trac/NSIS
//                      
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; version 2 of the License
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// ===========================================================
#include <unistd.h>	// for getopt
#include <openssl/ssl.h>

#include "logfile.h"
#include "threads.h"

#include "gist_conf.h"
#include "protlibconf.h"
#include "configfile.h"

#include "natfw_config.h"
#include "natfw_daemon.h"


using namespace protlib;
using namespace protlib::log;
using namespace natfw;

namespace protlib {
	protlibconf plibconf;
}

namespace ntlp {
// configuration class
gistconf gconf;
}

natfw_config conf;

using namespace ntlp;

logfile commonlog("natfwd.log", natfw_config::USE_COLOURS);
logfile &protlib::log::DefaultLog(commonlog);


std::string config_filename;

void parse_commandline(int argc, char *argv[]) {
	std::string usage("usage: natfwd -c config_file\n");

	/*
	 * Parse command line options.
	 */
	while ( true ) {
		int c = getopt(argc, argv, "c:");

		if ( c == -1 )
			break;

		switch ( c ) {
			case 'c':
				conf.getparref<string>(natfwconf_conffilename) = optarg;
				break;
			default:
				std::cerr << usage;
				exit(1);
		}
	}

	if ( conf.getparref<string>(natfwconf_conffilename) == "" ) {
		std::cerr << usage;
		exit(1);
	}

	argv += optind;
	argc -= optind;

	/*
	 * Only non-option arguments are left. The first is in argv[0].
	 */
	if ( argc > 0 ) {
		std::cerr << usage;
		exit(1);
	}
}


int main(int argc, char *argv[]) {
	
	// create the global configuration parameter repository 
	conf.repository_init();

	// register all NATFW configuration parameters at the registry
	conf.setRepository();

	// register all GIST configuration parameters at the registry
	ntlp::gconf.setRepository();

	// register all protlib configuration parameters at the registry
	plibconf.setRepository();

	/*
	 * Initialize protlib, openssl, etc.
	 */
	parse_commandline(argc, argv);
	init_framework();



	// read all config values from config file
	configfile cfgfile(configpar_repository::instance());

	try {
		cfgfile.load(conf.getparref<string>(natfwconf_conffilename));
	}
	catch(configParException& cfgerr)
	{
		ERRLog("natfwd", "Error occurred while reading the configuration file: " << cfgerr.what());
		cerr << cfgerr.what() << endl << "Exiting." << endl;
		return 1;
	}

	/*
	 * Start the NATFW daemon thread. It will in turn start the other
	 * threads it requires.
	 */
	natfw_daemon_param param("natfwd", conf);
	ThreadStarter<natfw_daemon, natfw_daemon_param> natfwd_thread(
		conf.get_num_dispatcher_threads(), param);
	
	natfwd_thread.start_processing();

	// wait until all threads have been terminated
	natfwd_thread.wait_until_stopped();

	/*
	 * Free all resources.
	 */
	cleanup_framework();
}

// EOF
