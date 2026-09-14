/******************************************************************************/
/**
* @file        ImportExport.h
* @project     Pulse Protocol
* @author      Chia Wei Xuan Rachael - 100%
* @brief       Defines DLL export/import macros for exposing C++ engine
*              functions to external consumers (C# scripts, other DLLs).
*
* @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
*              Reproduction or disclosure of this file or its contents without the
*              prior written consent of DigiPen Institute of Technology is prohibited.
*
/******************************************************************************/

#ifndef IMPORTEXPORT_H
#define IMPORTEXPORT_H

#ifndef DLL_API
#   if defined DLL_API_EXPORT
#       define DLL_API __declspec(dllexport)
#   else
#       define DLL_API __declspec(dllimport)
#   endif
#endif

#endif // !IMPORTEXPORT_H
