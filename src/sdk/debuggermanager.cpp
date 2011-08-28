/*
 * This file is part of the Code::Blocks IDE and licensed under the GNU Lesser General Public License, version 3
 * http://www.gnu.org/licenses/lgpl-3.0.html
 *
 * $Revision$
 * $Id$
 * $HeadURL$
 */

#include "sdk_precomp.h"
#ifndef CB_PRECOMP
    #include <wx/artprov.h>
    #include <wx/bmpbuttn.h>
    #include <wx/combobox.h>
    #include <wx/filedlg.h>
    #include <wx/frame.h>
    #include <wx/menu.h>
    #include <wx/settings.h>
    #include <wx/sizer.h>
    #include <wx/stattext.h>
    #include <wx/regex.h>

    #include "cbeditor.h"
    #include "cbexception.h"
    #include "cbplugin.h"
    #include "cbproject.h"
    #include "compilerfactory.h"
    #include "configmanager.h"
    #include "editormanager.h"
    #include "logmanager.h"
    #include "projectmanager.h"
#endif

#include <algorithm>
#include <wx/toolbar.h>

#include "debuggermanager.h"

#include "annoyingdialog.h"
#include "cbdebugger_interfaces.h"
#include "loggers.h"
#include "manager.h"

cbBreakpoint::cbBreakpoint() :
    m_line(0),
    m_ignoreCount(0),
    m_type(Code),
    m_enabled(true),
    m_useIgnoreCount(false),
    m_useCondition(false)
{
}
cbBreakpoint::cbBreakpoint(const wxString &filename, int line) :
    m_filename(filename),
    m_line(line),
    m_ignoreCount(0),
    m_type(Code),
    m_enabled(true),
    m_useIgnoreCount(false),
    m_useCondition(false)
{
}

cbBreakpoint::cbBreakpoint(const wxString &dataExpression, bool breakOnRead, bool breakOnWrite) :
    m_type(Data),
    m_dataExpression(dataExpression),
    m_breakOnRead(breakOnRead),
    m_breakOnWrite(breakOnWrite)
{
}

void cbBreakpoint::SetLine(int line)
{
    m_line = line;
}

void cbBreakpoint::SetCondition(wxString const &condition)
{
    m_condition = condition;
}

void cbBreakpoint::SetIgnoreCount(int count)
{
    m_ignoreCount = count;
}

void cbBreakpoint::SetEnabled(bool flag)
{
    m_enabled = flag;
}

void cbBreakpoint::SetUseIgnoreCount(bool flag)
{
    m_useIgnoreCount = flag;
}

void cbBreakpoint::SetUseCondition(bool flag)
{
    m_useCondition = flag;
}

const wxString & cbBreakpoint::GetFilename() const
{
    return m_filename;
}

const wxString & cbBreakpoint::GetCondition() const
{
    return m_condition;
}

int cbBreakpoint::GetLine() const
{
    return m_line;
}

int cbBreakpoint::GetIgnoreCount() const
{
    return m_ignoreCount;
}

cbBreakpoint::Type cbBreakpoint::GetType() const
{
    return m_type;
}

bool cbBreakpoint::IsEnabled() const
{
    return m_enabled;
}

bool cbBreakpoint::UseIgnoreCount() const
{
    return m_useIgnoreCount;
}

bool cbBreakpoint::UseCondition() const
{
    return m_useCondition;
}

const wxString& cbBreakpoint::GetDataExpression() const
{
    return m_dataExpression;
}

bool cbBreakpoint::GetBreakOnRead() const
{
    return m_breakOnRead;
}

bool cbBreakpoint::GetBreakOnWrite() const
{
    return m_breakOnWrite;
}

cbWatch::cbWatch() :
    m_parent(NULL),
    m_changed(true),
    m_removed(false),
    m_expanded(false)
{
}

cbWatch::~cbWatch()
{
    for(PtrContainer::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->Destroy();
    m_children.clear();
}

void cbWatch::Destroy()
{
    if (this)
        DoDestroy();
}

void cbWatch::AddChild(cbWatch *watch)
{
    watch->SetParent(this);
    m_children.push_back(watch);
}

void cbWatch::RemoveChild(int index)
{
    PtrContainer::iterator it = m_children.begin();
    std::advance(it, index);
    m_children.erase(it);
}

bool TestIfMarkedForRemoval(cbWatch *watch)
{
    if(watch->IsRemoved())
    {
        watch->Destroy();
        return true;
    }
    else
    {
        watch->RemoveMarkedChildren();
        return false;
    }
}

bool cbWatch::RemoveMarkedChildren()
{
    size_t start_size = m_children.size();
    PtrContainer::iterator new_last = std::remove_if(m_children.begin(), m_children.end(), &TestIfMarkedForRemoval);
    m_children.erase(new_last, m_children.end());

    return start_size != m_children.size();

}
void cbWatch::RemoveChildren()
{
    for(PtrContainer::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->Destroy();
    m_children.clear();
}

int cbWatch::GetChildCount() const
{
    return m_children.size();
}

cbWatch* cbWatch::GetChild(int index)
{
    PtrContainer::iterator it = m_children.begin();
    std::advance(it, index);
    return *it;
}

const cbWatch* cbWatch::GetChild(int index) const
{
    PtrContainer::const_iterator it = m_children.begin();
    std::advance(it, index);
    return *it;
}

cbWatch* cbWatch::FindChild(const wxString& symbol)
{
    for (PtrContainer::iterator it = m_children.begin(); it != m_children.end(); ++it)
    {
        wxString s;
        (*it)->GetSymbol(s);
        if(s == symbol)
            return *it;
    }
    return NULL;
}

int cbWatch::FindChildIndex(const wxString& symbol) const
{
    int index = 0;
    for (PtrContainer::const_iterator it = m_children.begin(); it != m_children.end(); ++it, ++index)
    {
        wxString s;
        (*it)->GetSymbol(s);
        if(s == symbol)
            return index;
    }
    return -1;
}

void cbWatch::SetParent(cbWatch *parent)
{
    m_parent = parent;
}

const cbWatch* cbWatch::GetParent() const
{
    return m_parent;
}

cbWatch* cbWatch::GetParent()
{
    return m_parent;
}

bool cbWatch::IsRemoved() const
{
    return m_removed;
}

bool cbWatch::IsChanged() const
{
    return m_changed;
}

void cbWatch::MarkAsRemoved(bool flag)
{
    m_removed = flag;
}

void cbWatch::MarkChildsAsRemoved()
{
    for(PtrContainer::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->MarkAsRemoved(true);
}
void cbWatch::MarkAsChanged(bool flag)
{
    m_changed = flag;
}

void cbWatch::MarkAsChangedRecursive(bool flag)
{
    m_changed = flag;
    for(PtrContainer::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->MarkAsChangedRecursive(flag);
}

bool cbWatch::IsExpanded() const
{
    return m_expanded;
}

void cbWatch::Expand(bool expand)
{
    m_expanded = expand;
}

cbWatch* DLLIMPORT cbGetRootWatch(cbWatch *watch)
{
    cbWatch *root = watch;
    while(root && root->GetParent())
        root = root->GetParent();
    return root;
}

cbStackFrame::cbStackFrame() :
    m_valid(false)
{
}

void cbStackFrame::SetNumber(int number)
{
    m_number = number;
}

void cbStackFrame::SetAddress(unsigned long int address)
{
    m_address = address;
}

void cbStackFrame::SetSymbol(const wxString& symbol)
{
    m_symbol = symbol;
}

void cbStackFrame::SetFile(const wxString& filename, const wxString &line)
{
    m_file = filename;
    m_line = line;
}

void cbStackFrame::MakeValid(bool flag)
{
    m_valid = flag;
}

int cbStackFrame::GetNumber() const
{
    return m_number;
}

unsigned long int cbStackFrame::GetAddress() const
{
    return m_address;
}

const wxString& cbStackFrame::GetSymbol() const
{
    return m_symbol;
}

const wxString& cbStackFrame::GetFilename() const
{
    return m_file;
}

const wxString& cbStackFrame::GetLine() const
{
    return m_line;
}

bool cbStackFrame::IsValid() const
{
    return m_valid;
}

cbThread::cbThread()
{
}

cbThread::cbThread(bool active, int number, const wxString& info)
{
    m_active = active;
    m_number = number;
    m_info = info;
}

bool cbThread::IsActive() const
{
    return m_active;
}

int cbThread::GetNumber() const
{
    return m_number;
}

const wxString& cbThread::GetInfo() const
{
    return m_info;
}

cbDebuggerConfiguration::cbDebuggerConfiguration(const ConfigManagerWrapper &config) :
    m_config(config),
    m_menuId(wxID_ANY)
{
}

cbDebuggerConfiguration::cbDebuggerConfiguration(const cbDebuggerConfiguration &o) :
    m_config(o.m_config),
    m_name(o.m_name)
{
}

void cbDebuggerConfiguration::SetName(const wxString &name)
{
    m_name = name;
}
const wxString& cbDebuggerConfiguration::GetName() const
{
    return m_name;
}

const ConfigManagerWrapper& cbDebuggerConfiguration::GetConfig() const
{
    return m_config;
}

void cbDebuggerConfiguration::SetConfig(const ConfigManagerWrapper &config)
{
    m_config = config;
}

void cbDebuggerConfiguration::SetMenuId(long id)
{
    m_menuId = id;
}

long cbDebuggerConfiguration::GetMenuId() const
{
    return m_menuId;
}

bool cbDebuggerCommonConfig::GetFlag(Flags flag)
{
    ConfigManager *c = Manager::Get()->GetConfigManager(wxT("debugger_common"));
    switch (flag)
    {
        case AutoBuild:
            return c->ReadBool(wxT("/common/auto_build"), true);
        case AutoSwitchFrame:
            return c->ReadBool(wxT("/common/auto_switch_frame"), true);
        case ShowDebuggersLog:
            return c->ReadBool(wxT("/common/debug_log"), false);
        case JumpOnDoubleClick:
            return c->ReadBool(wxT("/common/jump_on_double_click"), false);
        case RequireCtrlForTooltips:
            return c->ReadBool(wxT("/common/require_ctrl_for_tooltips"), false);
        default:
            return false;
    }
}

void cbDebuggerCommonConfig::SetFlag(Flags flag, bool value)
{
    ConfigManager *c = Manager::Get()->GetConfigManager(wxT("debugger_common"));
    switch (flag)
    {
        case AutoBuild:
            c->Write(wxT("/common/auto_build"), value);
            break;
        case AutoSwitchFrame:
            c->Write(wxT("/common/auto_switch_frame"), value);
            break;
        case ShowDebuggersLog:
            c->Write(wxT("/common/debug_log"), value);
            break;
        case JumpOnDoubleClick:
            c->Write(wxT("/common/jump_on_double_click"), value);
            break;
        case RequireCtrlForTooltips:
            c->Write(wxT("/common/require_ctrl_for_tooltips"), value);
            break;
        default:
            ;
    }
}

wxString cbDebuggerCommonConfig::GetValueTooltipFont()
{
    wxFont system = wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT);
    system.SetPointSize(std::max(system.GetPointSize() - 3, 7));
    wxString defaultFont = system.GetNativeFontInfo()->ToString();

    ConfigManager *c = Manager::Get()->GetConfigManager(wxT("debugger_common"));
    wxString configFont = c->Read(wxT("/common/tooltip_font"));

    return configFont.empty() ? defaultFont : configFont;
}

void cbDebuggerCommonConfig::SetValueTooltipFont(const wxString &font)
{
    const wxString &oldFont = GetValueTooltipFont();

    if (font != oldFont && !font.empty())
    {
        ConfigManager *c = Manager::Get()->GetConfigManager(wxT("debugger_common"));
        c->Write(wxT("/common/tooltip_font"), font);
    }
}

wxString cbDetectDebuggerExecutable(const wxString &exeName)
{
    wxString exeExt(platform::windows ? wxT(".exe") : wxEmptyString);
    wxString exePath = cbFindFileInPATH(exeName);
    wxChar sep = wxFileName::GetPathSeparator();

    if (exePath.empty())
    {
        if (!platform::windows)
            exePath = wxT("/usr/bin/") + exeName + exeExt;
        else
        {
            const wxString &cbInstallFolder = ConfigManager::GetExecutableFolder();
            if (wxFileExists(cbInstallFolder + sep + wxT("MINGW") + sep + wxT("bin") + sep + exeName + exeExt))
                exePath = cbInstallFolder + sep + wxT("MINGW") + sep + wxT("bin");
            else
            {
                exePath = wxT("C:\\MinGW\\bin");
                if (!wxDirExists(exePath))
                    exePath = wxT("C:\\MinGW32\\bin");
            }
        }
    }
    if (!wxDirExists(exePath))
        return wxEmptyString;
    return exePath + wxFileName::GetPathSeparator() + exeName + exeExt;
}


class DebugTextCtrlLogger : public TextCtrlLogger
{
public:
    DebugTextCtrlLogger(bool fixedPitchFont, bool debugLog) :
        TextCtrlLogger(fixedPitchFont),
        m_panel(NULL),
        m_debugLog(debugLog)
    {
    }

    wxWindow* CreateTextCtrl(wxWindow *parent)
    {
        return TextCtrlLogger::CreateControl(parent);
    }

    virtual wxWindow* CreateControl(wxWindow* parent);

private:
    wxPanel *m_panel;
    bool    m_debugLog;
};

class DebugLogPanel : public wxPanel
{
public:
    DebugLogPanel(wxWindow *parent, DebugTextCtrlLogger *text_control_logger, bool debug_log) :
        wxPanel(parent),
        m_text_control_logger(text_control_logger),
        m_debug_log(debug_log)
    {
        int idDebug_LogEntryControl = wxNewId();
        int idDebug_ExecuteButton = wxNewId();
        int idDebug_ClearButton = wxNewId();
        int idDebug_LoadButton = wxNewId();

        wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer *control_sizer = new wxBoxSizer(wxHORIZONTAL);

        wxWindow *text_control = text_control_logger->CreateTextCtrl(this);
        sizer->Add(text_control, wxEXPAND, wxEXPAND | wxALL , 0);
        sizer->Add(control_sizer, 0, wxEXPAND | wxALL, 0);

        wxStaticText *label = new wxStaticText(this, wxID_ANY, _T("Command:"),
                                               wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE);

        m_command_entry = new wxComboBox(this, idDebug_LogEntryControl, wxEmptyString,
                                         wxDefaultPosition, wxDefaultSize, 0, 0,
                                         wxCB_DROPDOWN | wxTE_PROCESS_ENTER);

        wxBitmap execute_bitmap = wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_EXECUTABLE_FILE")),
                                                           wxART_BUTTON);
        wxBitmap clear_bitmap = wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_DELETE")),wxART_BUTTON);
        wxBitmap file_open_bitmap =wxArtProvider::GetBitmap(wxART_MAKE_ART_ID_FROM_STR(_T("wxART_FILE_OPEN")),
                                                            wxART_BUTTON);

        wxBitmapButton *button_execute;
        button_execute = new wxBitmapButton(this, idDebug_ExecuteButton, execute_bitmap, wxDefaultPosition,
                                            wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator,
                                            _T("idDebug_ExecuteButton"));
        button_execute->SetToolTip(_("Execute current command"));

        wxBitmapButton *button_load = new wxBitmapButton(this, idDebug_LoadButton, file_open_bitmap, wxDefaultPosition,
                                                         wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator,
                                                         _T("idDebug_LoadButton"));
        button_load->SetDefault();
        button_load->SetToolTip(_("Load from file"));

        wxBitmapButton *button_clear = new wxBitmapButton(this, idDebug_ClearButton, clear_bitmap, wxDefaultPosition,
                                                          wxDefaultSize, wxBU_AUTODRAW, wxDefaultValidator,
                                                          _T("idDebug_ClearButton"));
        button_clear->SetDefault();
        button_clear->SetToolTip(_("Clear output window"));

        control_sizer->Add(label, 0, wxALIGN_CENTER | wxALL, 2);
        control_sizer->Add(m_command_entry, wxEXPAND, wxEXPAND | wxALL, 2);
        control_sizer->Add(button_execute, 0, wxEXPAND | wxALL, 0);
        control_sizer->Add(button_load, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);
        control_sizer->Add(button_clear, 0, wxALL|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 0);

        SetSizer(sizer);

        Connect(idDebug_LogEntryControl,
                wxEVT_COMMAND_TEXT_ENTER,
                wxObjectEventFunction(&DebugLogPanel::OnEntryCommand));
        Connect(idDebug_ExecuteButton,
                wxEVT_COMMAND_BUTTON_CLICKED,
                wxObjectEventFunction(&DebugLogPanel::OnEntryCommand));
        Connect(idDebug_ClearButton,
                wxEVT_COMMAND_BUTTON_CLICKED,
                wxObjectEventFunction(&DebugLogPanel::OnClearLog));
        Connect(idDebug_LoadButton,
                wxEVT_COMMAND_BUTTON_CLICKED,
                wxObjectEventFunction(&DebugLogPanel::OnLoadFile));

        // UpdateUI events
        Connect(idDebug_ExecuteButton,
                wxEVT_UPDATE_UI,
                wxObjectEventFunction(&DebugLogPanel::OnUpdateUI));
        Connect(idDebug_LoadButton,
                wxEVT_UPDATE_UI,
                wxObjectEventFunction(&DebugLogPanel::OnUpdateUI));
        Connect(idDebug_LogEntryControl,
                wxEVT_UPDATE_UI,
                wxObjectEventFunction(&DebugLogPanel::OnUpdateUI));
    }

    void OnEntryCommand(wxCommandEvent& event)
    {
        assert(m_command_entry);
        wxString cmd = m_command_entry->GetValue();
        cmd.Trim(false);
        cmd.Trim(true);

        if (cmd.IsEmpty())
            return;
        cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
        if (plugin)
        {
            plugin->SendCommand(cmd, m_debug_log);

            //If it already exists in the list, remove it and add it back at the end
            int index = m_command_entry->FindString(cmd);
            if (index != wxNOT_FOUND)
                m_command_entry->Delete(index);
            m_command_entry->Append(cmd);

            m_command_entry->SetValue(wxEmptyString);
        }
    }

    void OnClearLog(wxCommandEvent& event)
    {
        assert(m_command_entry);
        assert(m_text_control_logger);
        m_text_control_logger->Clear();
        m_command_entry->SetFocus();
    }

    void OnLoadFile(wxCommandEvent& event)
    {
        cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
        if (!plugin)
            return;

        ConfigManager* manager = Manager::Get()->GetConfigManager(_T("app"));
        wxString path = manager->Read(_T("/file_dialogs/file_run_dbg_script/directory"), wxEmptyString);

        wxFileDialog dialog(this, _("Load script"), path, wxEmptyString,
                            _T("Debugger script files (*.gdb)|*.gdb"), wxFD_OPEN | compatibility::wxHideReadonly);

        if (dialog.ShowModal() == wxID_OK)
        {
            manager->Write(_T("/file_dialogs/file_run_dbg_script/directory"), dialog.GetDirectory());

            plugin->SendCommand(_T("source ") + dialog.GetPath(), m_debug_log);
        }
    }

    void OnUpdateUI(wxUpdateUIEvent &event)
    {
        cbDebuggerPlugin *plugin = Manager::Get()->GetDebuggerManager()->GetActiveDebugger();
        event.Enable(plugin && plugin->IsRunning() && plugin->IsStopped());
    }
private:
    DebugTextCtrlLogger *m_text_control_logger;
    wxComboBox  *m_command_entry;
    bool m_debug_log;
};

wxWindow* DebugTextCtrlLogger::CreateControl(wxWindow* parent)
{
    if(!m_panel)
        m_panel = new DebugLogPanel(parent, this, m_debugLog);

    return m_panel;
}

template<> DebuggerManager* Mgr<DebuggerManager>::instance = 0;
template<> bool  Mgr<DebuggerManager>::isShutdown = false;

void ReadActiveDebuggerConfig(wxString &name, int &configIndex)
{
    ConfigManager &config = *Manager::Get()->GetConfigManager(_T("debugger_common"));
    name = config.Read(wxT("active_debugger"), wxEmptyString);
    if (name.empty())
        configIndex = -1;
    else
        configIndex = std::max(0, config.ReadInt(wxT("active_debugger_config"), 0));
}

void WriteActiveDebuggerConfig(const wxString &name, int configIndex)
{
    ConfigManager &configMgr = *Manager::Get()->GetConfigManager(_T("debugger_common"));
    configMgr.Write(wxT("active_debugger"), name);
    configMgr.Write(wxT("active_debugger_config"), configIndex);
}

cbDebuggerConfiguration* DebuggerManager::PluginData::GetConfiguration(int index)
{
    if (m_configurations.empty())
        cbAssert(false);
    if (index >= static_cast<int>(m_configurations.size()))
        return nullptr;
    else
        return m_configurations[index];
}

DebuggerManager::DebuggerManager() :
    m_interfaceFactory(nullptr),
    m_activeDebugger(NULL),
    m_menuHandler(nullptr),
    m_backtraceDialog(NULL),
    m_breakPointsDialog(NULL),
    m_cpuRegistersDialog(NULL),
    m_disassemblyDialog(NULL),
    m_examineMemoryDialog(NULL),
    m_threadsDialog(NULL),
    m_watchesDialog(NULL),
    m_logger(NULL),
    m_debugLogger(NULL),
    m_loggerIndex(-1),
    m_debugLoggerIndex(-1),
    m_isDisassemblyMixedMode(false),
    m_useTargetsDefault(false)
{
    typedef cbEventFunctor<DebuggerManager, CodeBlocksEvent> Event;
    Manager::Get()->RegisterEventSink(cbEVT_PROJECT_ACTIVATE, new Event(this, &DebuggerManager::OnProjectActivated));
    Manager::Get()->RegisterEventSink(cbEVT_BUILDTARGET_SELECTED, new Event(this, &DebuggerManager::OnTargetSelected));
    Manager::Get()->RegisterEventSink(cbEVT_SETTINGS_CHANGED, new Event(this, &DebuggerManager::OnSettingsChanged));
    Manager::Get()->RegisterEventSink(cbEVT_PLUGIN_LOADING_COMPLETE,
                                      new Event(this, &DebuggerManager::OnPluginLoadingComplete));

    wxString activeDebuggerName;
    int activeConfig;
    ReadActiveDebuggerConfig(activeDebuggerName, activeConfig);
    if (activeDebuggerName.empty() && activeConfig == -1)
        m_useTargetsDefault = true;
}

DebuggerManager::~DebuggerManager()
{
    for (RegisteredPlugins::iterator it = m_registered.begin(); it != m_registered.end(); ++it)
        it->second.ClearConfigurations();

    Manager::Get()->RemoveAllEventSinksFor(this);
    delete m_interfaceFactory;
}

bool DebuggerManager::RegisterDebugger(cbDebuggerPlugin *plugin, const wxString &guiName, const wxString &settingsName)
{
    RegisteredPlugins::iterator it = m_registered.find(plugin);
    if (it != m_registered.end())
        return false;

    wxRegEx regExSettingsName(wxT("^[a-z_][a-z0-9_]+$"));
    if (!regExSettingsName.Matches(settingsName))
    {
        wxString s;
        s = wxString::Format(_("The settings name for the debugger plugin \"%s\" - \"%s\" contains invalid characters"),
                             guiName.c_str(), settingsName.c_str());
        Manager::Get()->GetLogManager()->LogError(s);
        return false;
    }

    int normalIndex = -1, debugIndex = -1;
    GetLogger(false, normalIndex);
    if (cbDebuggerCommonConfig::GetFlag(cbDebuggerCommonConfig::ShowDebuggersLog))
        GetLogger(true, debugIndex);
    plugin->SetupLogs(normalIndex, debugIndex);

    PluginData data;
    data.m_guiName = guiName;
    data.m_settingsName = settingsName;

    m_registered[plugin] = data;
    it = m_registered.find(plugin);
    ProcessSettings(it);

    // There should be at least one configuration for every plugin.
    // If this is not the case, something is wrong and should be fixed.
    cbAssert(!it->second.GetConfigurations().empty());

    wxString activeDebuggerName;
    int activeConfig;
    ReadActiveDebuggerConfig(activeDebuggerName, activeConfig);

    if (activeDebuggerName == settingsName)
    {
        if (activeConfig > static_cast<int>(it->second.GetConfigurations().size()))
            activeConfig = 0;

        m_activeDebugger = plugin;
        m_activeDebugger->SetActiveConfig(activeConfig);

        m_menuHandler->SetActiveDebugger(m_activeDebugger);
    }

    m_menuHandler->RebuildActiveDebuggersMenu();

    return true;
}

bool DebuggerManager::UnregisterDebugger(cbDebuggerPlugin *plugin)
{
    RegisteredPlugins::iterator it = m_registered.find(plugin);
    if(it == m_registered.end())
        return false;

    m_registered.erase(it);
    if (plugin == m_activeDebugger)
    {
        if (m_registered.empty())
            m_activeDebugger = NULL;
        else
            m_activeDebugger = m_registered.begin()->first;
        m_menuHandler->SetActiveDebugger(m_activeDebugger);
    }
    m_menuHandler->RebuildActiveDebuggersMenu();

    if (m_registered.empty())
    {
        m_interfaceFactory->DeleteBacktrace(m_backtraceDialog);
        m_backtraceDialog = NULL;

        m_interfaceFactory->DeleteBreakpoints(m_breakPointsDialog);
        m_breakPointsDialog = NULL;

        m_interfaceFactory->DeleteCPURegisters(m_cpuRegistersDialog);
        m_cpuRegistersDialog = NULL;

        m_interfaceFactory->DeleteDisassembly(m_disassemblyDialog);
        m_disassemblyDialog = NULL;

        m_interfaceFactory->DeleteMemory(m_examineMemoryDialog);
        m_examineMemoryDialog = NULL;

        m_interfaceFactory->DeleteThreads(m_threadsDialog);
        m_threadsDialog = NULL;

        m_interfaceFactory->DeleteWatches(m_watchesDialog);
        m_watchesDialog = NULL;

        if (Manager::Get()->GetLogManager())
        {
            Manager::Get()->GetDebuggerManager()->HideLogger(true);
            Manager::Get()->GetDebuggerManager()->HideLogger(false);
        }
    }

    return true;
}

void DebuggerManager::ProcessSettings(RegisteredPlugins::iterator it)
{
    cbDebuggerPlugin *plugin = it->first;
    PluginData &data = it->second;
    ConfigManager *config = Manager::Get()->GetConfigManager(wxT("debugger_common"));
    wxString path = wxT("/sets/") + data.GetSettingsName();
    wxArrayString configs = config->EnumerateSubPaths(path);
    configs.Sort();

    if (configs.empty())
    {
        config->Write(path + wxT("/conf1/name"), wxString(wxT("Default")));
        configs = config->EnumerateSubPaths(path);
        configs.Sort();
    }

    data.ClearConfigurations();
    data.m_lastConfigID = -1;

    for (size_t jj = 0; jj < configs.Count(); ++jj)
    {
        wxString configPath = path + wxT("/") + configs[jj];
        wxString name = config->Read(configPath + wxT("/name"));

        cbDebuggerConfiguration *pluginConfig;
        pluginConfig = plugin->LoadConfig(ConfigManagerWrapper(wxT("debugger_common"), configPath + wxT("/values")));
        if (pluginConfig)
        {
            pluginConfig->SetName(name);
            data.GetConfigurations().push_back(pluginConfig);
        }
    }
}

ConfigManagerWrapper DebuggerManager::NewConfig(cbDebuggerPlugin *plugin, const wxString &name)
{
    RegisteredPlugins::iterator it = m_registered.find(plugin);
    if (it == m_registered.end())
        return ConfigManagerWrapper();

    wxString path = wxT("/sets/") + it->second.GetSettingsName();

    if (it->second.m_lastConfigID == -1)
    {
        ConfigManager *config = Manager::Get()->GetConfigManager(wxT("debugger_common"));
        wxArrayString configs = config->EnumerateSubPaths(path);
        for (size_t ii = 0; ii < configs.GetCount(); ++ii)
        {
            long id;
            if (configs[ii].Remove(0, 4).ToLong(&id))
                it->second.m_lastConfigID = std::max<long>(it->second.m_lastConfigID, id);
        }
    }

    path << wxT("/conf") << ++it->second.m_lastConfigID;

    return ConfigManagerWrapper(wxT("debugger_common"), path +  wxT("/values"));
}

void DebuggerManager::RebuildAllConfigs()
{
    for (RegisteredPlugins::iterator it = m_registered.begin(); it != m_registered.end(); ++it)
        ProcessSettings(it);
    m_menuHandler->RebuildActiveDebuggersMenu();
}

wxMenu* DebuggerManager::GetMenu()
{
    wxMenuBar *menuBar = Manager::Get()->GetAppFrame()->GetMenuBar();
    cbAssert(menuBar);
    wxMenu *menu = NULL;

    int menu_pos = menuBar->FindMenu(_("&Debug"));

    if(menu_pos != wxNOT_FOUND)
        menu = menuBar->GetMenu(menu_pos);

    if (!menu)
    {
        menu = Manager::Get()->LoadMenu(_T("debugger_menu"),true);

        // ok, now, where do we insert?
        // three possibilities here:
        // a) locate "Compile" menu and insert after it
        // b) locate "Project" menu and insert after it
        // c) if not found (?), insert at pos 5
        int finalPos = 5;
        int projcompMenuPos = menuBar->FindMenu(_("&Build"));
        if (projcompMenuPos == wxNOT_FOUND)
            projcompMenuPos = menuBar->FindMenu(_("&Compile"));

        if (projcompMenuPos != wxNOT_FOUND)
            finalPos = projcompMenuPos + 1;
        else
        {
            projcompMenuPos = menuBar->FindMenu(_("&Project"));
            if (projcompMenuPos != wxNOT_FOUND)
                finalPos = projcompMenuPos + 1;
        }
        menuBar->Insert(finalPos, menu, _("&Debug"));

        m_menuHandler->RebuildActiveDebuggersMenu();
    }
    return menu;
}

void DebuggerManager::BuildContextMenu(wxMenu &menu, const wxString& word_at_caret, bool is_running)
{
    m_menuHandler->BuildContextMenu(menu, word_at_caret, is_running);
}

TextCtrlLogger* DebuggerManager::GetLogger(bool for_debug, int &index)
{
    LogManager* msgMan = Manager::Get()->GetLogManager();

    if (for_debug)
    {
        if (!m_debugLogger)
        {
            m_debugLogger = new DebugTextCtrlLogger(true, true);
            m_debugLoggerIndex = msgMan->SetLog(m_debugLogger);
            LogSlot &slot = msgMan->Slot(m_debugLoggerIndex);

            slot.title = _("Debugger (debug)");
            // set log image
            wxString prefix = ConfigManager::GetDataFolder() + _T("/images/");
            wxBitmap* bmp = new wxBitmap(cbLoadBitmap(prefix + _T("contents_16x16.png"), wxBITMAP_TYPE_PNG));
            slot.icon = bmp;

            CodeBlocksLogEvent evtAdd(cbEVT_ADD_LOG_WINDOW, m_debugLogger, slot.title, slot.icon);
            Manager::Get()->ProcessEvent(evtAdd);
        }
        index = m_debugLoggerIndex;
        return m_debugLogger;
    }
    else
    {
        if (!m_logger)
        {
            m_logger = new DebugTextCtrlLogger(true, false);
            m_loggerIndex = msgMan->SetLog(m_logger);
            LogSlot &slot = msgMan->Slot(m_loggerIndex);
            slot.title = _("Debugger");
            // set log image
            wxString prefix = ConfigManager::GetDataFolder() + _T("/images/");
            wxBitmap* bmp = new wxBitmap(cbLoadBitmap(prefix + _T("misc_16x16.png"), wxBITMAP_TYPE_PNG));
            slot.icon = bmp;

            CodeBlocksLogEvent evtAdd(cbEVT_ADD_LOG_WINDOW, m_logger, slot.title, slot.icon);
            Manager::Get()->ProcessEvent(evtAdd);
        }

        index = m_loggerIndex;
        return m_logger;
    }
}

TextCtrlLogger* DebuggerManager::GetLogger(bool for_debug)
{
    int index;
    return GetLogger(for_debug, index);
}

void DebuggerManager::HideLogger(bool for_debug)
{
    if (for_debug)
    {
        CodeBlocksLogEvent evt(cbEVT_REMOVE_LOG_WINDOW, m_debugLogger);
        Manager::Get()->ProcessEvent(evt);
        m_debugLogger = NULL;
        m_debugLoggerIndex = -1;
    }
    else
    {
        CodeBlocksLogEvent evt(cbEVT_REMOVE_LOG_WINDOW, m_logger);
        Manager::Get()->ProcessEvent(evt);
        m_logger = NULL;
        m_loggerIndex = -1;
    }
}

void DebuggerManager::SetInterfaceFactory(cbDebugInterfaceFactory *factory)
{
    delete m_interfaceFactory;
    m_interfaceFactory = factory;
}

cbDebugInterfaceFactory* DebuggerManager::GetInterfaceFactory()
{
    return m_interfaceFactory;
}

void DebuggerManager::SetMenuHandler(cbDebuggerMenuHandler *handler)
{
    m_menuHandler = handler;
}

cbBacktraceDlg* DebuggerManager::GetBacktraceDialog()
{
    if (!m_backtraceDialog)
        m_backtraceDialog = m_interfaceFactory->CreateBacktrace();
    return m_backtraceDialog;
}

cbBreakpointsDlg* DebuggerManager::GetBreakpointDialog()
{
    if (!m_breakPointsDialog)
        m_breakPointsDialog = m_interfaceFactory->CreateBreapoints();
    return m_breakPointsDialog;
}

cbCPURegistersDlg* DebuggerManager::GetCPURegistersDialog()
{
    if (!m_cpuRegistersDialog)
        m_cpuRegistersDialog = m_interfaceFactory->CreateCPURegisters();
    return m_cpuRegistersDialog;
}

cbDisassemblyDlg* DebuggerManager::GetDisassemblyDialog()
{
    if (!m_disassemblyDialog)
        m_disassemblyDialog = m_interfaceFactory->CreateDisassembly();
    return m_disassemblyDialog;
}

cbExamineMemoryDlg* DebuggerManager::GetExamineMemoryDialog()
{
    if (!m_examineMemoryDialog)
        m_examineMemoryDialog = m_interfaceFactory->CreateMemory();
    return m_examineMemoryDialog;
}

cbThreadsDlg* DebuggerManager::GetThreadsDialog()
{
    if (!m_threadsDialog)
        m_threadsDialog = m_interfaceFactory->CreateThreads();
    return m_threadsDialog;
}

cbWatchesDlg* DebuggerManager::GetWatchesDialog()
{
    if (!m_watchesDialog)
        m_watchesDialog = m_interfaceFactory->CreateWatches();
    return m_watchesDialog;
}

bool DebuggerManager::ShowBacktraceDialog()
{
    cbBacktraceDlg *dialog = GetBacktraceDialog();

    if (!IsWindowReallyShown(dialog->GetWindow()))
    {
        // show the backtrace window
        CodeBlocksDockEvent evt(cbEVT_SHOW_DOCK_WINDOW);
        evt.pWindow = dialog->GetWindow();
        Manager::Get()->ProcessEvent(evt);
        return true;
    }
    else
        return false;
}

bool DebuggerManager::UpdateBacktrace()
{
    return m_backtraceDialog && IsWindowReallyShown(m_backtraceDialog->GetWindow());
}

bool DebuggerManager::UpdateCPURegisters()
{
    return m_cpuRegistersDialog && IsWindowReallyShown(m_cpuRegistersDialog->GetWindow());
}

bool DebuggerManager::UpdateDisassembly()
{
    return m_disassemblyDialog && IsWindowReallyShown(m_disassemblyDialog->GetWindow());
}

bool DebuggerManager::UpdateExamineMemory()
{
    return m_examineMemoryDialog && IsWindowReallyShown(m_examineMemoryDialog->GetWindow());
}

bool DebuggerManager::UpdateThreads()
{
    return m_threadsDialog && IsWindowReallyShown(m_threadsDialog->GetWindow());
}

cbDebuggerPlugin* DebuggerManager::GetDebuggerHavingWatch(cbWatch *watch)
{
    watch = cbGetRootWatch(watch);
    for (RegisteredPlugins::iterator it = m_registered.begin(); it != m_registered.end(); ++it)
    {
        if (it->first->HasWatch(watch))
            return it->first;
    }
    return NULL;
}

bool DebuggerManager::ShowValueTooltip(const cbWatch::Pointer &watch, const wxRect &rect)
{
    return m_interfaceFactory->ShowValueTooltip(watch, rect);
}

DebuggerManager::RegisteredPlugins const & DebuggerManager::GetAllDebuggers() const
{
    return m_registered;
}
DebuggerManager::RegisteredPlugins & DebuggerManager::GetAllDebuggers()
{
    return m_registered;
}
cbDebuggerPlugin* DebuggerManager::GetActiveDebugger()
{
    return m_activeDebugger;
}

void RefreshBreakpoints(const cbDebuggerPlugin *plugin)
{
    EditorManager *editorManager = Manager::Get()->GetEditorManager();
    int count = editorManager->GetEditorsCount();
    for (int ii = 0; ii < count; ++ii)
    {
        EditorBase *editor = editorManager->GetEditor(ii);
        if (!editor->IsBuiltinEditor())
            continue;
        editor->RefreshBreakpointMarkers(plugin);
    }
}

void DebuggerManager::SetActiveDebugger(cbDebuggerPlugin* activeDebugger, ConfigurationVector::iterator config)
{
    RegisteredPlugins::const_iterator it = m_registered.find(activeDebugger);
    cbAssert(it != m_registered.end());

    m_useTargetsDefault = false;
    m_activeDebugger = activeDebugger;
    int index = std::distance<ConfigurationVector::const_iterator>(it->second.GetConfigurations().begin(), config);
    m_activeDebugger->SetActiveConfig(index);

    m_menuHandler->SetActiveDebugger(activeDebugger);
    WriteActiveDebuggerConfig(it->second.GetSettingsName(), index);
    RefreshBreakpoints(activeDebugger);
}

bool DebuggerManager::IsActiveDebuggerTargetsDefault() const
{
    return m_activeDebugger && m_useTargetsDefault;
}

void DebuggerManager::SetTargetsDefaultAsActiveDebugger()
{
    m_activeDebugger = nullptr;
    m_menuHandler->SetActiveDebugger(nullptr);
    FindTargetsDebugger();
}

void DebuggerManager::FindTargetsDebugger()
{
    m_activeDebugger = nullptr;
    m_menuHandler->SetActiveDebugger(nullptr);

    if (m_registered.empty())
    {
        m_menuHandler->MarkActiveTargetAsValid(false);
        return;
    }

    ProjectManager* projectMgr = Manager::Get()->GetProjectManager();
    LogManager* log = Manager::Get()->GetLogManager();
    cbProject* project = projectMgr->GetActiveProject();
    ProjectBuildTarget *target = nullptr;
    if (project)
    {
        const wxString &targetName = project->GetActiveBuildTarget();
        if (project->BuildTargetValid(targetName))
            target = project->GetBuildTarget(targetName);
    }


    Compiler *compiler = nullptr;
    if (!target)
    {
        compiler = CompilerFactory::GetDefaultCompiler();
        if (!compiler)
        {
            log->LogError(_("Can't get the active target, nor default compiler!"));
            m_menuHandler->MarkActiveTargetAsValid(false);
            return;
        }
    }
    else
    {
        compiler = CompilerFactory::GetCompiler(target->GetCompilerID());
        if (!compiler)
        {
            log->LogError(_("Current target doesn't have valid compiler!"));
            m_menuHandler->MarkActiveTargetAsValid(false);
            return;
        }
    }
    wxString dbgString = compiler->GetPrograms().DBGconfig;
    wxString::size_type pos = dbgString.find(wxT(':'));

    wxString name, config;
    if (pos != wxString::npos)
    {
        name = dbgString.substr(0, pos);
        config = dbgString.substr(pos + 1, dbgString.length() - pos - 1);
    }

    if (name.empty() || config.empty())
    {
        log->LogError(_("Current compiler doesn't have correctly defined debugger!"));
        m_menuHandler->MarkActiveTargetAsValid(false);
        return;
    }

    for (RegisteredPlugins::iterator it = m_registered.begin(); it != m_registered.end(); ++it)
    {
        PluginData &data = it->second;
        if (data.GetSettingsName() == name)
        {
            ConfigurationVector &configs = data.GetConfigurations();
            int index = 0;
            for (ConfigurationVector::iterator itConf = configs.begin(); itConf != configs.end(); ++itConf, ++index)
            {
                if ((*itConf)->GetName() == config)
                {
                    m_activeDebugger = it->first;
                    m_activeDebugger->SetActiveConfig(index);
                    m_useTargetsDefault = true;

                    m_menuHandler->SetActiveDebugger(m_activeDebugger);
                    WriteActiveDebuggerConfig(wxEmptyString, -1);
                    RefreshBreakpoints(m_activeDebugger);
                    m_menuHandler->MarkActiveTargetAsValid(true);
                    return;
                }
            }
        }
    }

    log->LogError(wxString::Format(_("Can't find the debugger config: '%s:%s' for the current target!"),
                                   name.c_str(), config.c_str()));
    m_menuHandler->MarkActiveTargetAsValid(false);
}

bool DebuggerManager::IsDisassemblyMixedMode()
{
    return m_isDisassemblyMixedMode;
}

void DebuggerManager::SetDisassemblyMixedMode(bool mixed)
{
    m_isDisassemblyMixedMode = mixed;
}

void DebuggerManager::OnProjectActivated(CodeBlocksEvent& event)
{
    if (m_useTargetsDefault)
        FindTargetsDebugger();
}

void DebuggerManager::OnTargetSelected(CodeBlocksEvent& event)
{
    if (m_useTargetsDefault)
        FindTargetsDebugger();
}

void DebuggerManager::OnSettingsChanged(CodeBlocksEvent& event)
{
    if (event.GetInt() == cbSettingsType::Compiler || event.GetInt() == cbSettingsType::Debugger)
    {
        if (m_useTargetsDefault)
            FindTargetsDebugger();
    }
}

void DebuggerManager::OnPluginLoadingComplete(CodeBlocksEvent& event)
{
    if (!m_activeDebugger)
    {
        m_useTargetsDefault = true;
        FindTargetsDebugger();
    }
}
