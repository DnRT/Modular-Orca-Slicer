#include "PluginsPanel.hpp"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/dcclient.h>
#include <wx/graphics.h>
#include <fstream>
#include <boost/filesystem.hpp>
#include "I18N.hpp"
#include "GUI.hpp"
#include "GUI_App.hpp"
#include "MainFrame.hpp"
#include "Tab.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/Modular/ModuleManager.hpp"
#include <nlohmann/json.hpp>

namespace fs = boost::filesystem;
using json = nlohmann::json;

namespace Slic3r::GUI {

// ----------------------------------------------------------------------------
// PluginCard: Individual Marketplace Card Component
// ----------------------------------------------------------------------------

PluginCard::PluginCard(wxWindow* parent, const PluginItemData& data, std::function<void(const std::string&, bool)> on_toggle)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
    , m_data(data)
    , m_on_toggle(std::move(on_toggle))
{
    bool is_dark = wxGetApp().dark_mode();

    // Theme palette
    wxColour bg_card    = is_dark ? wxColour(34, 38, 40) : wxColour(255, 255, 255);
    wxColour text_title = is_dark ? wxColour(242, 244, 245) : wxColour(30, 33, 36);
    wxColour text_sub   = is_dark ? wxColour(150, 155, 160) : wxColour(110, 115, 120);
    wxColour text_desc  = is_dark ? wxColour(200, 205, 210) : wxColour(70, 75, 80);
    wxColour badge_bg   = is_dark ? wxColour(20, 65, 58) : wxColour(228, 245, 241);
    wxColour badge_txt  = is_dark ? wxColour(64, 206, 178) : wxColour(0, 137, 123);
    wxColour avatar_bg  = is_dark ? wxColour(45, 50, 54) : wxColour(240, 243, 245);

    SetBackgroundColour(bg_card);

    auto* card_sizer = new wxBoxSizer(wxVERTICAL);

    // 1. Top Section: Avatar Icon + Title/Author + Category Chip
    auto* top_sizer = new wxBoxSizer(wxHORIZONTAL);

    // Avatar Badge
    auto* avatar_panel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(46, 46));
    avatar_panel->SetBackgroundColour(avatar_bg);
    auto* avatar_sizer = new wxBoxSizer(wxVERTICAL);
    auto* icon_txt = new wxStaticText(avatar_panel, wxID_ANY, m_data.icon_symbol, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    wxFont icon_font = icon_txt->GetFont();
    icon_font.SetPointSize(icon_font.GetPointSize() + 4);
    icon_font.SetWeight(wxFONTWEIGHT_BOLD);
    icon_txt->SetFont(icon_font);
    avatar_sizer->AddStretchSpacer(1);
    avatar_sizer->Add(icon_txt, 0, wxALIGN_CENTER_HORIZONTAL);
    avatar_sizer->AddStretchSpacer(1);
    avatar_panel->SetSizer(avatar_sizer);
    top_sizer->Add(avatar_panel, 0, wxALIGN_TOP | wxRIGHT, 12);

    // Title & Author Stack
    auto* title_sizer = new wxBoxSizer(wxVERTICAL);
    auto* name_lbl = new wxStaticText(this, wxID_ANY, m_data.name);
    wxFont title_font = name_lbl->GetFont();
    title_font.SetPointSize(title_font.GetPointSize() + 2);
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    name_lbl->SetFont(title_font);
    name_lbl->SetForegroundColour(text_title);
    title_sizer->Add(name_lbl, 0, wxBOTTOM, 2);

    wxString author_str = wxString::Format(_L("por %s"), m_data.author);
    if (m_data.verified) {
        author_str += "  [✓ Verificado]";
    }
    auto* author_lbl = new wxStaticText(this, wxID_ANY, author_str);
    author_lbl->SetForegroundColour(text_sub);
    title_sizer->Add(author_lbl, 0, 0, 0);

    top_sizer->Add(title_sizer, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);

    // Category Tag Chip
    auto* chip_panel = new wxPanel(this, wxID_ANY);
    chip_panel->SetBackgroundColour(badge_bg);
    auto* chip_sizer = new wxBoxSizer(wxHORIZONTAL);
    auto* chip_txt = new wxStaticText(chip_panel, wxID_ANY, m_data.category_tag);
    wxFont chip_font = chip_txt->GetFont();
    chip_font.SetPointSize(chip_font.GetPointSize() - 1);
    chip_font.SetWeight(wxFONTWEIGHT_SEMIBOLD);
    chip_txt->SetFont(chip_font);
    chip_txt->SetForegroundColour(badge_txt);
    chip_sizer->Add(chip_txt, 0, wxALL, 5);
    chip_panel->SetSizer(chip_sizer);
    top_sizer->Add(chip_panel, 0, wxALIGN_TOP);

    card_sizer->Add(top_sizer, 0, wxEXPAND | wxALL, 14);

    // 2. Middle Section: Maker-friendly Description
    auto* desc_lbl = new wxStaticText(this, wxID_ANY, m_data.desc);
    desc_lbl->SetForegroundColour(text_desc);
    desc_lbl->Wrap(420);
    card_sizer->Add(desc_lbl, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);

    // 3. Bottom Section: Version + Status Indicator + Primary Action Button
    auto* bottom_sizer = new wxBoxSizer(wxHORIZONTAL);

    auto* ver_lbl = new wxStaticText(this, wxID_ANY, m_data.version);
    ver_lbl->SetForegroundColour(text_sub);
    bottom_sizer->Add(ver_lbl, 0, wxALIGN_CENTER_VERTICAL);

    bottom_sizer->AddStretchSpacer(1);

    m_status_lbl = new wxStaticText(this, wxID_ANY, "");
    wxFont st_font = m_status_lbl->GetFont();
    st_font.SetWeight(wxFONTWEIGHT_BOLD);
    m_status_lbl->SetFont(st_font);
    bottom_sizer->Add(m_status_lbl, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);

    m_action_btn = new wxButton(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, 30));
    m_action_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        bool target = !m_data.is_active;
        if (m_on_toggle) {
            m_on_toggle(m_data.id, target);
        }
    });
    bottom_sizer->Add(m_action_btn, 0, wxALIGN_CENTER_VERTICAL);

    card_sizer->Add(bottom_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 14);

    SetSizer(card_sizer);

    Bind(wxEVT_PAINT, &PluginCard::on_paint, this);

    update_state(m_data.is_active);
}

void PluginCard::update_state(bool is_active)
{
    m_data.is_active = is_active;
    if (is_active) {
        m_status_lbl->SetLabelText(_L("✓ Activado"));
        m_status_lbl->SetForegroundColour(wxColour(46, 175, 100)); // Vivid green
        m_action_btn->SetLabelText(_L("Desactivar"));
    } else {
        m_status_lbl->SetLabelText(_L("Disponible"));
        m_status_lbl->SetForegroundColour(wxColour(140, 145, 150));
        m_action_btn->SetLabelText(_L("Activar Plugin"));
    }
    m_action_btn->Refresh();
    m_status_lbl->Refresh();
    Layout();
}

void PluginCard::on_paint(wxPaintEvent&)
{
    wxPaintDC dc(this);
    bool is_dark = wxGetApp().dark_mode();
    wxColour border_col = is_dark ? wxColour(58, 63, 66) : wxColour(220, 224, 228);
    wxSize sz = GetSize();

    dc.SetPen(wxPen(border_col, 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(0, 0, sz.x, sz.y);
}


// ----------------------------------------------------------------------------
// PluginsPanel: Main Marketplace View
// ----------------------------------------------------------------------------

PluginsPanel::PluginsPanel(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxPanel(parent, id, pos, size, style)
{
    build();
}

void PluginsPanel::build()
{
    bool is_dark = wxGetApp().dark_mode();
    wxColour bg_root = is_dark ? wxColour(24, 27, 28) : wxColour(247, 248, 250);
    wxColour text_pri = is_dark ? wxColour(245, 245, 245) : wxColour(25, 25, 25);
    wxColour text_sec = is_dark ? wxColour(160, 165, 170) : wxColour(110, 115, 120);

    SetBackgroundColour(bg_root);

    auto* root_sizer = new wxBoxSizer(wxVERTICAL);

    // =========================================================================
    // 1. Marketplace Hero Header Banner
    // =========================================================================
    auto* hero_panel = new wxPanel(this, wxID_ANY);
    hero_panel->SetBackgroundColour(is_dark ? wxColour(30, 34, 36) : wxColour(255, 255, 255));
    auto* hero_sizer = new wxBoxSizer(wxVERTICAL);

    auto* top_row = new wxBoxSizer(wxHORIZONTAL);

    // Title & Subtitle
    auto* text_box = new wxBoxSizer(wxVERTICAL);
    auto* title_txt = new wxStaticText(hero_panel, wxID_ANY, _L("Marketplace de Extensiones & Plugins"));
    wxFont h1_font = title_txt->GetFont();
    h1_font.SetPointSize(h1_font.GetPointSize() + 6);
    h1_font.SetWeight(wxFONTWEIGHT_BOLD);
    title_txt->SetFont(h1_font);
    title_txt->SetForegroundColour(text_pri);
    text_box->Add(title_txt, 0, wxBOTTOM, 3);

    auto* sub_txt = new wxStaticText(hero_panel, wxID_ANY,
        _L("Explora, activa e instala algoritmos de corte comunitarios y mejoras modulares para tus proyectos 3D."));
    sub_txt->SetForegroundColour(text_sec);
    text_box->Add(sub_txt, 0, 0, 0);

    top_row->Add(text_box, 1, wxALIGN_CENTER_VERTICAL | wxALL, 18);

    // Quick Actions
    auto* top_actions = new wxBoxSizer(wxHORIZONTAL);
    auto* btn_add = new wxButton(hero_panel, wxID_ANY, _L("+ Instalar Extensión..."));
    auto* btn_dir = new wxButton(hero_panel, wxID_ANY, _L("Carpeta de Plugins"));

    btn_add->Bind(wxEVT_BUTTON, &PluginsPanel::on_install_plugin, this);
    btn_dir->Bind(wxEVT_BUTTON, &PluginsPanel::on_open_plugins_dir, this);

    top_actions->Add(btn_add, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    top_actions->Add(btn_dir, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 18);
    top_row->Add(top_actions, 0, wxALIGN_CENTER_VERTICAL);

    hero_sizer->Add(top_row, 0, wxEXPAND);

    // Search and Filter Bar Row
    auto* filter_row = new wxBoxSizer(wxHORIZONTAL);

    // Real-time Search Box
    m_search_ctrl = new wxTextCtrl(hero_panel, wxID_ANY, "", wxDefaultPosition, wxSize(280, 32), wxTE_PROCESS_ENTER);
    m_search_ctrl->SetHint(_L("Buscar plugins, algoritmos o creadores..."));
    m_search_ctrl->Bind(wxEVT_TEXT, &PluginsPanel::on_search_changed, this);
    filter_row->Add(m_search_ctrl, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT | wxBOTTOM, 18);

    // Category Tabs / Filter Pills
    std::vector<std::pair<wxString, wxString>> categories = {
        { "Todos", _L("Todos") },
        { "Algoritmos de Corte", _L("Algoritmos de Corte") },
        { "Rendimiento", _L("Rendimiento") },
        { "Cinemática", _L("Cinemática") },
        { "Instalados", _L("Instalados") }
    };

    for (const auto& cat : categories) {
        auto* f_btn = new wxButton(hero_panel, wxID_ANY, cat.second, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
        f_btn->Bind(wxEVT_BUTTON, [this, cat_id = cat.first](wxCommandEvent&) {
            set_category_filter(cat_id);
        });
        m_filter_btns.push_back(f_btn);
        filter_row->Add(f_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 8);
    }

    hero_sizer->Add(filter_row, 0, wxEXPAND);
    hero_panel->SetSizer(hero_sizer);
    root_sizer->Add(hero_panel, 0, wxEXPAND | wxBOTTOM, 10);

    // =========================================================================
    // 2. Scrollable Cards Marketplace Grid
    // =========================================================================
    m_scroll_win = new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
    m_scroll_win->SetScrollRate(0, 15);
    m_scroll_win->SetBackgroundColour(bg_root);

    m_cards_sizer = new wxFlexGridSizer(0, 2, 14, 14);
    m_cards_sizer->AddGrowableCol(0, 1);
    m_cards_sizer->AddGrowableCol(1, 1);

    auto* scroll_container = new wxBoxSizer(wxVERTICAL);
    scroll_container->Add(m_cards_sizer, 1, wxEXPAND | wxALL, 18);
    m_scroll_win->SetSizer(scroll_container);

    root_sizer->Add(m_scroll_win, 1, wxEXPAND);

    // =========================================================================
    // 3. Footer Bar: Status Note & Restart Shortcut
    // =========================================================================
    auto* footer_panel = new wxPanel(this, wxID_ANY);
    footer_panel->SetBackgroundColour(is_dark ? wxColour(30, 34, 36) : wxColour(255, 255, 255));
    auto* footer_sizer = new wxBoxSizer(wxHORIZONTAL);

    m_footer_text = new wxStaticText(footer_panel, wxID_ANY,
        _L("ℹ Los cambios en las extensiones se aplican al reiniciar Modular OrcaSlicer para garantizar la estabilidad del corte."));
    m_footer_text->SetForegroundColour(text_sec);
    footer_sizer->Add(m_footer_text, 1, wxALIGN_CENTER_VERTICAL | wxALL, 14);

    m_restart_btn = new wxButton(footer_panel, wxID_ANY, _L("Reiniciar Modular OrcaSlicer"), wxDefaultPosition, wxSize(200, 32));
    m_restart_btn->Bind(wxEVT_BUTTON, &PluginsPanel::on_restart_requested, this);
    footer_sizer->Add(m_restart_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 14);

    footer_panel->SetSizer(footer_sizer);
    root_sizer->Add(footer_panel, 0, wxEXPAND);

    SetSizer(root_sizer);
    Layout();

    refresh_catalog();
}

void PluginsPanel::refresh_catalog()
{
    // Pre-populate modern community marketplace items
    m_plugins = {
        {
            "wave_overhangs",
            _L("Wave Overhangs"),
            "OverhangStrategy",
            "Janis Andersons & Dennis Klappe",
            "Algoritmos de Corte",
            _L("Voladizos"),
            "v1.2.0",
            "WAVE",
            _L("Genera voladizos autoportantes de hasta 85° sin necesidad de soportes tradicionales mediante ondas continuas de extrusión adaptativa."),
            true,
            false
        },
        {
            "resource_optimizer_lite",
            _L("Low-RAM Model Optimizer"),
            "ResourceOptimizer",
            "Modular RepRap Community",
            "Rendimiento",
            _L("Rendimiento"),
            "v1.0.4",
            "RAM",
            _L("Reduce drásticamente el consumo de memoria RAM desacoplando los buffers de malla en modelos masivos con millones de triángulos."),
            true,
            false
        },
        {
            "arc_overhangs",
            _L("Arc Overhangs"),
            "OverhangStrategy",
            "Steven McCulloch",
            "Algoritmos de Corte",
            _L("Puentes"),
            "v2.1.0",
            "ARC",
            _L("Utiliza arcos circulares concéntricos para tender puentes en el aire en ángulos extremos sin requerir estructuras de soporte inferiores."),
            true,
            false
        },
        {
            "non_planar_kinematics",
            _L("Non-Planar 3-Axis Kinematics"),
            "KinematicsEngine",
            "Open Non-Planar Project",
            "Cinemática",
            _L("Superficies 3D"),
            "v0.9.5-beta",
            "3D-Z",
            _L("Permite la deposición tridimensional de capas continuas en XYZ simultáneos, eliminando el efecto escalera en superficies curvas."),
            false,
            false
        },
        {
            "modular_infill_walls",
            _L("Algorithmic Walls & Infill"),
            "InfillPattern",
            "Modular RepRap Community",
            "Algoritmos de Corte",
            _L("Paredes y Relleno"),
            "v1.0.0",
            "WALL",
            _L("Optimiza trayectorias de paredes y relleno con amortiguación adaptativa para evitar rozaduras de boquilla."),
            true,
            false
        },
        {
            "adaptive_2d_simplifier",
            _L("Adaptive 2D Simplifier"),
            "ResourceOptimizer",
            "Modular RepRap Community",
            "Rendimiento",
            _L("Geometría 2D"),
            "v1.0.0",
            "DECIM",
            _L("Simplificación poligonal 2D adaptativa escalada a cualquier diámetro de boquilla (0.2 a 1.0mm). Acelera drásticamente el cálculo de paredes Arachne en piezas complejas."),
            true,
            false
        },
        {
            "eager_layer_flusher",
            _L("Eager Layer Flusher"),
            "ResourceOptimizer",
            "Modular RepRap Community",
            "Rendimiento",
            _L("Memoria RAM"),
            "v1.0.0",
            "FLUSH",
            _L("Libera activamente búferes intermedios de capas al completar perímetros y relleno, estabilizando el consumo de RAM en modelos con cientos de capas (0.2 a 1.0mm)."),
            true,
            false
        }
    };

    // Query active status from ModuleManager and modpack.json
    for (auto& item : m_plugins) {
        item.is_active = check_is_active(item.id);
    }

    rebuild_cards();
}

bool PluginsPanel::check_is_active(const std::string& mod_id) const
{
    // 1. Check live kernel
    const auto &loaded = Modular::ModuleManager::instance().get_loaded_mods();
    for (const auto &m : loaded) {
        if (m.id == mod_id || (mod_id == "wave_overhangs" && m.name.find("Wave") != std::string::npos)) {
            return true;
        }
        if (mod_id == "resource_optimizer_lite" && m.id == "resource_optimizer_lite") {
            return true;
        }
    }

    // 2. Check modpack.json on disk
    fs::path modpack_path = fs::path(Slic3r::data_dir()) / "modpack.json";
    if (fs::exists(modpack_path)) {
        try {
            std::ifstream in(modpack_path.string());
            json j;
            in >> j;
            if (j.contains("mods") && j["mods"].is_array()) {
                for (const auto& m : j["mods"]) {
                    if (m.value("id", "") == mod_id) {
                        return m.value("enabled", false);
                    }
                }
            }
        } catch (...) {}
    }

    return false;
}

void PluginsPanel::sync_modpack_disk(const std::string& mod_id, bool enable)
{
    fs::path modpack_path = fs::path(Slic3r::data_dir()) / "modpack.json";
    json manifest;

    if (fs::exists(modpack_path)) {
        try {
            std::ifstream in(modpack_path.string());
            in >> manifest;
        } catch (...) {
            manifest = json::object();
        }
    }

    manifest["modpack_id"] = "user_orchestrated";
    manifest["name"] = "Modular OrcaSlicer Active Profile";
    if (!manifest.contains("mods") || !manifest["mods"].is_array()) {
        manifest["mods"] = json::array();
    }

    bool found = false;
    for (auto& m : manifest["mods"]) {
        if (m.value("id", "") == mod_id) {
            m["enabled"] = enable;
            found = true;
            break;
        }
    }

    if (!found) {
        json item;
        item["id"] = mod_id;
        item["enabled"] = enable;
        for (const auto& p : m_plugins) {
            if (p.id == mod_id) {
                item["name"] = p.name.ToStdString();
                item["replaces"] = p.internal_hook;
                break;
            }
        }
        manifest["mods"].push_back(item);
    }

    try {
        std::ofstream out(modpack_path.string());
        if (out.is_open()) {
            out << manifest.dump(2);
        }
    } catch (...) {}
}

void PluginsPanel::rebuild_cards()
{
    if (!m_cards_sizer || !m_scroll_win) return;

    // Destroy existing card widgets
    for (auto* card : m_card_widgets) {
        card->Destroy();
    }
    m_card_widgets.clear();
    m_cards_sizer->Clear(false);

    wxString q = m_search_query.Lower();

    for (const auto& item : m_plugins) {
        // Filter by category
        if (m_active_filter == "Instalados" && !item.is_active) {
            continue;
        }
        if (m_active_filter != "Todos" && m_active_filter != "Instalados" && item.category != m_active_filter) {
            continue;
        }

        // Filter by search text
        if (!q.empty()) {
            bool matches = item.name.Lower().Contains(q) ||
                           item.author.Lower().Contains(q) ||
                           item.desc.Lower().Contains(q) ||
                           item.category_tag.Lower().Contains(q);
            if (!matches) continue;
        }

        auto* card = new PluginCard(m_scroll_win, item, [this](const std::string& id, bool target) {
            on_toggle_plugin(id, target);
        });

        m_card_widgets.push_back(card);
        m_cards_sizer->Add(card, 1, wxEXPAND);
    }

    m_scroll_win->Layout();
    m_scroll_win->FitInside();
}

void PluginsPanel::on_toggle_plugin(const std::string& mod_id, bool enable)
{
    sync_modpack_disk(mod_id, enable);
    Modular::ModuleManager::instance().set_plugin_active(mod_id, enable);

    for (auto& item : m_plugins) {
        if (item.id == mod_id) {
            item.is_active = enable;
            break;
        }
    }

    for (auto* card : m_card_widgets) {
        if (card->get_data().id == mod_id) {
            card->update_state(enable);
            break;
        }
    }

    if (enable) {
        m_footer_text->SetLabelText(_L("✓ Extensión activada. Se aplicará al reiniciar Modular OrcaSlicer."));
        m_footer_text->SetForegroundColour(wxColour(46, 175, 100));
    } else {
        m_footer_text->SetLabelText(_L("ℹ Extensión desactivada. Se aplicará al reiniciar Modular OrcaSlicer."));
        m_footer_text->SetForegroundColour(wxColour(200, 160, 50));
    }
    m_footer_text->Refresh();

    try {
        if (Tab* print_tab = wxGetApp().get_tab(Preset::TYPE_PRINT)) {
            print_tab->update_visibility();
        }
    } catch (...) {}
}

void PluginsPanel::on_search_changed(wxCommandEvent&)
{
    if (m_search_ctrl) {
        m_search_query = m_search_ctrl->GetValue();
        rebuild_cards();
    }
}

void PluginsPanel::set_category_filter(const wxString& category)
{
    m_active_filter = category;
    rebuild_cards();
}

void PluginsPanel::on_install_plugin(wxCommandEvent&)
{
    wxMessageBox(
        _L("Modular OrcaSlicer integra todos los módulos y algoritmos avanzados nativamente en el motor de corte para máxima resiliencia, estabilidad y ciberseguridad, eliminando la inyección de DLLs externas no verificadas.\n\nPuedes activar o desactivar cada módulo con los botones de este panel y seleccionar el algoritmo deseado para cada laminado en la configuración de proceso."),
        _L("Gestor de Módulos Nativos"), wxOK | wxICON_INFORMATION);
    refresh_catalog();
}

void PluginsPanel::on_open_plugins_dir(wxCommandEvent&)
{
    wxString dir = from_u8(Slic3r::data_dir()) + wxFileName::GetPathSeparator() + "plugins";
    if (!wxDirExists(dir)) {
        wxMkdir(dir);
    }
    wxLaunchDefaultApplication(dir);
}

void PluginsPanel::on_restart_requested(wxCommandEvent&)
{
    int answer = wxMessageBox(
        _L("¿Deseas reiniciar Modular OrcaSlicer ahora para aplicar las extensiones y configuraciones seleccionadas?"),
        _L("Confirmar Reinicio"), wxYES_NO | wxICON_QUESTION);
    if (answer == wxYES) {
        wxString exec_path = wxStandardPaths::Get().GetExecutablePath();
        wxExecute("\"" + exec_path + "\"");
        if (wxGetApp().mainframe) {
            wxGetApp().mainframe->Close();
        }
    }
}

} // namespace Slic3r::GUI
