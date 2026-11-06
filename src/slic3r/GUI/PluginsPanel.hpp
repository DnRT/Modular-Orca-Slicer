#pragma once

#include "wx/wxprec.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <functional>
#include <vector>
#include <string>
#include "slic3r/GUI/Lazy.hpp"

namespace Slic3r::GUI {

struct PluginItemData {
    std::string id;
    wxString name;
    std::string internal_hook;
    wxString author;
    wxString category;
    wxString category_tag;
    wxString version;
    wxString icon_symbol;
    wxString desc;
    bool verified{true};
    bool is_active{false};
};

class PluginCard : public wxPanel
{
public:
    PluginCard(wxWindow* parent, const PluginItemData& data, std::function<void(const std::string&, bool)> on_toggle);

    void update_state(bool is_active);
    const PluginItemData& get_data() const { return m_data; }

private:
    void on_paint(wxPaintEvent& evt);

    PluginItemData m_data;
    std::function<void(const std::string&, bool)> m_on_toggle;
    wxPanel* m_status_badge{nullptr};
    wxStaticText* m_status_lbl{nullptr};
    wxButton* m_action_btn{nullptr};
};

class PluginsPanel : public wxPanel, public LazyInstance<PluginsPanel>
{
public:
    PluginsPanel(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL);
    ~PluginsPanel() override = default;

    void build();
    void refresh_catalog();

private:
    void on_search_changed(wxCommandEvent &event);
    void set_category_filter(const wxString& category);
    void on_toggle_plugin(const std::string& mod_id, bool enable);
    void on_install_plugin(wxCommandEvent &event);
    void on_open_plugins_dir(wxCommandEvent &event);
    void on_restart_requested(wxCommandEvent &event);

    void rebuild_cards();
    void sync_modpack_disk(const std::string& mod_id, bool enable);
    bool check_is_active(const std::string& mod_id) const;

    wxTextCtrl* m_search_ctrl{nullptr};
    wxScrolledWindow* m_scroll_win{nullptr};
    wxFlexGridSizer* m_cards_sizer{nullptr};
    wxStaticText* m_footer_text{nullptr};
    wxButton* m_restart_btn{nullptr};

    std::vector<wxButton*> m_filter_btns;
    std::vector<PluginItemData> m_plugins;
    std::vector<PluginCard*> m_card_widgets;
    wxString m_active_filter{"Todos"};
    wxString m_search_query{""};
};

} // namespace Slic3r::GUI
