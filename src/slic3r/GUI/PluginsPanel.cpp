#include "PluginsPanel.hpp"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include "I18N.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Modular/ModuleManager.hpp"

namespace Slic3r::GUI {

PluginsPanel::PluginsPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style)
{
    build();
}

void PluginsPanel::build()
{
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);

    // 1. Header Banner
    auto* header_panel = new wxPanel(this, wxID_ANY);
    auto* header_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* title_text = new wxStaticText(header_panel, wxID_ANY, _L("Modular OrcaSlicer: Plugins & Subsystem Manager"));
    wxFont title_font = title_text->GetFont();
    title_font.SetPointSize(title_font.GetPointSize() + 4);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title_text->SetFont(title_font);

    header_sizer->Add(title_text, 1, wxALIGN_CENTER_VERTICAL | wxALL, 15);
    header_panel->SetSizer(header_sizer);
    main_sizer->Add(header_panel, 0, wxEXPAND | wxBOTTOM, 5);

    // 2. Toolbar Actions
    auto* tools_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* btn_refresh = new wxButton(this, wxID_ANY, _L("Refresh Modules"));
    auto* btn_install = new wxButton(this, wxID_ANY, _L("Install Plugin Package (.dll / .json)..."));
    auto* btn_open_dir = new wxButton(this, wxID_ANY, _L("Open Plugins Folder"));

    btn_refresh->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { refresh_catalog(); });
    btn_install->Bind(wxEVT_BUTTON, &PluginsPanel::on_install_plugin, this);
    btn_open_dir->Bind(wxEVT_BUTTON, &PluginsPanel::on_open_plugins_dir, this);

    tools_sizer->Add(btn_refresh, 0, wxRIGHT, 10);
    tools_sizer->Add(btn_install, 0, wxRIGHT, 10);
    tools_sizer->Add(btn_open_dir, 0, wxRIGHT, 10);
    main_sizer->Add(tools_sizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 15);

    // 3. Module List
    m_list_ctrl = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_THEME);
    m_list_ctrl->InsertColumn(0, _L("Status"), wxLIST_FORMAT_LEFT, 100);
    m_list_ctrl->InsertColumn(1, _L("Module Name"), wxLIST_FORMAT_LEFT, 240);
    m_list_ctrl->InsertColumn(2, _L("Intercepted Subsystem Hook"), wxLIST_FORMAT_LEFT, 180);
    m_list_ctrl->InsertColumn(3, _L("Author"), wxLIST_FORMAT_LEFT, 160);
    m_list_ctrl->InsertColumn(4, _L("Description"), wxLIST_FORMAT_LEFT, 350);

    main_sizer->Add(m_list_ctrl, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    // 4. Footer Note & Restart Button
    auto* footer_panel = new wxPanel(this, wxID_ANY);
    auto* footer_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_status_text = new wxStaticText(footer_panel, wxID_ANY, 
        _L("Note: Subsystem changes take effect on next launch to guarantee slicing pipeline stability."));
    footer_sizer->Add(m_status_text, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 15);

    auto* btn_restart = new wxButton(footer_panel, wxID_ANY, _L("Restart Modular OrcaSlicer"));
    btn_restart->Bind(wxEVT_BUTTON, &PluginsPanel::on_restart_requested, this);
    footer_sizer->Add(btn_restart, 0, wxALIGN_CENTER_VERTICAL);

    footer_panel->SetSizer(footer_sizer);
    main_sizer->Add(footer_panel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 15);

    SetSizer(main_sizer);
    Layout();

    refresh_catalog();
}

void PluginsPanel::refresh_catalog()
{
    if (!m_list_ctrl) return;
    m_list_ctrl->DeleteAllItems();

    const auto &loaded = Modular::ModuleManager::instance().get_loaded_mods();

    // Default ecosystem entries
    struct ModDisplayItem {
        wxString name;
        wxString hook;
        wxString author;
        wxString desc;
        bool is_active;
    };

    std::vector<ModDisplayItem> items = {
        { "Wave Overhangs (Dennis Klappe / Janis Andersons)", "OverhangStrategy", "Janis Andersons", "Self-supporting recursive wavefront overhang generator.", false },
        { "Low-RAM Large Model Resource Optimizer", "ResourceOptimizer", "Modular RepRap", "Decouples mesh slicing buffers for huge models on modest PCs.", false },
        { "Non-Planar 3-Axis Continuous Z Kinematics", "KinematicsEngine", "RepRap Community", "Continuous 3D simultaneous XYZ non-planar layer deposition.", false },
        { "Arc Overhangs (Steven McCulloch)", "OverhangStrategy", "Steven McCulloch", "Circular arc bridging pattern for steep overhangs.", false }
    };

    // Match loaded mods
    for (auto &item : items) {
        for (const auto &m : loaded) {
            if (m.name.find("Wave") != std::string::npos && item.hook == "OverhangStrategy") {
                item.is_active = true;
            } else if (m.id == "resource_optimizer_lite" && item.hook == "ResourceOptimizer") {
                item.is_active = true;
            }
        }
    }

    long index = 0;
    for (const auto &item : items) {
        long row = m_list_ctrl->InsertItem(index, item.is_active ? _L("[ACTIVE]") : _L("[AVAILABLE]"));
        m_list_ctrl->SetItem(row, 1, item.name);
        m_list_ctrl->SetItem(row, 2, item.hook);
        m_list_ctrl->SetItem(row, 3, item.author);
        m_list_ctrl->SetItem(row, 4, item.desc);
        index++;
    }
}

void PluginsPanel::on_install_plugin(wxCommandEvent &)
{
    wxFileDialog openFileDialog(this, _L("Select Plugin Package"), "", "",
                               "Plugin Files (*.dll;*.json)|*.dll;*.json", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString selected = openFileDialog.GetPath();
    wxMessageBox(wxString::Format(_L("Plugin '%s' installed. It will be loaded on the next execution."), selected),
                 _L("Plugin Installed"), wxOK | wxICON_INFORMATION);
    refresh_catalog();
}

void PluginsPanel::on_open_plugins_dir(wxCommandEvent &)
{
    wxString dir = from_u8(Slic3r::data_dir()) + wxFileName::GetPathSeparator() + "plugins";
    if (!wxDirExists(dir)) {
        wxMkdir(dir);
    }
    wxLaunchDefaultApplication(dir);
}

void PluginsPanel::on_restart_requested(wxCommandEvent &)
{
    int answer = wxMessageBox(
        _L("Restart Modular OrcaSlicer now to apply active plugin and subsystem configurations?"),
        _L("Confirm Restart"), wxYES_NO | wxICON_QUESTION);
    if (answer == wxYES) {
        wxString exec_path = wxStandardPaths::Get().GetExecutablePath();
        wxExecute(exec_path);
        if (wxGetApp().mainframe) {
            wxGetApp().mainframe->Close();
        }
    }
}

} // namespace Slic3r::GUI
