//
//  cards
// 
//  Copyright (c) 2000-2005 Per Liden
//  Copyright (c) 2006-2013 by CRUX team (http://crux.nu)
//  Copyright (c) 2013-2016 by NuTyX team (http://nutyx.org)
// 
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
//  USA.
//

#include "pkgrm.h"
#include <unistd.h>
#include <stdio.h>

using namespace std;

Pkgrm::Pkgrm ()
	: Pkgdbh("pkgrm")
{
}
void Pkgrm::run(int argc, char** argv)
{
	parseArguments(argc, argv);
	run();
	finish();
}
void Pkgrm::run()
{
	// Check UID
	if (getuid())
	{
		m_actualError = ONLY_ROOT_CAN_INSTALL_UPGRADE_REMOVE;
		treatErrors("");
	}

	Db_lock lock(m_root, true);

	// Get the list of installed packages
	getListOfPackageNames(m_root);

	// Retrieve info about all the packages
	buildDatabaseWithDetailInfos(false);

	set<string> listOfPackagesToRemove;
	for (auto i : m_listOfInstPackages) {
		if ( i.second.collection == m_packageName) {
			listOfPackagesToRemove.insert(i.first);
		}
	}
	if ( listOfPackagesToRemove.empty() ) {
		if (!checkPackageNameExist(m_packageName))
		{
			m_actualError = PACKAGE_NOT_INSTALL;
			treatErrors(m_packageName);
		}

		// Remove metadata about the package removed
		removePackageFilesRefsFromDB(m_packageName);

		// Remove the files on hd
		removePackageFiles(m_packageName);
	} else {
		for (auto i : listOfPackagesToRemove) {
			// Remove metadata about the package removed
			removePackageFilesRefsFromDB(i);

			// Remove the files on hd
			removePackageFiles(i);
		}
	}
}
void Pkgrm::printHelp() const
{
	cout << USAGE << m_utilName << " [options] <package>" << endl
	     << OPTIONS << endl
	     << "  -r, --root <path>   specify alternative installation root" << endl
	     << "  -v, --version       print version and exit" << endl
	     << "  -h, --help          print help and exit" << endl;
}
// vim:set ts=2 :
