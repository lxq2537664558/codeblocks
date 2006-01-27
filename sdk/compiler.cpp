#include "sdk_precomp.h"

#ifndef CB_PRECOMP
    #include "compiler.h"
    #include "manager.h"
    #include "messagemanager.h"
    #include "configmanager.h"
    #include "globals.h"

    #include <wx/intl.h>
    #include <wx/regex.h>
#endif


#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(RegExArray);

wxString Compiler::CommandTypeDescriptions[COMPILER_COMMAND_TYPES_COUNT] =
{
    // These are the strings that describe each CommandType enumerator...
    // No need to say that it must have the same order as the enumerators!
    _("Compile single file to object file"),
    _("Generate dependencies for file"),
    _("Compile Win32 resource file"),
    _("Link object files to executable"),
    _("Link object files to console executable"),
    _("Link object files to dynamic library"),
    _("Link object files to static library")
};
long Compiler::CompilerIDCounter = 0; // built-in compilers can have IDs from 1 to 255
long Compiler::UserCompilerIDCounter = 255; // user compilers have IDs over 255 (256+)

Compiler::Compiler(const wxString& name)
    : m_Name(name),
    m_ID(++CompilerIDCounter),
    m_ParentID(-1)
{
	//ctor
    Manager::Get()->GetMessageManager()->DebugLog(_("Added compiler \"%s\""), m_Name.c_str());
}

Compiler::Compiler(const Compiler& other)
    : CompileOptionsBase(other),
    m_ID(++UserCompilerIDCounter),
    m_ParentID(other.m_ID)
{
    m_Name = _("Copy of ") + other.m_Name;
    m_MasterPath = other.m_MasterPath;
    m_Programs = other.m_Programs;
    m_Switches = other.m_Switches;
    m_Options = other.m_Options;
    m_IncludeDirs = other.m_IncludeDirs;
    m_LibDirs = other.m_LibDirs;
    m_CompilerOptions = other.m_CompilerOptions;
    m_LinkerOptions = other.m_LinkerOptions;
    m_LinkLibs = other.m_LinkLibs;
    m_CmdsBefore = other.m_CmdsBefore;
    m_CmdsAfter = other.m_CmdsAfter;
    m_RegExes = other.m_RegExes;
    for (int i = 0; i < COMPILER_COMMAND_TYPES_COUNT; ++i)
    {
        m_Commands[i] = other.m_Commands[i];
    }
}

Compiler::~Compiler()
{
	//dtor
}

void Compiler::SaveSettings(const wxString& baseKey)
{
    ConfigManager* cfg = Manager::Get()->GetConfigManager(_T("compiler"));

    wxString tmp;
    tmp.Printf(_T("%s/set%3.3d"), baseKey.c_str(), (int)m_ID);

	cfg->Write(tmp + _T("/name"), m_Name);
	cfg->Write(tmp + _T("/parent"), (int)m_ParentID);

	wxString key = GetStringFromArray(m_CompilerOptions);
	cfg->Write(tmp + _T("/compiler_options"), key, true);
	key = GetStringFromArray(m_LinkerOptions);
	cfg->Write(tmp + _T("/linker_options"), key, true);
	key = GetStringFromArray(m_IncludeDirs);
	cfg->Write(tmp + _T("/include_dirs"), key, true);
	key = GetStringFromArray(m_ResIncludeDirs);
	cfg->Write(tmp + _T("/res_include_dirs"), key, true);
	key = GetStringFromArray(m_LibDirs);
	cfg->Write(tmp + _T("/library_dirs"), key, true);
	key = GetStringFromArray(m_LinkLibs);
	cfg->Write(tmp + _T("/libraries"), key, true);
	key = GetStringFromArray(m_CmdsBefore);
	cfg->Write(tmp + _T("/commands_before"), key, true);
	key = GetStringFromArray(m_CmdsAfter);
	cfg->Write(tmp + _T("/commands_after"), key, true);

    cfg->Write(tmp + _T("/master_path"), m_MasterPath, true);
    cfg->Write(tmp + _T("/extra_paths"), GetStringFromArray(m_ExtraPaths, _T(";")), true);
    cfg->Write(tmp + _T("/c_compiler"), m_Programs.C, true);
    cfg->Write(tmp + _T("/cpp_compiler"), m_Programs.CPP, true);
    cfg->Write(tmp + _T("/linker"), m_Programs.LD, true);
    cfg->Write(tmp + _T("/lib_linker"), m_Programs.LIB, true);
    cfg->Write(tmp + _T("/res_compiler"), m_Programs.WINDRES, true);
    cfg->Write(tmp + _T("/make"), m_Programs.MAKE, true);
    cfg->Write(tmp + _T("/debugger"), m_Programs.DBG, true);

    for (int i = 0; i < COMPILER_COMMAND_TYPES_COUNT; ++i)
    {
        cfg->Write(tmp + _T("/macros/") + CommandTypeDescriptions[i], m_Commands[i], true);
    }

    // switches
    cfg->Write(tmp + _T("/switches/includes"), m_Switches.includeDirs, true);
    cfg->Write(tmp + _T("/switches/libs"), m_Switches.libDirs, true);
    cfg->Write(tmp + _T("/switches/link"), m_Switches.linkLibs, true);
    cfg->Write(tmp + _T("/switches/define"), m_Switches.defines, true);
    cfg->Write(tmp + _T("/switches/generic"), m_Switches.genericSwitch, true);
    cfg->Write(tmp + _T("/switches/objectext"), m_Switches.objectExtension, true);
    cfg->Write(tmp + _T("/switches/deps"), m_Switches.needDependencies);
    cfg->Write(tmp + _T("/switches/forceCompilerQuotes"), m_Switches.forceCompilerUseQuotes);
    cfg->Write(tmp + _T("/switches/forceLinkerQuotes"), m_Switches.forceLinkerUseQuotes);
    cfg->Write(tmp + _T("/switches/logging"), m_Switches.logging);
    cfg->Write(tmp + _T("/switches/buildMethod"), m_Switches.buildMethod);
    cfg->Write(tmp + _T("/switches/libPrefix"), m_Switches.libPrefix, true);
    cfg->Write(tmp + _T("/switches/libExtension"), m_Switches.libExtension, true);
    cfg->Write(tmp + _T("/switches/linkerNeedsLibPrefix"), m_Switches.linkerNeedsLibPrefix);
    cfg->Write(tmp + _T("/switches/linkerNeedsLibExtension"), m_Switches.linkerNeedsLibExtension);

    // regexes
    cfg->DeleteSubPath(tmp + _T("/regex"));
    wxString group;
    for (size_t i = 0; i < m_RegExes.Count(); ++i)
    {
        group.Printf(_T("%s/regex/re%3.3d"), tmp.c_str(), i + 1);
        RegExStruct& rs = m_RegExes[i];
        cfg->Write(group + _T("/description"), rs.desc, true);
        if (rs.lt != 0)
            cfg->Write(group + _T("/type"), rs.lt);
            cfg->Write(group + _T("/regex"), rs.regex, true);
        if (rs.msg[0] != 0)
            cfg->Write(group + _T("/msg1"), rs.msg[0]);
        if (rs.msg[1] != 0)
            cfg->Write(group + _T("/msg2"), rs.msg[1]);
        if (rs.msg[2] != 0)
            cfg->Write(group + _T("/msg3"), rs.msg[2]);
        if (rs.filename != 0)
            cfg->Write(group + _T("/filename"), rs.filename);
        if (rs.line != 0)
            cfg->Write(group + _T("/line"), rs.line);
    }

    // custom vars
    wxString configpath = tmp + _T("/custom_variables/");
	cfg->DeleteSubPath(configpath);
    const StringHash& v = GetAllVars();
    for (StringHash::const_iterator it = v.begin(); it != v.end(); ++it)
    {
        cfg->Write(configpath + it->first, it->second);
    }
}

void Compiler::LoadSettings(const wxString& baseKey)
{
    ConfigManager* cfg = Manager::Get()->GetConfigManager(_T("compiler"));

    wxString tmp;
    tmp.Printf(_T("%s/set%3.3d"), baseKey.c_str(), (int)m_ID);
    if (!cfg->Exists(tmp + _T("/name")))
        return;

    wxString sep = wxFileName::GetPathSeparator();

    if (m_ID > 255) // name changes are allowed only for user compilers
        m_Name = cfg->Read(tmp + _T("/name"), m_Name);

    m_MasterPath = cfg->Read(tmp + _T("/master_path"), m_MasterPath);
    m_ExtraPaths = GetArrayFromString(cfg->Read(tmp + _T("/extra_paths"), _T("")), _T(";"));
    m_Programs.C = cfg->Read(tmp + _T("/c_compiler"), m_Programs.C);
    m_Programs.CPP = cfg->Read(tmp + _T("/cpp_compiler"), m_Programs.CPP);
    m_Programs.LD = cfg->Read(tmp + _T("/linker"), m_Programs.LD);
    m_Programs.LIB = cfg->Read(tmp + _T("/lib_linker"), m_Programs.LIB);
    m_Programs.WINDRES = cfg->Read(tmp + _T("/res_compiler"), m_Programs.WINDRES);
    m_Programs.MAKE = cfg->Read(tmp + _T("/make"), m_Programs.MAKE);
    m_Programs.DBG = cfg->Read(tmp + _T("/debugger"), m_Programs.DBG);

    SetCompilerOptions(GetArrayFromString(cfg->Read(tmp + _T("/compiler_options"), wxEmptyString)));
    SetLinkerOptions(GetArrayFromString(cfg->Read(tmp + _T("/linker_options"), wxEmptyString)));
    SetIncludeDirs(GetArrayFromString(cfg->Read(tmp + _T("/include_dirs"), m_MasterPath + sep + _T("include"))));
    SetResourceIncludeDirs(GetArrayFromString(cfg->Read(tmp + _T("/res_include_dirs"), m_MasterPath + sep + _T("include"))));
    SetLibDirs(GetArrayFromString(cfg->Read(tmp + _T("/library_dirs"), m_MasterPath + sep + _T("lib"))));
    SetLinkLibs(GetArrayFromString(cfg->Read(tmp + _T("/libraries"), _T(""))));
    SetCommandsBeforeBuild(GetArrayFromString(cfg->Read(tmp + _T("/commands_before"), wxEmptyString)));
    SetCommandsAfterBuild(GetArrayFromString(cfg->Read(tmp + _T("/commands_after"), wxEmptyString)));

    for (int i = 0; i < COMPILER_COMMAND_TYPES_COUNT; ++i)
    {
        m_Commands[i] = cfg->Read(tmp + _T("/macros/") + CommandTypeDescriptions[i], m_Commands[i]);
    }

    // switches
    m_Switches.includeDirs = cfg->Read(tmp + _T("/switches/includes"), m_Switches.includeDirs);
    m_Switches.libDirs = cfg->Read(tmp + _T("/switches/libs"), m_Switches.libDirs);
    m_Switches.linkLibs = cfg->Read(tmp + _T("/switches/link"), m_Switches.linkLibs);
    m_Switches.defines = cfg->Read(tmp + _T("/switches/define"), m_Switches.defines);
    m_Switches.genericSwitch = cfg->Read(tmp + _T("/switches/generic"), m_Switches.genericSwitch);
    m_Switches.objectExtension = cfg->Read(tmp + _T("/switches/objectext"), m_Switches.objectExtension);
    m_Switches.needDependencies = cfg->ReadBool(tmp + _T("/switches/deps"), m_Switches.needDependencies);
    m_Switches.forceCompilerUseQuotes = cfg->ReadBool(tmp + _T("/switches/forceCompilerQuotes"), m_Switches.forceCompilerUseQuotes);
    m_Switches.forceLinkerUseQuotes = cfg->ReadBool(tmp + _T("/switches/forceLinkerQuotes"), m_Switches.forceLinkerUseQuotes);
    m_Switches.logging = (CompilerLoggingType)cfg->ReadInt(tmp + _T("/switches/logging"), m_Switches.logging);
    m_Switches.buildMethod = (CompilerBuildMethod)cfg->ReadInt(tmp + _T("/switches/buildMethod"), m_Switches.buildMethod);
    m_Switches.libPrefix = cfg->Read(tmp + _T("/switches/libPrefix"), m_Switches.libPrefix);
    m_Switches.libExtension = cfg->Read(tmp + _T("/switches/libExtension"), m_Switches.libExtension);
    m_Switches.linkerNeedsLibPrefix = cfg->ReadBool(tmp + _T("/switches/linkerNeedsLibPrefix"), m_Switches.linkerNeedsLibPrefix);
    m_Switches.linkerNeedsLibExtension = cfg->ReadBool(tmp + _T("/switches/linkerNeedsLibExtension"), m_Switches.linkerNeedsLibExtension);

    // regexes
    wxString group;
    int index = 1;
    bool cleared = false;
    while (true)
    {
        group.Printf(_T("%s/regex/re%3.3d"), tmp.c_str(), index++);
        if (!cfg->Exists(group+_T("/description")))
            break;
        else if (!cleared)
        {
            cleared = true;
            m_RegExes.Clear();
        }
        RegExStruct rs;
        rs.desc = cfg->Read(group + _T("/description"));
        rs.lt = (CompilerLineType)cfg->ReadInt(group + _T("/type"), 0);
        rs.regex = cfg->Read(group + _T("/regex"));
        rs.msg[0] = cfg->ReadInt(group + _T("/msg1"), 0);
        rs.msg[1] = cfg->ReadInt(group + _T("/msg2"), 0);
        rs.msg[2] = cfg->ReadInt(group + _T("/msg3"), 0);
        rs.filename = cfg->ReadInt(group + _T("/filename"), 0);
        rs.line = cfg->ReadInt(group + _T("/line"), 0);
        m_RegExes.Add(rs);
    }

    // custom vars
    wxString configpath = tmp + _T("/custom_variables/");
	UnsetAllVars();
	wxArrayString list = cfg->EnumerateKeys(configpath);
	for (unsigned int i = 0; i < list.GetCount(); ++i)
		SetVar(list[i], cfg->Read(configpath + _T('/') + list[i]), false);
}

CompilerLineType Compiler::CheckForWarningsAndErrors(const wxString& line)
{
    m_ErrorFilename.Clear();
    m_ErrorLine.Clear();
    m_Error.Clear();

    for (size_t i = 0; i < m_RegExes.Count(); ++i)
    {
        RegExStruct& rs = m_RegExes[i];
        if (rs.regex.IsEmpty())
            continue;
        wxRegEx regex(rs.regex);
        if (regex.Matches(line))
        {
            if (rs.filename > 0)
                 m_ErrorFilename = UnixFilename(regex.GetMatch(line, rs.filename));
            if (rs.line > 0)
                m_ErrorLine = regex.GetMatch(line, rs.line);
            for (int x = 0; x < 3; ++x)
            {
                if (rs.msg[x] > 0)
                {
                    if (!m_Error.IsEmpty())
                        m_Error << _T(" ");
                    m_Error << regex.GetMatch(line, rs.msg[x]);
                }
            }
            return rs.lt;
        }
    }
    return cltNormal; // default return value
}
