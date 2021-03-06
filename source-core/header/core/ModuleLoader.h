//READ LICENSE BEFORE ANY USAGE
/* Copyright (C) 2018  Damien DUBUC ddubuc@aneo.fr (ANEO S.A.S)
 *  Team Contact : hipe@aneo.fr
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *  
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *  
 *  In addition, we kindly ask you to acknowledge ANEO and its authors in any program 
 *  or publication in which you use HIPE. You are not required to do so; it is up to your 
 *  common sense to decide whether you want to comply with this request or not.
 *  
 *  Non-free versions of HIPE are available under terms different from those of the General 
 *  Public License. e.g. they do not require you to accompany any object code using HIPE 
 *  with the corresponding source code. Following the new licensing any change request from 
 *  contributors to ANEO must accept terms of re-license by a general announcement. 
 *  For these alternative terms you must request a license from ANEO S.A.S Company 
 *  Licensing Office. Users and or developers interested in such a license should 
 *  contact us (hipe@aneo.fr) for more information.
 */

#pragma once
#include <string>
#include <core/misc.h>
#include <core/StringTools.h>
#include <core/HipeException.h>

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "ImageHlp.h"
#else
#include <link.h>
#include <dlfcn.h>
#endif

#pragma warning(push, 0)
#include <boost/function_types/components.hpp>
#include <boost/function_types/function_pointer.hpp>
#include <corefilter/tools/Localenv.h>
#pragma warning(pop)


namespace core
{
#ifndef WIN32
	typedef void * HINSTANCE;
#endif
	class ModuleLoader
	{
		std::vector<std::string> _filenames;
		std::vector<HINSTANCE> dllHandles;

#ifdef WIN32
		

		template <class T>
		PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva, T* pNTHeader) // 'T' == PIMAGE_NT_HEADERS 
		{
			PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
			unsigned i;

			for (i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++)
			{
				// This 3 line idiocy is because Watcom's linker actually sets the
				// Misc.VirtualSize field to 0.  (!!! - Retards....!!!)
				DWORD size = section->Misc.VirtualSize;
				if (0 == size)
					size = section->SizeOfRawData;

				// Is the RVA within this section?
				if ((rva >= section->VirtualAddress) &&
					(rva < (section->VirtualAddress + size)))
					return section;
			}

			return 0;
		}

		template <class T>
		LPVOID GetPtrFromRVA(DWORD rva, T* pNTHeader, PBYTE imageBase) // 'T' = PIMAGE_NT_HEADERS 
		{
			PIMAGE_SECTION_HEADER pSectionHdr;
			INT delta;

			pSectionHdr = GetEnclosingSectionHeader(rva, pNTHeader);
			if (!pSectionHdr)
				return 0;

			delta = (INT)(pSectionHdr->VirtualAddress - pSectionHdr->PointerToRawData);
			return (PVOID)(imageBase + rva - delta);
		}


		void DumpDllFromPath(std::string path, std::vector<std::string> & stackDeps)
		{
			char name[300];
			std::wstring mainDllpath = std::wstring(path.begin(), path.end());
			wcstombs(name, mainDllpath.c_str(), 300);

			PLOADED_IMAGE image = ImageLoad(name, 0);

			if (! image )
			{
				std::cout <<  " Deps dllname = " << name << " [ IMAGE NOT FOUND ]" << std::endl;
				stackDeps.push_back(name);
				std::sort(stackDeps.begin(), stackDeps.end());
				return;
			}
			std::vector<std::string> listDeps;

			if (image->FileHeader->OptionalHeader.NumberOfRvaAndSizes >= 2)
			{
				PIMAGE_IMPORT_DESCRIPTOR importDesc =
					(PIMAGE_IMPORT_DESCRIPTOR)GetPtrFromRVA(
						image->FileHeader->OptionalHeader.DataDirectory[1].VirtualAddress,
						image->FileHeader, image->MappedAddress);
				if (importDesc)
				{
					while (1)
					{
						// See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR
						if ((importDesc->TimeDateStamp == 0) && (importDesc->Name == 0))
							break;

						//printf("  %s\n", );
						listDeps.push_back((char *)GetPtrFromRVA(importDesc->Name,
													   image->FileHeader,
													   image->MappedAddress));
						importDesc++;
					}
				}
				else
				{
					std::cout <<  " Deps dllname = " << image->ModuleName << " [ Warnings : MISSING IMPORT ]" << std::endl;
				}


			}

			for (std::string dllname : listDeps)
			{
				//std::cout <<  " Deps dllname = " << dllname;
				HINSTANCE dllDepsHandle = LoadLibraryEx(path.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
				if (dllDepsHandle)
				{
					//std::cout << " [ OK ] " << std::endl;
					FreeLibrary(dllDepsHandle);
					std::wstring widestr = std::wstring(dllname.begin(), dllname.end());
					if (std::find(stackDeps.begin(), stackDeps.end(), dllname) == stackDeps.end())
					{
						stackDeps.push_back(dllname);
						std::sort(stackDeps.begin(), stackDeps.end());
						DumpDllFromPath(dllname, stackDeps);
					}
				}
				else
				{
					std::cout <<  " Deps dllname = " << dllname << " [ NOT FOUND ] " << std::endl;
				}
			}

			ImageUnload(image);
		}
#endif

	public:
		ModuleLoader()
		{
		}

		//Define the function prototype
		void loadLibrary(std::string filename)
		{
			//Get the directory name
			std::string dll_dir = extractDirectoryName(filename);

			addEnv(dll_dir);

			corefilter::getLocalEnv().setValue("pydata_path", dll_dir);

#ifdef WIN32

			//Load the dll and keep the handle to it
			HINSTANCE dllHandle = LoadLibrary(filename.c_str());
			DWORD dw = GetLastError();
			
#else
			//#FIXME
			void * dllHandle = dlopen(filename.c_str(), RTLD_LAZY);
#endif
			
			if (!dllHandle)
			{
				std::stringstream msg;
				msg << "Could not locate the shared library \"" << filename + "\"";
#ifdef WIN32
				wchar_t err[4096];
				memset(err, 0, 4096);
				//std::wstring widestr = std::wstring(filename.begin(), filename.end());
				std::vector<std::string> stackDeps;
				DumpDllFromPath(filename, stackDeps);
				FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
							  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 4095, NULL);
				
				std::cerr << msg.str() << " Last Error " << err << std::endl;
#else
				std::cerr << msg.str() << " DL message : " << dlerror() << std::endl;

#endif
				throw HipeException(msg.str());
			}
			else
			{
				dllHandles.push_back(dllHandle);
			}
#ifdef WIN32
			
#else // LINUX
			char path_to_module[4096];
			dlinfo(dllHandle, RTLD_DI_ORIGIN, (void *)path_to_module);
			std::cout << "Module is located : " << path_to_module << std::endl;
			corefilter::getLocalEnv().setValue("pydata_path", std::string(path_to_module));
#endif
			
		}

		template <typename T>
		std::function<T> callFunction(const std::string& funcName)
		{
			std::function<T> fctPtr = nullptr;

			//TODO need to check if funcName is in multiple dll
			for (auto dllHandle : dllHandles)
			{
				std::function<T> call_function = callFunction<T>(funcName, dllHandle);
				if (call_function != nullptr)
					return call_function;
			}
			return fctPtr;
		}

		template <typename T>
		std::function<T> callFunction(const std::string& funcName, HINSTANCE dllHandle)
		{
			namespace ft = boost::function_types;
			typedef typename ft::function_pointer< typename ft::components<T>::type >::type fp_t;
			fp_t fun_ptr = nullptr;
			// Locate function in DLL.
#ifdef WIN32
			
			FARPROC lpfnGetProcessID = GetProcAddress(dllHandle, funcName.c_str());
#else
			void * lpfnGetProcessID = dlsym(dllHandle, funcName.c_str());
#endif
			// Check if function was located.
			if (!lpfnGetProcessID)
			{
				std::stringstream msg;
				msg << "Could not locate the function \"" << funcName << "\"";
				std::cerr << msg.str() << std::endl;	
				return fun_ptr;
			}
			

			*reinterpret_cast<void**>(&fun_ptr) = lpfnGetProcessID;


			return fun_ptr;
		}

		
		void freeLibrary()
		{
#ifdef WIN32
			
			BOOL freeResult = FALSE;
			//Free the library:
			for (auto dllHandle : dllHandles)
				freeResult = FreeLibrary(dllHandle);
#else
			//FIXME
#endif
		}
	};

}
