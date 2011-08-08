#include "protlibconf.h"

using namespace protlib;

void 
protlibconf::repository_init()
{
	DLog("protlibconf", "start - creating configuration parameter singleton");
	configpar_repository::create_instance(protlib_realm+1);
}


void protlibconf::registerAllPars()
{
  DLog("protlibconf::registerAllPars", "starting registering protlib parameters.");

  registerPar( new configpar<bool>(protlib_realm, protlibconf_ipv4_only, "ipv4-only", "sets up IPv4 only sockets", false, false) );

  DLog("protlibconf::registerAllPars", "finished registering protlib parameters.");
}

/** sets the repository pointer and registers all parameters 
 *  (not done in constructor, because of allowing simulation hosts to initialize later)
 **/ 
void
protlibconf::setRepository(configpar_repository* newcfp_rep)
{
	if (newcfp_rep == 0)
		throw  configParExceptionNoRepository();
	
	cfp_rep= newcfp_rep;
	// register the realm
	cfp_rep->registerRealm(protlib_realm, "protlib", protlibconf_maxparno);
	DLog("protlibconf", "registered protlib realm with realm id " << (int) protlib_realm);
	
	// now register all parameters
	registerAllPars();
}


string
protlibconf::to_string()
{
  ostringstream os;

  unsigned int max_pars= configpar_repository::instance()->getRealmSize(protlib_realm);
  configparBase* cfgpar= NULL;
  for (unsigned int i= 0; i < max_pars; i++) {
	  try {
		  // get config par with id i
		  cfgpar= configpar_repository::instance()->getConfigPar(protlib_realm, i);
		  os << cfgpar->getName() << (( cfgpar->getName().length() < 8 ) ? "\t\t: " : "\t: ");
		  cfgpar->writeVal(os);
		  if (cfgpar->getUnitInfo())
			  os << " " << cfgpar->getUnitInfo();
		  os << endl;
	  }
	  catch (configParExceptionParNotRegistered) {
		  cfgpar= NULL;
		  continue;
	  } // end catch
	  catch (...) {
		  throw;
	  }
  } // end for
	    
  return os.str();
}


