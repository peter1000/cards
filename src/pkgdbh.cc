//  pkgdbh.cc
//
//  Copyright (c) 2000-2005 Per Liden
//  Copyright (c) 2006-2013 by CRUX team (http://crux.nu)
//  Copyright (c) 2013-2014 by NuTyX team (http://nutyx.org)
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
#include "string_utils.h"
#include "file_utils.h"
#include "pkgdbh.h"
#include "process.h"

#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <csignal>
#include <ext/stdio_filebuf.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>

using __gnu_cxx::stdio_filebuf;

Pkgdbh::Pkgdbh(const string& name)
	: m_utilName(name)
{
	// Ignore signals
/*	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigaction(SIGHUP, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGQUIT, &sa, 0);
	sigaction(SIGTERM, &sa, 0); */
}
void Pkgdbh::treatErrors(const string& s) const
{
	switch ( m_actualError )
	{
		case CANNOT_DOWNLOAD_FILE:
			throw runtime_error("could not download " + s);
			break;
		case CANNOT_CREATE_FILE:
			throw RunTimeErrorWithErrno("could not created  " + s);
			break;
		case CANNOT_OPEN_FILE:
			throw RunTimeErrorWithErrno("could not open " + s);
			break;
		case CANNOT_FIND_FILE:
			throw RunTimeErrorWithErrno("could not find " + s);
			break;
		case CANNOT_READ_FILE:
			throw runtime_error("could not read " + s);
			break;
		case CANNOT_READ_DIRECTORY:
			throw RunTimeErrorWithErrno("could not read directory " + s);
			break;
		case CANNOT_WRITE_FILE:
			throw RunTimeErrorWithErrno("could not write file " + s);
			break;
		case CANNOT_SYNCHRONIZE:
			throw RunTimeErrorWithErrno("could not synchronize " + s);
			break;
		case CANNOT_RENAME_FILE:
			throw RunTimeErrorWithErrno("could not rename " + s);
			break;
		case CANNOT_DETERMINE_NAME_BUILDNR:
			throw RunTimeErrorWithErrno("could not determine name / build number " + s);
			break;
		case EMPTY_PACKAGE:
			throw RunTimeErrorWithErrno("could not synchronize " + s);
			break;
		case CANNOT_FORK:
			throw RunTimeErrorWithErrno("fork() failed " + s);
			break;
		case WAIT_PID_FAILED:
			throw RunTimeErrorWithErrno("waitpid() failed " + s);
			break;
		case DATABASE_LOCKED:
			throw RunTimeErrorWithErrno("Database  " + s + " locked by another user");
			break;
		case CANNOT_LOCK_DIRECTORY:
			throw RunTimeErrorWithErrno("could lock directory " + s);
			break;
		case CANNOT_REMOVE_FILE:
			throw RunTimeErrorWithErrno("could not remove file " + s);
			break;
		case CANNOT_RENAME_DIRECTORY:
			throw RunTimeErrorWithErrno("could not rename directory  " + s);
			break;
		case OPTION_ONE_ARGUMENT:
			throw RunTimeErrorWithErrno(s + " require one argument");
			break;
		case INVALID_OPTION:
			throw runtime_error(s + " invalid option" );
			break;
		case OPTION_MISSING:
			throw runtime_error(s + " option missing");
			break;
		case TOO_MANY_OPTIONS:
			throw runtime_error(s+ ": to many options");
			break;
		case ONLY_ROOT_CAN_INSTALL_UPGRADE_REMOVE:
			throw runtime_error(s + " only root can install / upgrade / remove packages");
			break;
		case PACKAGE_ALLREADY_INSTALL:
			throw runtime_error("package " + s + " allready installed (use -u to upgrade)");
			break;
		case PACKAGE_NOT_INSTALL:
			throw runtime_error("package " + s + " not yet installed");
			break;
		case PACKAGE_NOT_PREVIOUSLY_INSTALL:
			throw runtime_error("package " + s + " not previously installed (skip -u to install)");
			break;
		case LISTED_FILES_ALLREADY_INSTALLED:
			throw runtime_error(s + "listed file(s) allready installed (use -f to ignore and overwrite)");
			break;
		case PKGADD_CONFIG_LINE_TOO_LONG:
			throw RunTimeErrorWithErrno(s + ": line too long, aborting");
			break;
		case PKGADD_CONFIG_WRONG_NUMBER_ARGUMENTS:
			throw RunTimeErrorWithErrno(s + ": wrong number of arguments, aborting");
			break;
		case PKGADD_CONFIG_UNKNOWN_ACTION:
			throw RunTimeErrorWithErrno(s + "': config unknown action, should be YES or NO, aborting");
			break;
		case PKGADD_CONFIG_UNKNOWN_EVENT:
			throw RunTimeErrorWithErrno(s + "' unknown event, aborting");
			break;
		case CANNOT_COMPILE_REGULAR_EXPRESSION:
			throw runtime_error("error compiling regular expression '" + s + "', aborting");
			break;
		case NOT_INSTALL_PACKAGE_NEITHER_PACKAGE_FILE:
			throw runtime_error(s + " is neither an installed package nor a package file");
			break;
	}
}
void Pkgdbh::progressInfo() const
{
  static int j = 0;
  int i;
  switch ( m_actualAction )
  {
		case PKG_DOWNLOAD_START:
			break;
		case PKG_DOWNLOAD_RUN:
			break;
		case PKG_DOWNLOAD_END:
			break;
		case PKG_MOVE_META_START:
			break;
		case PKG_MOVE_META_END:
			break;
    case DB_OPEN_START:
      cout << "Retrieve info about the " << m_packagesList.size() << " packages: ";
      break;
    case DB_OPEN_RUN:
      if ( m_packagesList.size()>100)
      {
        i = j / ( m_packagesList.size() / 100);
        printf("%3d%%\b\b\b\b",i);
      }
      j++;
      break;
    case DB_OPEN_END:
      printf("100 %%\n");
      break;
		case PKG_PREINSTALL_START:
			cout << "pre-install: start" << endl;
			break;
		case PKG_PREINSTALL_END:
			cout << "pre-install: finish" << endl;
			break;
    case PKG_INSTALL_START:
      j = 0;
      cout << "Installing "<< m_filesNumber << " files: ";
      break;
    case PKG_INSTALL_RUN:
      if ( m_filesNumber > 100)
      {
        i = m_installedFilesNumber / ( m_filesNumber  / 100 );
        printf("%3u%%\b\b\b\b",i);
      }
      j++;
      break;
    case PKG_INSTALL_END:
      printf("100 %%\n");
      break;
		case PKG_POSTINSTALL_START:
			cout << "post-install: start" << endl;
			break;
		case PKG_POSTINSTALL_END:
			cout << "post-install: finish" << endl;
			break;
		case DB_ADD_PKG_START:
			break;
		case DB_ADD_PKG_END:
			break;
		case RM_PKG_FILES_START:
			j=0;
			cout << "Removing " << m_filesList.size() << " files: ";
			break;
		case RM_PKG_FILES_RUN:
			if ( m_filesList.size()>100)
			{
				i = j / ( m_filesList.size() / 100);
				printf("%3d%%\b\b\b\b",i);
			}
			j++;
			break;
		case RM_PKG_FILES_END:
			printf("100 %%\n");
			break;
		case LDCONFIG_START:
			break;
		case LDCONFIG_END:
			break;
  }
}
/* Append to the "DB" the number of packages founds (directory containg a file named files */
int Pkgdbh::getListOfPackages (const string& path)
{
	keyValue string_splited;
	m_root = trimFileName(path + "/");
	const string pathdb =  m_root + PKG_DB_DIR;
	if ( findFile(m_packagesList, pathdb) != 0 ) {
		m_actualError = CANNOT_READ_FILE;
    treatErrors(pathdb);
	}
#ifndef NDEBUG
  cerr << "Number of Packages: " << m_packagesList.size() << endl;
#endif
  return m_packagesList.size();
}
/* get details infos of a package */
pair<string, pkginfo_t> Pkgdbh::getInfosPackage(const string& packageName)
{
	pair<string, pkginfo_t> result;
	
	result.first = packageName;
	return result;
}
/* Populate the database with all details infos */
void Pkgdbh::getInstalledPackages(bool silent)
{
	if (!silent) {
		m_actualAction = DB_OPEN_START;
		progressInfo();
	}
	for (set<string>::iterator i = m_packagesList.begin();i != m_packagesList.end();++i) {
		if (!silent) {
			m_actualAction = DB_OPEN_RUN;
			if ( m_packagesList.size() > 100 )
				progressInfo();
		}
		pkginfo_t info;
		const string metaFile = m_root + PKG_DB_DIR + *i + '/' + PKG_META;
		itemList * contentFile = initItemList();
		readFile(contentFile,metaFile.c_str());
		for (unsigned int li=0; li< contentFile->count ; ++li) {
			if ( contentFile->items[li][0] == 'D' ) {
				string description = contentFile->items[li];
				info.description = description.substr(1);
			}
			if ( contentFile->items[li][0] == 'B' ) {
				string build = contentFile->items[li];
				info.build = build.substr(1);
			} 
			if ( contentFile->items[li][0] == 'V' ) {
				string version = contentFile->items[li];
				info.version = version.substr(1);
			}
			if ( contentFile->items[li][0] == 'a' ) {
				string arch = contentFile->items[li];
				info.arch = arch.substr(1);
			}
			if ( contentFile->items[li][0] == 'S' ) {
				string size = contentFile->items[li];
				info.size = size.substr(1);
			}
			if ( contentFile->items[li][0] == 'R' ) {
				string run = contentFile->items[li];
				info.run = run.substr(1) + ' ' + info.run ;
			}
		}
		freeItemList(contentFile);	
		// list of files
		const string filelist = m_root + PKG_DB_DIR + *i + PKG_FILES;
		int fd = open(filelist.c_str(), O_RDONLY);
		if (fd == -1) {
			m_actualError = CANNOT_OPEN_FILE;
			treatErrors(filelist);
		}
		stdio_filebuf<char> listbuf(fd, ios::in, getpagesize());
		istream in(&listbuf);
		if (!in)
			throw RunTimeErrorWithErrno("could not read " + filelist);

		while (!in.eof()){
			// read alls the files for alls the packages founds
			for (;;) {
				string file;
				getline(in, file);
				if (file.empty())
					break; // End of record
				info.files.insert(info.files.end(), file);
			}
			if (!info.files.empty())
				m_listOfInstPackages[*i] = info;
		}
	}
#ifndef NDEBUG
  cerr << endl;
  cerr << m_listOfInstPackages.size() << " packages found in database " << endl;
#endif
	if (!silent)
	{
		m_actualAction = DB_OPEN_END;
		progressInfo();
	}
}
void Pkgdbh::moveMetaFilesPackage(const string& name, pkginfo_t& info)
{
  set<string> metaFilesList;
	m_actualAction = PKG_MOVE_META_START;
	progressInfo();
	string package_meta_dir = ".";
	for (set<string>::const_iterator i = info.files.begin(); i != info.files.end(); ++i)
	{
		if ( strncmp(i->c_str(),package_meta_dir.c_str(),package_meta_dir.size()) == 0 )
		{
#ifndef NDEBUG
			cout << "*I: " << *i << endl;
#endif
			metaFilesList.insert(metaFilesList.end(), *i );
			info.files.erase(i);
		}
	}
	metaFilesList.insert(".META");
	metaFilesList.insert(".MTREE");
	set<string> fileContent;
	if ( parseFile(fileContent,".META") == -1 ) {
		m_actualError = CANNOT_FIND_FILE;
		treatErrors(".META");
	}
	for (set<string>::iterator i = fileContent.begin();i != fileContent.end();++i) {
		string s = *i;
		string::size_type pos = s.find('=');
		if (pos != string::npos) {
			s = stripWhiteSpace(s.substr(0,pos));
		}
		if ( s == "build" ) {
			cout << s << endl;
			m_build = stripWhiteSpace(s.substr(pos+2));
			break;
		}
	}
	const string packagedir = m_root + PKG_DB_DIR ;
	const string packagenamedir = m_root + PKG_DB_DIR + name ;

	mkdir(packagenamedir.c_str(),0755);
	for (set<string>::const_iterator i = metaFilesList.begin(); i!=  metaFilesList.end(); ++i)
	{
		char * destFile = const_cast<char*>(i->c_str());
		destFile++;
		string file = packagenamedir + "/" + destFile;
		if (rename(i->c_str(), file.c_str()) == -1)
		{
			m_actualError = CANNOT_RENAME_FILE;
			treatErrors( *i + " to " + file);
		}
	}
	m_actualAction = PKG_MOVE_META_END;
	progressInfo();
}
void Pkgdbh::addPackageFilesRefsToDB(const string& name, const pkginfo_t& info)
{

	m_listOfInstPackages[name] = info;
	const string packagedir = m_root + PKG_DB_DIR ;
	const string packagenamedir = m_root + PKG_DB_DIR + name ;
	mkdir(packagenamedir.c_str(),0755);
	const string fileslist = packagenamedir + PKG_FILES;
	const string fileslist_new = fileslist + ".imcomplete_transaction";
	int fd_new = creat(fileslist_new.c_str(),0644);
	if (fd_new == -1)
	{
		m_actualError = CANNOT_CREATE_FILE;
		treatErrors(fileslist_new);
	}

	stdio_filebuf<char> filebuf_new(fd_new, ios::out, getpagesize());

	ostream db_new(&filebuf_new);
	copy(info.files.begin(), info.files.end(), ostream_iterator<string>(db_new, "\n"));

	db_new.flush();
	if (!db_new)
	{
		rmdir(packagenamedir.c_str());
		m_actualError = CANNOT_WRITE_FILE;
		treatErrors(fileslist_new);
	}
	// Synchronize file to disk
	if (fsync(fd_new) == -1)
	{
		rmdir(packagenamedir.c_str());
		m_actualError = CANNOT_SYNCHRONIZE;
		treatErrors(fileslist_new);
	}
	// Move new database into place
	if (rename(fileslist_new.c_str(), fileslist.c_str()) == -1)
	{
		rmdir(packagenamedir.c_str());
		m_actualError = CANNOT_RENAME_FILE;
		treatErrors(fileslist_new + " to " + fileslist);
	}
}

bool Pkgdbh::checkPackageNameExist(const string& name)
{
	return (m_listOfInstPackages.find(name) != m_listOfInstPackages.end());
}

/* Remove meta data about the removed package */
void Pkgdbh::removePackageFilesRefsFromDB(const string& name)
{
	set<string> metaFilesList;
	const string packagedir = m_root + PKG_DB_DIR ;
	const string build = m_listOfInstPackages[name].build;
	const string arch = m_listOfInstPackages[name].arch;
	const string packagenamedir = m_root + PKG_DB_DIR + name;

	if ( findFile(metaFilesList, packagenamedir) != 0 ) {
		m_actualError = CANNOT_READ_FILE;
		treatErrors(packagenamedir);
	}
	if (metaFilesList.size() > 0) {
		for (set<string>::iterator i = metaFilesList.begin(); i != metaFilesList.end();++i) {
			const string filename = packagenamedir + "/" + *i;
			if (checkFileExist(filename) && remove(filename.c_str()) == -1) {
				const char* msg = strerror(errno);
					cerr << m_utilName << ": could not remove " << filename << ": " << msg << endl;
				}
#ifndef NDEBUG
				cout  << "File: " << filename << " is removed"<< endl;
#endif
			}
	}
	if( remove(packagenamedir.c_str()) == -1) {
		const char* msg = strerror(errno);
		cerr << m_utilName << ": could not remove " << packagenamedir << ": " << msg << endl;
	}
#ifndef NDEBUG
	cout  << "Directory: " << packagenamedir << " is removed"<< endl;
#endif
}

/* Remove the physical files after followings some rules */
void Pkgdbh::removePackageFiles(const string& name)
{
	m_filesList = m_listOfInstPackages[name].files;
	m_listOfInstPackages.erase(name);

#ifndef NDEBUG
	cerr << "Removing package phase 1 (all files in package):" << endl;
	copy(m_filesList.begin(), m_filesList.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Don't delete files that still have references
	for (packages_t::const_iterator i = m_listOfInstPackages.begin(); i != m_listOfInstPackages.end(); ++i)
		for (set<string>::const_iterator j = i->second.files.begin(); j != i->second.files.end(); ++j)
			m_filesList.erase(*j);

#ifndef NDEBUG
	cerr << "Removing package phase 2 (files that still have references excluded):" << endl;
	copy(m_filesList.begin(), m_filesList.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	m_actualAction = RM_PKG_FILES_START;
	progressInfo();
	// Delete the files from bottom to up to make shure we delete the contents of any folder before
	for (set<string>::const_reverse_iterator i = m_filesList.rbegin(); i != m_filesList.rend(); ++i) {
		m_actualAction = RM_PKG_FILES_RUN;
		progressInfo();
		const string filename = m_root + *i;
		if (checkFileExist(filename) && remove(filename.c_str()) == -1) {
			const char* msg = strerror(errno);
			cerr << m_utilName << ": could not remove " << filename << ": " << msg << endl;
		}
	}
	m_actualAction = RM_PKG_FILES_END;
	progressInfo();
}

void Pkgdbh::removePackageFiles(const string& name, const set<string>& keep_list)
{
	m_filesList = m_listOfInstPackages[name].files;
	m_listOfInstPackages.erase(name);

#ifndef NDEBUG
	cerr << "Removing package phase 1 (all files in package):" << endl;
	copy(m_filesList.begin(), m_filesList.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Don't delete files found in the keep list
	for (set<string>::const_iterator i = keep_list.begin(); i != keep_list.end(); ++i)
		m_filesList.erase(*i);

#ifndef NDEBUG
	cerr << "Removing package phase 2 (files that is in the keep list excluded):" << endl;
	copy(m_filesList.begin(), m_filesList.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Don't delete files that still have references
	for (packages_t::const_iterator i = m_listOfInstPackages.begin(); i != m_listOfInstPackages.end(); ++i)
		for (set<string>::const_iterator j = i->second.files.begin(); j != i->second.files.end(); ++j)
			m_filesList.erase(*j);

#ifndef NDEBUG
	cerr << "Removing package phase 3 (files that still have references excluded):" << endl;
	copy(m_filesList.begin(), m_filesList.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Delete the files
	m_actualAction = RM_PKG_FILES_START;
	progressInfo();
	for (set<string>::const_reverse_iterator i = m_filesList.rbegin(); i != m_filesList.rend(); ++i) {
		m_actualAction = RM_PKG_FILES_RUN;
		progressInfo();
		const string filename = m_root + *i;
		if (checkFileExist(filename) && remove(filename.c_str()) == -1) {
			if (errno == ENOTEMPTY)
				continue;
			const char* msg = strerror(errno);
			cerr << m_utilName << ": could not remove " << filename << ": " << msg << endl;
		}
	}
	m_actualAction = RM_PKG_FILES_END;
	progressInfo();
}

void Pkgdbh::removePackageFilesRefsFromDB(set<string> files, const set<string>& keep_list)
{
	// Remove all references
	for (packages_t::iterator i = m_listOfInstPackages.begin(); i != m_listOfInstPackages.end(); ++i)
		for (set<string>::const_iterator j = files.begin(); j != files.end(); ++j)
			i->second.files.erase(*j);

#ifndef NDEBUG
	cerr << "Removing files:" << endl;
	copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Don't delete files found in the keep list
	for (set<string>::const_iterator i = keep_list.begin(); i != keep_list.end(); ++i)
		files.erase(*i);

	// Delete the files
	for (set<string>::const_reverse_iterator i = files.rbegin(); i != files.rend(); ++i) {
		const string filename = m_root + *i;
		if (checkFileExist(filename) && remove(filename.c_str()) == -1) {
			if (errno == ENOTEMPTY)
				continue;
			const char* msg = strerror(errno);
			cerr << m_utilName << ": could not remove " << filename << ": " << msg << endl;
		}
	}
}

set<string> Pkgdbh::getConflictsFilesList(const string& name, const pkginfo_t& info)
{
	set<string> files;

	// Find conflicting files in database
	for (packages_t::const_iterator i = m_listOfInstPackages.begin(); i != m_listOfInstPackages.end(); ++i) {
		if (i->first != name) {
			set_intersection(info.files.begin(), info.files.end(),
					 i->second.files.begin(), i->second.files.end(),
					 inserter(files, files.end()));
		}
	}

#ifndef NDEBUG
	cerr << "Conflicts phase 1 (conflicts in database):" << endl;
	copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Find conflicting files in filesystem
	for (set<string>::iterator i = info.files.begin(); i != info.files.end(); ++i) {
		const string filename = m_root + *i;
		if (checkFileExist(filename) && files.find(*i) == files.end())
			files.insert(files.end(), *i);
	}

#ifndef NDEBUG
	cerr << "Conflicts phase 2 (conflicts in filesystem added):" << endl;
	copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// Exclude directories
	set<string> tmp = files;
	for (set<string>::const_iterator i = tmp.begin(); i != tmp.end(); ++i) {
		if ((*i)[i->length() - 1] == '/')
			files.erase(*i);
	}

#ifndef NDEBUG
	cerr << "Conflicts phase 3 (directories excluded):" << endl;
	copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n"));
	cerr << endl;
#endif

	// If this is an upgrade, remove files already owned by this package
	if (m_listOfInstPackages.find(name) != m_listOfInstPackages.end()) {
		for (set<string>::const_iterator i = m_listOfInstPackages[name].files.begin(); i != m_listOfInstPackages[name].files.end(); ++i)
			files.erase(*i);

#ifndef NDEBUG
		cerr << "Conflicts phase 4 (files already owned by this package excluded):" << endl;
		copy(files.begin(), files.end(), ostream_iterator<string>(cerr, "\n"));
		cerr << endl;
#endif
	}

	return files;
}
pair<string, pkginfo_t> Pkgdbh::openArchivePackage(const string& filename)
{
	pair<string, pkginfo_t> result;
	ArchiveUtils packageArchive(filename.c_str());
#ifndef NDEBUG
	cerr << "Number of files: " << packageArchive.size() << endl;
#endif
	string basename(filename, filename.rfind('/') + 1);
	m_filesNumber = packageArchive.size();
	if (m_filesNumber == 0 ) {
		m_actualError = EMPTY_PACKAGE;
		treatErrors(basename);
	}
	string name = packageArchive.name();
	if (name.empty() ) {
		m_actualError = CANNOT_DETERMINE_NAME_BUILDNR;
		treatErrors(basename);
	}
	result.first = name;
#ifndef NDEBUG
	cerr << "name: " << name << endl;
#endif
	set<string> fileList =  packageArchive.setofFiles();
	for (set<string>::iterator i = fileList.begin();i != fileList.end();++i) {
		result.second.files.insert(*i);
	}
	return result;	
}
void Pkgdbh::extractAndRunPREfromPackage(const string& filename)
{
	char buf[PATH_MAX];
	struct archive* archive;
	struct archive_entry* entry;
	archive = archive_read_new();
	INIT_ARCHIVE(archive);
	if (archive_read_open_filename(archive,
		filename.c_str(),
		DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK)
	{
		m_actualError = CANNOT_OPEN_FILE;
		treatErrors(filename);
	}

	getcwd(buf, sizeof(buf));
	chdir(m_root.c_str());

	for (m_installedFilesNumber = 0; archive_read_next_header(archive, &entry) ==
		ARCHIVE_OK; ++m_installedFilesNumber)
	{
		string archive_filename = archive_entry_pathname(entry);
		if ( strcmp(archive_filename.c_str(),PKG_PRE_INSTALL) == 0)
		{
			unsigned int flags = ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_UNLINK;
			if (archive_read_extract(archive, entry, flags) != ARCHIVE_OK)
			{
				const char* msg = archive_error_string(archive);
				cerr << m_utilName << ": could not install " + archive_filename << " : " << msg << endl;
				exit(EXIT_FAILURE);
			}
			break;

		}

	}
	FREE_ARCHIVE(archive);
	if (checkFileExist(PKG_PRE_INSTALL))
	{
		m_actualAction = PKG_PREINSTALL_START;
  	progressInfo();	
		process preinstall(SHELL,PKG_PRE_INSTALL, 0 );
		if (preinstall.executeShell()) {
			exit(EXIT_FAILURE);
		}
		removeFile(m_root,PKG_PRE_INSTALL);
		m_actualAction = PKG_PREINSTALL_END;
		progressInfo();
	}
	chdir(buf);
}
void Pkgdbh::installArchivePackage(const string& filename, const set<string>& keep_list, const set<string>& non_install_list)
{
	struct archive* archive;
	struct archive_entry* entry;
	char buf[PATH_MAX];
	string absm_root;

	archive = archive_read_new();
	INIT_ARCHIVE(archive);

	if (archive_read_open_filename(archive,
	    filename.c_str(),
	    DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK)
		{
			m_actualError = CANNOT_OPEN_FILE;
			treatErrors(filename);
		}
	chdir(m_root.c_str());
	absm_root = getcwd(buf, sizeof(buf));
#ifndef NDEBUG
	cout << "absm_root: " <<  absm_root  << " and m_root: " << m_root<< endl;
#endif
	m_actualAction = PKG_INSTALL_START;
	progressInfo();
	for (m_installedFilesNumber = 0; archive_read_next_header(archive, &entry) ==
	     ARCHIVE_OK; ++m_installedFilesNumber) {

		m_actualAction = PKG_INSTALL_RUN;
		progressInfo();
		string archive_filename = archive_entry_pathname(entry);
		string reject_dir = trimFileName(absm_root + string("/") + string(PKG_REJECTED));
		string original_filename = trimFileName(absm_root + string("/") + archive_filename);
		string real_filename = original_filename;

		// Check if file is filtered out via INSTALL
		if (non_install_list.find(archive_filename) != non_install_list.end()) {
			mode_t mode;

			cout << m_utilName << ": ignoring " << archive_filename << endl;

			mode = archive_entry_mode(entry);

			if (S_ISREG(mode))
				archive_read_data_skip(archive);

			continue;
		}

		// Check if file should be rejected
		if (checkFileExist(real_filename) && keep_list.find(archive_filename) != keep_list.end())
			real_filename = trimFileName(reject_dir + string("/") + archive_filename);

		archive_entry_set_pathname(entry, const_cast<char*>
		                           (real_filename.c_str()));

		// Extract file
		unsigned int flags = ARCHIVE_EXTRACT_OWNER | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_UNLINK;
		if (archive_read_extract(archive, entry, flags) != ARCHIVE_OK) {
			// If a file fails to install we just print an error message and
			// continue trying to install the rest of the package.
			const char* msg = archive_error_string(archive);
			cerr << m_utilName << ": could not install " + archive_filename << ": " << msg << endl;
			continue;

		}

		// Check rejected file
		if (real_filename != original_filename) {
			bool remove_file = false;
			mode_t mode = archive_entry_mode(entry);

			// Directory
			if (S_ISDIR(mode))
				remove_file = checkPermissionsEqual(real_filename, original_filename);
			// Other files
			else
				remove_file = checkPermissionsEqual(real_filename, original_filename) &&
					(checkFileEmpty(real_filename) || checkFilesEqual(real_filename, original_filename));

			// Remove rejected file or signal about its existence
			if (remove_file)
				removeFile(reject_dir, real_filename);
			else
				cout << m_utilName << ": rejecting " << archive_filename << ", keeping existing version" << endl;
		}
	}
	m_actualAction = PKG_INSTALL_END;
	progressInfo();
	if (m_installedFilesNumber == 0) {
		if (archive_errno(archive) == 0)
		{
			m_actualError = EMPTY_PACKAGE;
			treatErrors(filename);
		}
		else
		{
			m_actualError = CANNOT_READ_FILE;
			treatErrors(filename);
		}
	}
	FREE_ARCHIVE(archive);
}
void Pkgdbh::runLdConfig()
{
	// Only execute runLdConfig if /etc/ld.so.conf exists
	if (checkFileExist(m_root + LDCONFIG_CONF)) {
		string args = "-r " + m_root;
		process ldconfig(LDCONFIG, args,0);
		ldconfig.execute();
	}
}

void Pkgdbh::getFootprintPackage(string& filename)
{
	unsigned int i;
	struct archive* archive;
	struct archive_entry* entry;

	map<string, mode_t> hardlink_target_modes;

	// We first do a run over the archive and remember the modes
	// of regular files.
	// In the second run, we print the footprint - using the stored
	// modes for hardlinks.
	//
	// FIXME the code duplication here is butt ugly
	archive = archive_read_new();
	INIT_ARCHIVE(archive);

	if (archive_read_open_filename(archive,
	    filename.c_str(),
	    DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK)
			{
				m_actualError = CANNOT_OPEN_FILE;
				treatErrors(filename);
			}
      //         throw RunTimeErrorWithErrno("could not open " + filename, archive_errno(archive));

	for (i = 0; archive_read_next_header(archive, &entry) ==
	     ARCHIVE_OK; ++i) {

		mode_t mode = archive_entry_mode(entry);

		if (!archive_entry_hardlink(entry)) {
			const char *s = archive_entry_pathname(entry);

			hardlink_target_modes[s] = mode;
		}

		if (S_ISREG(mode) && archive_read_data_skip(archive))
		{
			m_actualError = CANNOT_READ_FILE;
			treatErrors(filename);
		//	throw RunTimeErrorWithErrno("could not read " + filename, archive_errno(archive));
		}
	}
	FREE_ARCHIVE(archive);

	// Too bad, there doesn't seem to be a way to reuse our archive
	// instance
	archive = archive_read_new();
	INIT_ARCHIVE(archive);

	if (archive_read_open_filename(archive,
	    filename.c_str(),
	    DEFAULT_BYTES_PER_BLOCK) != ARCHIVE_OK)
			{
				m_actualError = CANNOT_OPEN_FILE;
				treatErrors(filename);
        // throw RunTimeErrorWithErrno("could not open " + filename, archive_errno(archive));
			}
	for (i = 0; archive_read_next_header(archive, &entry) ==
	     ARCHIVE_OK; ++i) {
		mode_t mode = archive_entry_mode(entry);

		// Access permissions
		if (S_ISLNK(mode)) {
			// Access permissions on symlinks differ among filesystems, e.g. XFS and ext2 have different.
			// To avoid getting different footprints we always use "lrwxrwxrwx".
			cout << "lrwxrwxrwx";
		} else {
			const char *h = archive_entry_hardlink(entry);

			if (h)
				cout << mtos(hardlink_target_modes[h]);
			else
				cout << mtos(mode);
		}

		cout << '\t';

		// User
		uid_t uid = archive_entry_uid(entry);
		struct passwd* pw = getpwuid(uid);
		if (pw)
			cout << pw->pw_name;
		else
			cout << uid;

		cout << '/';

		// Group
		gid_t gid = archive_entry_gid(entry);
		struct group* gr = getgrgid(gid);
		if (gr)
			cout << gr->gr_name;
		else
			cout << gid;

		// Filename
		cout << '\t' << archive_entry_pathname(entry);

		// Special cases
		if (S_ISLNK(mode)) {
			// Symlink
			cout << " -> " << archive_entry_symlink(entry);
		} else if (S_ISCHR(mode) ||
		           S_ISBLK(mode)) {
			// Device
			cout << " (" << archive_entry_rdevmajor(entry)
			     << ", " << archive_entry_rdevminor(entry)
			     << ")";
		} else if (S_ISREG(mode) &&
		           archive_entry_size(entry) == 0) {
			// Empty regular file
			cout << " (EMPTY)";
		}

		cout << '\n';

		if (S_ISREG(mode) && archive_read_data_skip(archive))
		{
			m_actualError = CANNOT_READ_FILE;
			treatErrors(filename);
		//	throw RunTimeErrorWithErrno("could not read " + filename, archive_errno(archive));
		}
	}

	if (i == 0) {
		if (archive_errno(archive) == 0)
		{
			m_actualError = EMPTY_PACKAGE;
			treatErrors(filename);
		}
		else
		{
			m_actualError = CANNOT_READ_FILE;
			treatErrors(filename);
		}
	}
	FREE_ARCHIVE(archive);
}

void Pkgdbh::print_version() const
{
	cout << m_utilName << " (cards) " << VERSION << endl;
}

Db_lock::Db_lock(const string& m_root, bool exclusive)
	: m_dir(0)
{
	const string dirname = trimFileName(m_root + string("/") + PKG_DB_DIR);

	if (!(m_dir = opendir(dirname.c_str())))
		throw RunTimeErrorWithErrno("could not read directory " + dirname);

	if (flock(dirfd(m_dir), (exclusive ? LOCK_EX : LOCK_SH) | LOCK_NB) == -1) {
		if (errno == EWOULDBLOCK)
			throw runtime_error("package database is currently locked by another process");
		else
			throw RunTimeErrorWithErrno("could not lock directory " + dirname);
	}
}

Db_lock::~Db_lock()
{
	if (m_dir) {
		flock(dirfd(m_dir), LOCK_UN);
		closedir(m_dir);
	}
}

/*******************     End of Members *********************/

/*******************   Various fonctions ********************/

void assertArgument(char** argv, int argc, int index)
{
	if (argc - 1 < index + 1)
		throw runtime_error("option " + string(argv[index]) + " requires an argument");
}
void rotatingCursor() {
  static int pos=0;
  char cursor[4]={'/','-','\\','|'};
  printf("%c\b", cursor[pos]);
  fflush(stdout);
  pos = (pos+1) % 4;
}
// vim:set ts=2 :