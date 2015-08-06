﻿#include "HtmlFile.h"
#include "../DesktopEditor/common/File.h"
#include "../DesktopEditor/common/StringBuilder.h"
#include "../DesktopEditor/common/String.h"
#include "../DesktopEditor/xml/include/xmlutils.h"

#include <vector>
#include <map>

#ifdef LINUX
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#endif

CHtmlFile::CHtmlFile()
{

}

CHtmlFile::~CHtmlFile()
{

}

int CHtmlFile::Convert(const std::wstring& sXml, const std::wstring& sPathInternal)
{
    std::wstring sInternal = sPathInternal;
    if (sInternal.empty())
        sInternal = NSFile::GetProcessDirectory() + L"/HtmlFileInternal/";

    sInternal += L"HtmlFileInternal";

#ifdef WIN32
    sInternal += L".exe";
#endif

    int nReturnCode = 0;

#ifdef WIN32
    STARTUPINFO sturtupinfo;
    ZeroMemory(&sturtupinfo,sizeof(STARTUPINFO));
    sturtupinfo.cb = sizeof(STARTUPINFO);

    std::wstring sTempFileForParams = NSFile::CFileBinary::CreateTempFileWithUniqueName(NSFile::CFileBinary::GetTempPath(), L"XML");
    NSFile::CFileBinary oFile;
    oFile.CreateFileW(sTempFileForParams);
    oFile.WriteStringUTF8(sXml, true);
    oFile.CloseFile();

    std::wstring sApp = L"HtmlFileInternal <html>" + sTempFileForParams;
    wchar_t* pCommandLine = NULL;
    if (!sXml.empty())
    {
        pCommandLine = new wchar_t[sApp.length() + 1];
        memcpy(pCommandLine, sApp.c_str(), sApp.length() * sizeof(wchar_t));
        pCommandLine[sApp.length()] = (wchar_t)'\0';
    }

    PROCESS_INFORMATION processinfo;
    ZeroMemory(&processinfo,sizeof(PROCESS_INFORMATION));
    BOOL bResult = CreateProcessW(sInternal.c_str(), pCommandLine,
                               NULL, NULL, TRUE, NULL, NULL, NULL, &sturtupinfo, &processinfo);

    ::WaitForSingleObject(processinfo.hProcess, INFINITE);

    RELEASEARRAYOBJECTS(pCommandLine);

    //get exit code
    DWORD dwExitCode = 0;
    if (GetExitCodeProcess(processinfo.hProcess, &dwExitCode))
    {
        nReturnCode = (int)dwExitCode;
    }

    CloseHandle(processinfo.hProcess);
    CloseHandle(processinfo.hThread);

    NSFile::CFileBinary::Remove(sTempFileForParams);
#endif

#ifdef LINUX
    std::wstring sTempFileForParams = NSFile::CFileBinary::CreateTempFileWithUniqueName(NSFile::CFileBinary::GetTempPath(), L"XML");
    NSFile::CFileBinary oFile;
    oFile.CreateFileW(sTempFileForParams);
    oFile.WriteStringUTF8(sXml, true);
    oFile.CloseFile();

    pid_t pid = fork(); // create child process
    int status;

    std::string sProgramm = U_TO_UTF8(sInternal);
    std::string sXmlA = "<html>" + U_TO_UTF8(sTempFileForParams);

    switch (pid)
    {
    case -1: // error
        break;

    case 0: // child process
    {
        std::string sLibraryDir = sProgramm;
        if (std::string::npos != sProgramm.find_last_of('/'))
            sLibraryDir = "LD_LIBRARY_PATH=" + sProgramm.substr(0, sProgramm.find_last_of('/'));

        const char* nargs[2];
        nargs[0] = sXmlA.c_str();
        nargs[1] = NULL;

        const char* nenv[3];
        nenv[0] = sLibraryDir.c_str();
        nenv[1] = "DISPLAY=:0";
        nenv[2] = NULL;

        execve(sProgramm.c_str(),
               (char * const *)nargs,
               (char * const *)nenv);
        exit(EXIT_SUCCESS);
        break;
    }
    default: // parent process, pid now contains the child pid
        while (-1 == waitpid(pid, &status, 0)); // wait for child to complete
        if (WIFEXITED(status))
        {
            nReturnCode =  WEXITSTATUS(status);
        }
        break;
    }
#endif

    NSFile::CFileBinary::Remove(sTempFileForParams);
    return nReturnCode;
}

/////////////////////////////////////////////////////////////////
// EPUB
/////////////////////////////////////////////////////////////////

static std::vector<std::wstring> ParseEpub(const std::wstring& sPackagePath, std::wstring& sMetaInfo)
{
    std::vector<std::wstring> arHtmls;

    XmlUtils::CXmlNode oNodeRoot;
    if (!oNodeRoot.FromXmlFile(sPackagePath))
        return arHtmls;

    XmlUtils::CXmlNode oNodeMeta = oNodeRoot.ReadNodeNoNS(L"metadata");
    if (oNodeMeta.IsValid())
    {
        NSStringUtils::CStringBuilder oBuilder;

        std::wstring sTitle         = oNodeMeta.ReadValueString(L"dc:title");
        std::wstring sCreator       = oNodeMeta.ReadValueString(L"dc:creator");
        std::wstring sPublisher     = oNodeMeta.ReadValueString(L"dc:publisher");
        std::wstring sLanguage      = oNodeMeta.ReadValueString(L"dc:language");
        std::wstring sContributor   = oNodeMeta.ReadValueString(L"dc:contributor");
        std::wstring sDescription   = oNodeMeta.ReadValueString(L"dc:description");
        std::wstring sCoverage      = oNodeMeta.ReadValueString(L"dc:coverage");

        XmlUtils::CXmlNodes oMetaNodes = oNodeMeta.ReadNodesNoNS(L"meta");
        if (oMetaNodes.IsValid())
        {
            int nCountMeta = oMetaNodes.GetCount();
            for (int i = 0; i < nCountMeta; ++i)
            {
                XmlUtils::CXmlNode oNodeTmp;
                oMetaNodes.GetAt(i, oNodeTmp);

                std::wstring sName = oNodeTmp.GetAttribute(L"name");
                if (sName == L"cover")
                    sCoverage = L"1";
            }
        }

        if (!sTitle.empty())
        {
            oBuilder.WriteString(L"<name>");
            oBuilder.WriteEncodeXmlString(sTitle.c_str(), (int)sTitle.length());
            oBuilder.WriteString(L"</name>");
        }
        if (!sCreator.empty())
        {
            oBuilder.WriteString(L"<author>");
            oBuilder.WriteEncodeXmlString(sCreator.c_str(), (int)sCreator.length());
            oBuilder.WriteString(L"</author>");

            oBuilder.WriteString(L"<creator>");
            oBuilder.WriteEncodeXmlString(sCreator.c_str(), (int)sCreator.length());
            oBuilder.WriteString(L"</creator>");
        }
        if (!sPublisher.empty())
        {
            oBuilder.WriteString(L"<publisher>");
            oBuilder.WriteEncodeXmlString(sPublisher.c_str(), (int)sPublisher.length());
            oBuilder.WriteString(L"</publisher>");
        }
        if (!sLanguage.empty())
        {
            oBuilder.WriteString(L"<language>");
            oBuilder.WriteEncodeXmlString(sLanguage.c_str(), (int)sLanguage.length());
            oBuilder.WriteString(L"</language>");
        }
        if (!sContributor.empty())
        {
            oBuilder.WriteString(L"<creator>");
            oBuilder.WriteEncodeXmlString(sContributor.c_str(), (int)sContributor.length());
            oBuilder.WriteString(L"</creator>");
        }
        if (!sDescription.empty())
        {
            oBuilder.WriteString(L"<annotation>");
            oBuilder.WriteEncodeXmlString(sDescription.c_str(), (int)sDescription.length());
            oBuilder.WriteString(L"</annotation>");
        }
        if (!sCoverage.empty())
        {
            oBuilder.WriteString(L"<coverpage>1</coverpage>");
        }

        if (0 != oBuilder.GetCurSize())
            sMetaInfo = L"<meta>" + oBuilder.GetData() + L"</meta>";
    }

    XmlUtils::CXmlNode oNodeSpine = oNodeRoot.ReadNodeNoNS(L"spine");
    if (!oNodeRoot.IsValid())
        return arHtmls;

    XmlUtils::CXmlNodes oNodesItemRef = oNodeSpine.ReadNodesNoNS(L"itemref");
    if (!oNodeSpine.IsValid())
        return arHtmls;

    std::vector<std::wstring> sIds;
    int nCountRefs = oNodesItemRef.GetCount();
    for (int i = 0; i < nCountRefs; ++i)
    {
        XmlUtils::CXmlNode oNodeTmp;
        oNodesItemRef.GetAt(i, oNodeTmp);

        std::wstring sId = oNodeTmp.GetAttribute(L"idref");
        if (!sId.empty())
            sIds.push_back(sId);
    }

    if (0 == sIds.size())
        return arHtmls;

    XmlUtils::CXmlNode oNodeManifest = oNodeRoot.ReadNodeNoNS(L"manifest");
    if (!oNodeRoot.IsValid())
        return arHtmls;
    XmlUtils::CXmlNodes oNodesItems = oNodeManifest.ReadNodesNoNS(L"item");
    if (!oNodeManifest.IsValid())
        return arHtmls;

    size_t pos = sPackagePath.find_last_of((wchar_t)'/');
    std::wstring sPackagePathDir = sPackagePath;
    if (std::wstring::npos != pos)
        sPackagePathDir = sPackagePath.substr(0, pos + 1);

    std::map<std::wstring, std::wstring> mapHtmls;
    int nCountItems = oNodesItems.GetCount();
    for (int i = 0; i < nCountItems; ++i)
    {
        XmlUtils::CXmlNode oNodeTmp;
        oNodesItems.GetAt(i, oNodeTmp);

        std::wstring sMime = oNodeTmp.GetAttribute(L"media-type");
        std::wstring sHRef = oNodeTmp.GetAttribute(L"href");

#if 0
        //Decode URL
        sHRef.Replace(_T("%20"), _T(" "));
        sHRef.Replace(_T("%3B"), _T(";"));
        sHRef.Replace(_T("%2C"), _T(","));
        sHRef.Replace(_T("%26"), _T("&"));
        sHRef.Replace(_T("%3D"), _T("="));
        sHRef.Replace(_T("%2B"), _T("+"));
        sHRef.Replace(_T("%24"), _T("$"));
#endif

        std::wstring sId = oNodeTmp.GetAttribute(L"id");
        if (!sMime.empty() && !sHRef.empty())
            mapHtmls.insert(std::pair<std::wstring, std::wstring>(sId, sPackagePathDir + sHRef));
    }

    for (std::vector<std::wstring>::iterator iter = sIds.begin(); iter != sIds.end(); iter++)
    {
        std::map<std::wstring, std::wstring>::const_iterator i = mapHtmls.find(*iter);
        if (i != mapHtmls.end())
        {
            arHtmls.push_back(i->second);
        }
    }

    return arHtmls;
}

int CHtmlFile::ConvertEpub(const std::wstring& sFolder, std::wstring& sMetaInfo, const std::wstring& sXmlPart, const std::wstring& sPathInternal)
{
    std::wstring sFolderWithSlash = sFolder;
    NSStringExt::Replace(sFolderWithSlash, L"\\", L"/");

    if (!sFolderWithSlash.empty())
    {
        wchar_t c = sFolderWithSlash.c_str()[sFolderWithSlash.length() - 1];
        if (c != '/' && c != '\\')
            sFolderWithSlash += L"/";
    }

    std::wstring sMimeType = L"";
    if (!NSFile::CFileBinary::ReadAllTextUtf8(sFolderWithSlash + L"mimetype", sMimeType))
        return 1;

    if (sMimeType != L"application/epub+zip")
        return 1;

    std::wstring sContainer = sFolderWithSlash + L"META-INF/container.xml";
    XmlUtils::CXmlNode oNodeContainer;
    if (!oNodeContainer.FromXmlFile(sContainer))
        return 1;

    XmlUtils::CXmlNode oNodeRootFiles = oNodeContainer.ReadNodeNoNS(L"rootfiles");
    if (!oNodeRootFiles.IsValid())
        return 1;

    std::wstring sPackagePathXml;

    XmlUtils::CXmlNodes oNodesRootFile = oNodeRootFiles.ReadNodesNoNS(L"rootfile");
    if (!oNodeRootFiles.IsValid())
        return 1;

    int nCount = oNodesRootFile.GetCount();
    for (int i = 0; i < nCount; ++i)
    {
        XmlUtils::CXmlNode oNodeRF;
        oNodesRootFile.GetAt(i, oNodeRF);

        std::wstring sMime = oNodeRF.GetAttribute(L"media-type");
        std::wstring sPackagePath = oNodeRF.GetAttribute(L"full-path");

        if (!sPackagePath.empty() && L"application/oebps-package+xml" == sMime)
            sPackagePathXml = sFolderWithSlash + sPackagePath;
    }

    if (sPackagePathXml.empty())
        return 1;

    std::vector<std::wstring> arHtmls = ParseEpub(sPackagePathXml, sMetaInfo);
    if (arHtmls.size() == 0)
        return 1;

    NSStringUtils::CStringBuilder oBuilder;
    for (std::vector<std::wstring>::iterator iter = arHtmls.begin(); iter != arHtmls.end(); iter++)
    {
        oBuilder.WriteString(L"<file>", 6);

        wchar_t c = iter->c_str()[0];
        if (c == '/')
            oBuilder.WriteString(L"file://", 7);
        else
            oBuilder.WriteString(L"file:///", 8);
        oBuilder.WriteEncodeXmlString(iter->c_str(), iter->length());

        oBuilder.WriteString(L"</file>", 7);
    }

    return this->Convert(L"<html>" + oBuilder.GetData() + sXmlPart + L"</html>", sPathInternal);
}
