//////////////////////////////////////////////////////////////////////
// Godot Format 0.01
//
// WARNING : This program is DESTRUCTIVE, be sure to commit
// your code as a backup BEFORE running godot format.
// BE SURE to test on a test folder the first time you use it, to make sure
// you understand how to use.
// Running it in the wrong folder could be *very bad*, although it creates backups
// they would need to be restored manually.

// This simple program (linux only so far, feel free to #define windows file functions)
// automatically edits files to pass Godot static checks. It does the changes that clang format
// cannot.
//
// It operates RECURSIVELY through folders on .h, .cpp, .glsl files (but not .gen.h)
//
// * Adds licences
// * Removes extra blank lines
// * Removes extra tabs
//
// Installation
// g++ godot_format.cpp -o godot_format
//
// Running
// ./godot_format --verbose --dryrun --sure /home/juan/godot/drivers
//
// --verbose gives extra log info
// --dryrun is a good idea especially first time, it does everything except write the modified files
// --sure omits the question before modifying each file. In most cases you should not use this.

// Original files will be renamed to .gf_backup,
// Modified files will replace the originals.
// You will have to delete the backup files manually (as a safety measure)
//
// Note that Godot static checks are not run on some folders (e.g. thirdparty).
// So you will generally only need to run this program in areas you are modifying.
//////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <sys/stat.h>
#include <vector>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

bool g_Verbose = false;
bool g_DryRun = false;
bool g_SureQuestion = true;

using namespace std;

std::vector<char> g_Buffer;
std::vector<char> g_BufferTemp;
std::vector<char> g_BufferRemove;
std::vector<char> g_BufferLicence;

const char g_szLicence[] = "/*************************************************************************/\n\
/*                                                                       */\n\
/*************************************************************************/\n\
/*                       This file is part of:                           */\n\
/*                           GODOT ENGINE                                */\n\
/*                      https://godotengine.org                          */\n\
/*************************************************************************/\n\
/* Copyright (c) 2007-2020 Juan Linietsky, Ariel Manzur.                 */\n\
/* Copyright (c) 2014-2020 Godot Engine contributors (cf. AUTHORS.md).   */\n\
/*                                                                       */\n\
/* Permission is hereby granted, free of charge, to any person obtaining */\n\
/* a copy of this software and associated documentation files (the       */\n\
/* \"Software\"), to deal in the Software without restriction, including   */\n\
/* without limitation the rights to use, copy, modify, merge, publish,   */\n\
/* distribute, sublicense, and/or sell copies of the Software, and to    */\n\
/* permit persons to whom the Software is furnished to do so, subject to */\n\
/* the following conditions:                                             */\n\
/*                                                                       */\n\
/* The above copyright notice and this permission notice shall be        */\n\
/* included in all copies or substantial portions of the Software.       */\n\
/*                                                                       */\n\
/* THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,       */\n\
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */\n\
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/\n\
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */\n\
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */\n\
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */\n\
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */\n\
/*************************************************************************/\n\
\n";
		
const int g_iLicenceSize = strlen(g_szLicence);

class LFolder
{
public:
	bool SetFilename(const char * pszFilename);
	const char * GetFilename() const {return m_szFilename;}
	
private:	
	char m_szFilename[1024];
};

class LFile
{
public:
	enum eType
	{
		FT_NONE,
		FT_H,
		FT_CPP,
		FT_INC,
		FT_GLSL,
	};
	
	bool Run();
	
	void SetFilename(const char * pszFilename) {	strcpy(m_szFilename, pszFilename);}
	const char * GetFilename() const {return m_szFilename;}
	
	void SetShortName(const char * pszName) {	strcpy(m_szShortName, pszName);}
	const char * GetShortName() const {return m_szShortName;}
	
	eType m_Type;
	
private:	
	bool Contract(int &buffer_size);
	bool AddLicence(int &buffer_size);
	
	int GetFileSize(const char * pszFilename);
	char m_szFilename[1024];
	char m_szShortName[1024];
};


class LFileTree
{
public:
	bool Run(const char * pszPath);
	
private:
	bool ProcessFolder();
	void FoundFile(struct dirent *dirp, const char * pszPath);
	void FoundFolder(struct dirent *dirp, const char * pszPath);
	
	LFile::eType IsFileInteresting(const char * pszFilename);
	
	std::vector<LFile> m_Files;
	std::vector<LFolder> m_Folders;
};


/////////////////////////////////////////////



bool LFolder::SetFilename(const char * pszFilename)
{
	int l = strlen(pszFilename);	
	if (l > 1022)
		return false;
	
	strcpy(m_szFilename, pszFilename);
	if (m_szFilename[l-1] != '/')
		strcat (m_szFilename, "/");
	
	return true;
}

int LFile::GetFileSize(const char * pszFilename)
{
	struct stat stat_buf;
	int rc = stat(pszFilename, &stat_buf);
	return rc == 0 ? stat_buf.st_size : -1;
}

bool LFile::AddLicence(int &buffer_size)
{
	//	return false;
	
	// only for certain file types
	switch (m_Type)
	{
	case eType::FT_CPP:
	case eType::FT_H:
		break;
	default:
		return false;
		break;
	}
	
	
	// first construct the license for this file
	// blank the line
	memset(&g_BufferLicence[78], 32, 70);
	
	const char * pszName = GetShortName();
	int l = strlen(pszName);
	
	memcpy(&g_BufferLicence[80], pszName, l);
	
	
	// compare
	int comp = memcmp(&g_Buffer[0], &g_BufferLicence[0], g_iLicenceSize);
	
	if (comp != 0)
	{
		// licence does not match, add it to the file
		strcpy(&g_BufferTemp[0], &g_BufferLicence[0]);
		strcat(&g_BufferTemp[0], &g_Buffer[0]);
		
		strcpy(&g_Buffer[0], &g_BufferTemp[0]);
		buffer_size = strlen(&g_Buffer[0]);
		
		
		if (g_Verbose)
			cout << "\tadding LICENCE" << endl;
		
		return true;
		// do character by character
		//		for (int n=0; n<g_iLicenceSize; n++)
		//		{
		//			if (g_Buffer[n] != g_BufferLicence[n])
		//			{
		//				cout << g_Buffer[n];
		//			}
		//		}
		
		//		cout << &g_BufferLicence[0] << endl;
	}
	
	return false;	
}


// returns true if it contracts the buffer
bool LFile::Contract(int &buffer_size)
{
	// first blank the removal buffer
	memset(&g_BufferRemove[0], 0, buffer_size); 
	
	const char * psz = &g_Buffer[0];
	
	bool changed_file = false;
	
	int line = 0;
	for (int n=0; n<buffer_size-5; n++)
	{
		if (psz[n] == '\n')
			line++;
		
		if  ((psz[n] == '{')
			 && (psz[n+1] == '\n')
			 && (psz[n+2] == '\n')
			 && (psz[n+3] == '\t'))
		{
			changed_file = true;
			
			if (g_Verbose)
				cout << "\tmatch line " << line << ", { extra blank line" << endl;
			g_BufferRemove[n+1] = 1;
		}
		
		
		if  ((psz[n] == '\n')
			 && (psz[n+1] == '\n')
			 && (psz[n+2] == '}'))
		{
			changed_file = true;
			
			if (g_Verbose)
				cout << "\tmatch line " << line << ", } extra blank line" << endl;
			g_BufferRemove[n+1] = 1;
		}
		
		// double blank lines
		if  ((psz[n] == '\n')
			 && (psz[n+1] == '\n')
			 && (psz[n+2] == '\n'))
		{
			changed_file = true;
			
			if (g_Verbose)
				cout << "\tmatch line " << line << ", extra blank line" << endl;
			g_BufferRemove[n+1] = 1;
		}
		
		if  ((psz[n] == '\t')
			 && (psz[n+1] == '\n'))
		{
			changed_file = true;
			
			if (g_Verbose)
				cout << "\tmatch line " << line << ", extra tab" << endl;
			g_BufferRemove[n] = 1;
		}
		
		if  ((psz[n] == ' ')
			 && (psz[n+1] == '\n'))
		{
			changed_file = true;
			
			if (g_Verbose)
				cout << "\tmatch line " << line << ", extra space" << endl;
			g_BufferRemove[n] = 1;
		}
		
	}
	
	if (!changed_file)
		return false;
	
	// write out to temp buffer
	int count = 0;
	
	for (int n=0; n<buffer_size; n++)
	{
		// marked to remove?
		if (!g_BufferRemove[n])
		{
			g_BufferTemp[count++] = g_Buffer[n];
		}
	}
	
	// copy back to orig buffer
	memcpy(&g_Buffer[0], &g_BufferTemp[0], count);
	g_Buffer[count] = 0; // null terminator
	
	// change the buffer size
	buffer_size = count;
	
	return true;	
}


bool LFile::Run()
{
	int filesize = GetFileSize(GetFilename());
	if (filesize <= 0)
		return false;
	
	//	cout << "\t" << GetFilename() << endl;
	
	
	int max_buffer_size = filesize + g_iLicenceSize + 1;
	
	// read the data
	FILE * pFile = fopen(GetFilename(), "rb");
	if (!pFile)
	{
		if (g_Verbose)
			cout << "WARNING : cannot open file " << GetFilename() << ", ignoring" << endl;
		
		return false;
	}
	g_Buffer.resize(max_buffer_size);
	g_BufferTemp.resize(max_buffer_size);
	
	// make a buffer to indicate which chars to remove from the final stream
	g_BufferRemove.resize(max_buffer_size);
	memset(&g_BufferRemove[0], 0, max_buffer_size); 
	
	fread(&g_Buffer[0], 1, filesize, pFile);
	fclose (pFile);
	
	// null terminator!!
	g_Buffer[filesize] = 0;
	
	bool changed_file = false;
	int buffersize = filesize;
	
	// first make sure the MIT licence is correct
	if (AddLicence(buffersize))
		changed_file = true;
	
	while (Contract(buffersize))
	{
		changed_file = true;
	}
	
	if (!changed_file)
		return false;
	
	cout << "MODIFYING : " << endl << GetFilename() << endl;
	
	if (g_DryRun)
		return false;
	
	if (g_SureQuestion)
	{
		cout << "\tare you sure?(y/n)" << endl;
		
		fflush(stdin);
		char cSure = getchar();
		
		if (cSure != 'y')
		{
			cout << "USER chose to abort process." << endl;
			exit(0);
		}
	}	
	
	// if we have modified the file, let's create it
	char szTemp[2048];
	strcpy(szTemp, GetFilename());
	strcat(szTemp, ".gf_temp");
	
	pFile = fopen(szTemp, "wb");
	if (!pFile)
		return false;
	
	fwrite(&g_Buffer[0], 1, buffersize, pFile);
	
	fflush(pFile);
	fclose(pFile);
	
	
	char szBackup[2048];
	strcpy(szBackup, GetFilename());
	strcat(szBackup, ".gf_backup");
	
	// delete the original
	rename(GetFilename(), szBackup);
	//	remove(GetFilename());
	
	// rename to replace
	rename(szTemp, GetFilename());
	
	return true;
}


//////////////////////////////////

void LFileTree::FoundFile(struct dirent *dirp, const char * pszPath)
{
	//    DupItemInfo inf;
	//    memset (&inf, 0, sizeof (inf));
	
	const char * pszName = dirp->d_name;
	
	LFile::eType ftype = IsFileInteresting(pszName);
	
	if (ftype == LFile::eType::FT_NONE)
		return;
	
	int l = strlen(pszName);
	assert (l);
	
	//    cout << "\t" << pszName << endl;
	
	// create a new folder in the list of folders by appending the folder name to the path
	int lp = strlen(pszPath);
	int ln = strlen(pszName);
	
	char path[1024];
	
	if ((lp + ln) > 1022)
		return;
	
	strcpy (path, pszPath);
	strcat (path, pszName);
	
	LFile file;
	file.SetFilename(path);
	file.SetShortName(pszName);
	file.m_Type = ftype;
	
	m_Files.push_back(file);
	
}

bool EndsIn(const char * psz, const char * pszEnd)
{
	int l = strlen(psz);
	int le = strlen(pszEnd);
	
	if (l <= le)
		return false;
	
	if (!memcmp(&psz[l-le], pszEnd, le))
		return true;
	
	return false;
}

LFile::eType LFileTree::IsFileInteresting(const char * pszFilename)
{
	if (!pszFilename)
		return LFile::eType::FT_NONE;
	
	int l = strlen(pszFilename);
	if (l < 9)
		return LFile::eType::FT_NONE;

	if (EndsIn(pszFilename, ".h"))
	{
		// only if NOT a gen file!
		if (EndsIn(pszFilename, ".gen.h"))
			return LFile::eType::FT_NONE;
			
		return LFile::eType::FT_H;
	}
	
	if (EndsIn(pszFilename, ".cpp"))
	{
		// only if NOT a gen file!
		if (EndsIn(pszFilename, ".gen.cpp"))
			return LFile::eType::FT_NONE;
			
		return LFile::eType::FT_CPP;
	}
	
	if (EndsIn(pszFilename, ".glsl"))
	{
		return LFile::eType::FT_GLSL;
	}
	
	return LFile::eType::FT_NONE;
}


void LFileTree::FoundFolder(struct dirent *dirp, const char * pszPath)
{
	const char * pszName = dirp->d_name;
	
	// check for specials : . and ..
	if (!strcmp(pszName, "."))
		return;
	
	if (!strcmp(pszName, ".."))
		return;
	
	// create a new folder in the list of folders by appending the folder name to the path
	int lp = strlen(pszPath);
	int ln = strlen(pszName);
	
	char path[1024];
	
	if ((lp + ln) > 1022)
		return;
	
	strcpy (path, pszPath);
	strcat (path, pszName);
	
	LFolder fold;
	fold.SetFilename(path);
	m_Folders.push_back(fold);
}

bool LFileTree::Run(const char * pszPath)
{
	// set up licence
	g_BufferLicence.resize(g_iLicenceSize+1);
	strcpy(&g_BufferLicence[0], g_szLicence);
	
	
	m_Folders.resize(m_Folders.size()+1);
	
	LFolder &fold = m_Folders[m_Folders.size()-1];
	
	fold.SetFilename(pszPath);
	
	while (m_Folders.size())
		ProcessFolder();
	
	// process files
	for (int n=0; n<m_Files.size(); n++)
	{
		m_Files[n].Run();
		//			return true;
	}
	
	return true;
}


bool LFileTree::ProcessFolder()
{
	LFolder fold = m_Folders[0];
	const char * pszPath = fold.GetFilename();
	
	// delete folder from list
	m_Folders[0] = m_Folders[m_Folders.size()-1];
	m_Folders.pop_back();
	
	
	//    cout << "Folder\t" << pszPath << endl;
	
	DIR *dp;
	struct dirent *dirp;
	
	if((dp = opendir(pszPath)) == 0)
	{
		cout << "ERROR(" << errno << ") opening " << pszPath << endl;
		return false;
	}
	
	while ((dirp = readdir(dp)) != 0)
	{
		
		switch (dirp->d_type)
		{
		case DT_REG:
			// do nothing, regular file
			FoundFile(dirp, pszPath);
			break;
		case DT_DIR:
			FoundFolder(dirp, pszPath);
			break;
		default:
			continue; // ignore special files
			break;
		}
	}
	closedir(dp);
	
	
	return true;
}

int main(int argc, char *argv[])
{
	cout << "GodotFormat v0.01" << endl;
	
	bool args_wrong = false;
	const char * pszPath = 0;
	
	for (int n=1; n<argc; n++)
	{
		const char * pArg = argv[n];
		
		if (!strcmp(pArg, "--verbose"))
		{
			g_Verbose = true;
			cout << "VERBOSE" << endl;
		}
		else if (!strcmp(pArg, "--dryrun"))
		{
			g_DryRun = true;
			cout << "DRYRUN" << endl;
		}
		else if (!strcmp(pArg, "--sure"))
		{
			g_SureQuestion = false;
			cout << "SURE" << endl;
		}
		else
		{
			if (n == (argc-1))
			{
				pszPath = pArg;
			}
			else
			{
				args_wrong = true;
				break;
			}
		}
	}
	
	if (args_wrong)
	{
		cout << "wrong arguments, expects [--verbose] [--dryrun] [folder path to process]" << endl;
		return 0;
	}
	
	cout << "Path : " << pszPath << endl;
	
	LFileTree tree;
	
	//	tree.Run("/home/baron/Apps/Godot4/godot/drivers");
	//	tree.Run("/home/baron/Apps/Godot4/godot/platform/linuxbsd");
	//	tree.Run("/home/baron/Apps/Godot4/godot/platform/windows");
	
	tree.Run(pszPath);
	
	cout << "completed OK" << endl;
	
	return 0;
}
