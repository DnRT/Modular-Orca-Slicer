#pragma once

#include "wx/wxprec.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/listctrl.h>
#include <wx/checkbox.h>
#include "slic3r/GUI/Widgets/LazyInstance.hpp"

namespace Slic3r::GUI {

class PluginsPanel : public wxPanel, public StagedBuild, public LazyInstance<PluginsPanel>
{
public:
    PluginsPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL);
    ~PluginsPanel() override = default;

    void build() override;
    void refresh_catalog();

private:
    void on_install_plugin(wxCommandEvent &event);
    void on_open_plugins_dir(wxCommandEvent &event);
    void on_restart_requested(wxCommandEvent &event);

    wxListCtrl* m_list_ctrl{nullptr};
    wxStaticText* m_status_text{nullptr};
};

} // namespace Slic3r::GUI
