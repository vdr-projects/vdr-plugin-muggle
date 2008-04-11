/*!
 * \file vdr_setup.h
 * \brief A setup class for a VDR media plugin (muggle)
 *
 * \version $Revision: 1.2 $
 * \date    $Date$
 * \author  Ralf Klueber, Lars von Wedel, Andreas Kellner
 * \author  Responsible author: $Author$
 *
 * $Id$
 *
 * Adapted from
 * MP3/MPlayer plugin to VDR (C++)
 * (C) 2001,2002 Stefan Huelswitt <huels@iname.com>
 */

#ifndef ___VDR_SETUP_MG_H
#define ___VDR_SETUP_MG_H

#include <string>

#include "mg_setup.h"

#include <vdr/menuitems.h>

#define MAX_PATH 256

/*!
 * \brief allow user to modify setup on OSD
 */
class mgMenuSetup : public cMenuSetupPage
{
	protected:
		virtual void Store ();
	public:
		mgMenuSetup ();
};
#endif
