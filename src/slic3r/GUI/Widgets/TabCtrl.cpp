#include "TabCtrl.hpp"

#include "slic3r/GUI/Widgets/StaticBox.hpp"
#include <wx/anybutton.h>
#include <wx/checklst.h>
#include <cstddef>
#include <utility>
#include <wx/colour.h>
#include <algorithm>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include "slic3r/GUI/Widgets/Button.hpp"
#include "slic3r/GUI/Widgets/StateColor.hpp"
#include "slic3r/GUI/Widgets/StateHandler.hpp"

wxDEFINE_EVENT(wxEVT_TAB_SEL_CHANGING, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_TAB_SEL_CHANGED, wxCommandEvent);

BEGIN_EVENT_TABLE(TabCtrl, StaticBox)

EVT_KEY_DOWN(TabCtrl::keyDown)

END_EVENT_TABLE()

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

#define TAB_BUTTON_SPACE 2
#define TAB_BUTTON_PADDING_X 3
#define TAB_BUTTON_PADDING_Y 2
#define TAB_BUTTON_PADDING TAB_BUTTON_PADDING_X, TAB_BUTTON_PADDING_Y

TabCtrl::TabCtrl(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : StaticBox(parent, id, pos, size, style)
{
#if 0
    radius = 5;
#else
    radius = 1;
#endif
    border_width = 1;
    SetBorderColor(0xcecece);
    sizer = new wxBoxSizer(wxHORIZONTAL);
    sizer->AddSpacer(10);
    auto hsizer = new wxBoxSizer(wxVERTICAL);
    hsizer->Add(sizer, 0, wxEXPAND | wxBOTTOM, border_width * 4);
    SetSizer(hsizer);
    Bind(wxEVT_COMMAND_BUTTON_CLICKED, &TabCtrl::buttonClicked, this);
    Bind(wxEVT_MOUSEWHEEL, &TabCtrl::mouseWheel, this);
    // wxString reason;
    // IsTransparentBackgroundSupported(&reason);
}

TabCtrl::~TabCtrl() { delete images; }

int TabCtrl::GetSelection() const { return sel; }

void TabCtrl::SelectItem(int item)
{
    if (item == sel || !sendTabCtrlEvent(true))
        return;
    if (sel >= 0 && sel < static_cast<int>(btns.size())) {
        wxCommandEvent e(wxEVT_CHECKBOX);
        auto b = btns[sel];
        e.SetEventObject(b);
        b->GetEventHandler()->ProcessEvent(e);
    }
    sel = item;
    if (sel >= 0 && sel < static_cast<int>(btns.size())) {
        wxCommandEvent e(wxEVT_CHECKBOX);
        auto b = btns[sel];
        e.SetEventObject(b);
        b->GetEventHandler()->ProcessEvent(e);
    }
    sendTabCtrlEvent();
    relayout();
    Refresh();
}

void TabCtrl::Unselect() { SelectItem(-1); }

void TabCtrl::Rescale()
{
    for (auto& b : btns)
        b->Rescale();
    relayout();
}

bool TabCtrl::SetFont(wxFont const& font)
{
    StaticBox::SetFont(font);
    bold = font.Bold();
    for (size_t i = 0; i < btns.size(); ++i)
        btns[i]->SetFont(i == sel ? bold : font);
    return true;
}

int TabCtrl::AppendItem(const wxString& item, int image, int selImage, void* clientData)
{
    Button* btn = new Button();
    btn->Create(this, item, "", wxBORDER_NONE);
    btn->SetFont(GetFont());
    btn->SetTextColor(
        StateColor(std::make_pair(0x6B6B6C, (int) StateColor::NotChecked), std::make_pair(wxColour("#262E30"), (int) StateColor::Normal)));
    btn->SetBackgroundColor(StateColor());
    btn->SetCornerRadius(0);
    btn->SetPaddingSize({TAB_BUTTON_PADDING});
    btns.push_back(btn);
    if (btns.size() > 1)
        sizer->GetItem(sizer->GetItemCount() - 1)->SetMinSize({0, 0});
    btn->Bind(wxEVT_MOUSEWHEEL, [this](wxMouseEvent& e) { this->mouseWheel(e); });
    sizer->Add(btn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, TAB_BUTTON_SPACE * 2);
    sizer->AddStretchSpacer(1);
    relayout();
    return btns.size() - 1;
}

int TabCtrl::AppendItem(const wxString& item, const wxBitmap& bitmap, void* clientData)
{
    const int index = AppendItem(item, -1, -1, clientData);
    SetItemBitmap(index, bitmap);
    return index;
}

bool TabCtrl::DeleteItem(int item)
{
    if (item < 0 || item >= btns.size()) {
        return false;
    }
    const bool selection_changed = sel >= item;

    if (selection_changed) {
        sendTabCtrlEvent(true);
    }

    Button* btn = btns[item];
    btn->Destroy();
    btns.erase(btns.begin() + item);
    sizer->Remove(item * 2);
    if (btns.size() > 1)
        sizer->GetItem(sizer->GetItemCount() - 1)->SetMinSize({0, 0});

    if (selection_changed) {
        sel--; // `relayout()` uses `sel` so we need to update this before calling `relayout()`
    }
    relayout();
    if (selection_changed) {
        sendTabCtrlEvent();
    }

    return true;
}

void TabCtrl::DeleteAllItems()
{
    sizer->Clear(true);
    sizer->AddSpacer(10);
    btns.clear();
    if (sel >= 0) {
        sel = -1;
        sendTabCtrlEvent();
    }
}

unsigned int TabCtrl::GetCount() const { return btns.size(); }

wxString TabCtrl::GetItemText(unsigned int item) const { return item < btns.size() ? btns[item]->GetLabel() : wxString{}; }

void TabCtrl::SetItemText(unsigned int item, wxString const& value)
{
    if (item >= btns.size())
        return;
    btns[item]->SetLabel(value);
}

void TabCtrl::SetItemBitmap(unsigned int item, const wxBitmap& bitmap)
{
    if (item >= btns.size())
        return;
    btns[item]->SetIcon(bitmap);
    relayout();
}

void TabCtrl::SetItemIndicator(unsigned int item, bool on)
{
    if (item >= btns.size())
        return;
    btns[item]->SetIndicator(on);
    relayout();
}

bool TabCtrl::GetItemBold(unsigned int item) const
{
    if (item >= btns.size())
        return false;
    return btns[item]->GetFont() == bold;
}

void TabCtrl::SetItemBold(unsigned int item, bool bold)
{
    if (item >= btns.size())
        return;
    btns[item]->SetFont(bold ? this->bold : GetFont());
    btns[item]->Rescale();
}

void* TabCtrl::GetItemData(unsigned int item) const
{
    if (item >= btns.size())
        return nullptr;
    return btns[item]->GetClientData();
}

void TabCtrl::SetItemData(unsigned int item, void* clientData)
{
    if (item >= btns.size())
        return;
    btns[item]->SetClientData(clientData);
}

void TabCtrl::AssignImageList(wxImageList* imageList)
{
    if (images == imageList)
        return;
    delete images;
    images = imageList;
}

void TabCtrl::SetItemTextColour(unsigned int item, const StateColor& col)
{
    if (item >= btns.size())
        return;
    btns[item]->SetTextColor(col);
}

int TabCtrl::GetFirstVisibleItem() const { return btns.size() == 0 ? -1 : 0; }

int TabCtrl::GetNextVisible(int item) const { return ++item < btns.size() ? item : -1; }

bool TabCtrl::IsVisible(unsigned int item) const { return true; }

void TabCtrl::DoSetSize(int x, int y, int width, int height, int sizeFlags)
{
    auto size = GetSize();
    wxWindow::DoSetSize(x, y, width, height, sizeFlags);
    if (m_in_relayout || (size.x == GetSize().x && size.y == GetSize().y))
        return;
    relayout();
}

#ifdef __WIN32__

WXLRESULT TabCtrl::MSWWindowProc(WXUINT nMsg, WXWPARAM wParam, WXLPARAM lParam)
{
    if (nMsg == WM_GETDLGCODE) {
        return DLGC_WANTARROWS;
    }
    return wxWindow::MSWWindowProc(nMsg, wParam, lParam);
}

#endif

void TabCtrl::relayout()
{
    if (m_in_relayout)
        return;
    m_in_relayout = true;

    if (btns.empty()) {
        Layout();
        m_in_relayout = false;
        return;
    }

    const size_t count = sizer ? sizer->GetItemCount() : 0;
    const int n = static_cast<int>(btns.size());
    const int width = GetSize().x;

    if (m_first_visible < 0)
        m_first_visible = 0;
    if (m_first_visible >= n)
        m_first_visible = n - 1;

    if (sel >= 0 && sel < n) {
        if (sel < m_first_visible) {
            m_first_visible = sel;
        } else {
            while (m_first_visible < sel) {
                int needed = 10;
                for (int j = m_first_visible; j <= sel; ++j)
                    needed += btns[j]->GetMinSize().x + TAB_BUTTON_SPACE * 2;
                if (needed <= width)
                    break;
                ++m_first_visible;
            }
        }
    }

    int offset = 10;
    int last_visible = m_first_visible;

    for (int i = 0; i < n; ++i) {
        bool show = false;
        if (i >= m_first_visible) {
            int btn_w = btns[i]->GetMinSize().x + TAB_BUTTON_SPACE * 2;
            if (offset + btn_w <= width || i == m_first_visible) {
                show = true;
                offset += btn_w;
                last_visible = i;
            }
        }

        if (static_cast<size_t>(i * 2 + 1) < count)
            sizer->Show(i * 2 + 1, show);
        if (static_cast<size_t>(i * 2 + 2) < count)
            sizer->Show(i * 2 + 2, show);

        if (static_cast<size_t>(i * 2 + 2) < count) {
            if (auto it = sizer->GetItem(i * 2 + 2))
                it->SetMinSize({0, 0});
        }
    }

    int b = width - offset - 10 - (last_visible + 1 - m_first_visible) * TAB_BUTTON_SPACE * 8;
    if (last_visible >= 0 && static_cast<size_t>(last_visible * 2 + 2) < count) {
        if (auto it = sizer->GetItem(last_visible * 2 + 2))
            it->SetMinSize({b > 0 ? b : 0, 0});
    }

    Layout();
    m_in_relayout = false;
}

int TabCtrl::GetFullSize() const
{
    // Mirrors relayout(): a 10px leading spacer plus every button's min width.
    int width = 10;
    for (const Button* btn : btns)
        width += btn->GetMinSize().x;
    return width;
}

void TabCtrl::buttonClicked(wxCommandEvent& event)
{
    SetFocus();
    auto btn  = event.GetEventObject();
    auto iter = std::find(btns.begin(), btns.end(), btn);
    SelectItem(iter == btns.end() ? -1 : iter - btns.begin());
}

void TabCtrl::keyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
    case WXK_UP:
    case WXK_DOWN:
    case WXK_LEFT:
    case WXK_RIGHT:
        if ((event.GetKeyCode() == WXK_UP || event.GetKeyCode() == WXK_LEFT) && GetSelection() > 0) {
            SelectItem(GetSelection() - 1);
        } else if ((event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_RIGHT) && GetSelection() + 1 < static_cast<int>(btns.size())) {
            SelectItem(GetSelection() + 1);
        }
        break;
    default:
        event.Skip();
        break;
    }
}

void TabCtrl::mouseWheel(wxMouseEvent& event)
{
    int rotation = event.GetWheelRotation();
    if (rotation == 0)
        return;
    if (event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL)
        rotation = -rotation;

    int new_first = m_first_visible + (rotation > 0 ? -1 : 1);
    if (new_first < 0)
        new_first = 0;
    if (new_first >= static_cast<int>(btns.size()))
        new_first = static_cast<int>(btns.size()) - 1;

    if (new_first != m_first_visible) {
        m_first_visible = new_first;
        relayout();
        Refresh();
    }
}

void TabCtrl::doRender(wxDC& dc)
{
    wxSize size = GetSize();
    int states  = state_handler.states();
    const int BS2 = (1 + border_width) / 2;

    dc.SetPen(wxPen(border_color.colorForStates(states), border_width));
    dc.DrawLine(0, size.y - BS2, size.x, size.y - BS2);

    if (sel >= 0 && sel < static_cast<int>(btns.size()) && btns[sel]->IsShown()) {
        auto x1 = btns[sel]->GetPosition().x;
        auto x2 = x1 + btns[sel]->GetSize().x;
        wxColour c("#009688"); // ORCA selected tab underline stroke color
        dc.SetPen(wxPen(c, 1));
        dc.SetBrush(c);
        dc.DrawRoundedRectangle(x1 - radius, size.y - BS2 - border_width * 3, x2 + radius * 2 - x1, border_width * 3, radius);
    }
}

bool TabCtrl::sendTabCtrlEvent(bool changing)
{
    wxCommandEvent event(changing ? wxEVT_TAB_SEL_CHANGING : wxEVT_TAB_SEL_CHANGED, GetId());
    event.SetEventObject(this);
    event.SetInt(sel);
    GetEventHandler()->ProcessEvent(event);
    return true;
}
