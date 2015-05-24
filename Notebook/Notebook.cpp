#include "Notebook.h"
#include <wx/sizer.h>
#include <wx/dcbuffer.h>
#include <wx/dcgraph.h>
#include <wx/wupdlock.h>
#include <algorithm>
#include <wx/image.h>
#include <wx/xrc/xmlres.h>
#include <wx/xrc/xh_bmp.h>
#include <wx/menu.h>

wxDEFINE_EVENT(wxEVT_BOOK_PAGE_CHANGING, wxBookCtrlEvent);
wxDEFINE_EVENT(wxEVT_BOOK_PAGE_CHANGED, wxBookCtrlEvent);
wxDEFINE_EVENT(wxEVT_BOOK_PAGE_CLOSING, wxBookCtrlEvent);
wxDEFINE_EVENT(wxEVT_BOOK_PAGE_CLOSED, wxBookCtrlEvent);

extern void Notebook_Init_Bitmaps();

Notebook::Notebook(wxWindow* parent,
                   wxWindowID id,
                   const wxPoint& pos,
                   const wxSize& size,
                   long style,
                   const wxString& name)
    : wxPanel(parent, id, pos, size, style, name)
{
    static bool once = false;
    if(!once) {
        // Add PNG and Bitmap handler
        wxImage::AddHandler(new wxPNGHandler);
        wxXmlResource::Get()->AddHandler(new wxBitmapXmlHandler);
        Notebook_Init_Bitmaps();
        once = true;
    }

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);

    m_tabCtrl = new clTabCtrl(this, style);
    sizer->Add(m_tabCtrl, 0, wxEXPAND);
    m_book = new wxSimplebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0);
    sizer->Add(m_book, 1, wxEXPAND);
    Layout();
}

Notebook::~Notebook() {}

void Notebook::AddPage(wxWindow* page, const wxString& label, bool selected, const wxBitmap& bmp)
{
    clTabInfo::Ptr_t tab(new clTabInfo(GetStyle(), page, label, bmp));
    tab->SetActive(selected, GetStyle());
    m_tabCtrl->AddPage(tab);
}

void Notebook::DoChangeSelection(wxWindow* page)
{
    for(size_t i = 0; i < m_book->GetPageCount(); ++i) {
        if(m_book->GetPage(i) == page) {
            m_book->ChangeSelection(i);
            break;
        }
    }
}

bool Notebook::InsertPage(size_t index, wxWindow* page, const wxString& label, bool selected, const wxBitmap& bmp)
{
    clTabInfo::Ptr_t tab(new clTabInfo(GetStyle(), page, label, bmp));
    tab->SetActive(selected, GetStyle());
    return m_tabCtrl->InsertPage(index, tab);
}

void Notebook::SetStyle(size_t style)
{
    m_tabCtrl->SetStyle(style);
    Refresh();
}

wxWindow* Notebook::GetCurrentPage() const
{
    if(m_tabCtrl->GetSelection() == wxNOT_FOUND) return NULL;
    return m_tabCtrl->GetPage(m_tabCtrl->GetSelection());
}

int Notebook::FindPage(wxWindow* page) const { return m_tabCtrl->FindPage(page); }

bool Notebook::RemovePage(size_t page) { return m_tabCtrl->RemovePage(page, false, false); }
bool Notebook::DeletePage(size_t page) { return m_tabCtrl->RemovePage(page, true, true); }

bool Notebook::DeleteAllPages() { return m_tabCtrl->DeleteAllPages(); }

//----------------------------------------------------------
// Tab label
//----------------------------------------------------------
void clTabInfo::Draw(wxDC& dc, const clTabInfo::Colours& colours, size_t style)
{
    const int TOP_SMALL_HEIGHT = 2;
    wxColour bgColour(IsActive() ? colours.activeTabBgColour : colours.inactiveTabBgColour);
    wxColour penColour(IsActive() ? colours.activeTabPenColour : colours.inactiveTabPenColour);
    {
        wxPoint points[6];
        points[0] = m_rect.GetBottomLeft();

        points[1].x = points[0].x + MAJOR_CURVE_WIDTH;
        points[1].y = m_rect.GetLeftTop().y + TOP_SMALL_HEIGHT;

        points[2].x = points[1].x + SMALL_CURVE_WIDTH;
        points[2].y = points[1].y - TOP_SMALL_HEIGHT;

        points[3].x = points[0].x + (m_rect.GetWidth() - (MAJOR_CURVE_WIDTH + SMALL_CURVE_WIDTH));
        points[3].y = points[2].y;

        points[4].x = points[3].x + SMALL_CURVE_WIDTH;
        points[4].y = points[3].y + TOP_SMALL_HEIGHT;

        points[5] = m_rect.GetBottomRight();

        dc.SetPen(penColour);
        dc.SetBrush(bgColour);
        dc.DrawPolygon(6, points);
    }

    {
        wxPoint points[6];
        points[0] = m_rect.GetBottomLeft();
        points[0].x += 1;

        points[1].x = points[0].x + MAJOR_CURVE_WIDTH;
        points[1].y = m_rect.GetLeftTop().y + TOP_SMALL_HEIGHT + 1;

        points[2].x = points[1].x + SMALL_CURVE_WIDTH;
        points[2].y = points[1].y - TOP_SMALL_HEIGHT;

        points[3].x = points[0].x + (m_rect.GetWidth() - 2 - (MAJOR_CURVE_WIDTH + SMALL_CURVE_WIDTH));
        points[3].y = points[2].y;

        points[4].x = points[3].x + SMALL_CURVE_WIDTH;
        points[4].y = points[3].y + TOP_SMALL_HEIGHT;

        points[5] = m_rect.GetBottomRight();
        points[5].x -= 2;

        dc.SetPen(IsActive() ? colours.activeTabInnerPenColour : colours.inactiveTabInnerPenColour);
        dc.SetBrush(bgColour);
        dc.DrawPolygon(6, points);
    }

    // Draw bitmap
    if(m_bitmap.IsOk()) {
        dc.DrawBitmap(m_bitmap, m_bmpX + m_rect.GetX(), m_bmpY);
    }

    // Draw the text
    dc.SetTextForeground(IsActive() ? colours.activeTabTextColour : colours.inactiveTabTextColour);
    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
#ifdef __WXOSX__
    font.SetPointSize(10);
#else
    font.SetPointSize(9);
#endif

    dc.SetFont(font);
    dc.DrawText(m_label, m_textX + m_rect.GetX(), m_textY);

    if(IsActive() && (style & kNotebook_CloseButtonOnActiveTab)) {
        dc.DrawBitmap(colours.closeButton, m_bmpCloseX + m_rect.GetX(), m_bmpCloseY);
    }
}

void clTabInfo::CalculateOffsets(size_t style)
{
    wxBitmap b(1, 1);
    wxMemoryDC memDC(b);
    m_bmpCloseX = wxNOT_FOUND;
    m_bmpCloseY = wxNOT_FOUND;

    wxFont font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    font.SetPointSize(10);
    memDC.SetFont(font);
    wxSize sz = memDC.GetTextExtent(m_label);
    m_height = TAB_HEIGHT;

    m_width = 0;
    m_width += MAJOR_CURVE_WIDTH;
    m_width += SMALL_CURVE_WIDTH;
    m_width += X_SPACER;

    // bitmap
    m_bmpX = wxNOT_FOUND;
    m_bmpY = wxNOT_FOUND;
    if(m_bitmap.IsOk()) {
        m_bmpX = m_width;
        m_width += m_bitmap.GetWidth();
        m_width += X_SPACER;
        m_bmpY = ((m_height - m_bitmap.GetHeight()) / 2);
    }

    // Text
    m_textX = m_width;
    m_textY = ((m_height - sz.y) / 2);
    m_width += sz.x;

    // x button
    if((style & kNotebook_CloseButtonOnActiveTab)) {
        m_width += X_SPACER;
        m_bmpCloseX = m_width;
        m_bmpCloseY = ((m_height - 12) / 2);
        m_width += 12; // X button is 10 pixels in size
    }
    m_width += X_SPACER;
    m_width += SMALL_CURVE_WIDTH;
    m_width += MAJOR_CURVE_WIDTH;

    // Update the rect width
    m_rect.SetWidth(m_width);
}

void clTabInfo::SetBitmap(const wxBitmap& bitmap, size_t style)
{
    this->m_bitmap = bitmap;
    CalculateOffsets(style);
}

void clTabInfo::SetLabel(const wxString& label, size_t style)
{
    this->m_label = label;
    CalculateOffsets(style);
}

void clTabInfo::SetActive(bool active, size_t style)
{
    this->m_active = active;
    CalculateOffsets(style);
}

bool clTabCtrl::ShiftRight(clTabInfo::Vec_t& tabs)
{
    // Move the first tab from the list and adjust the remainder
    // of the tabs x coordiate
    if(!tabs.empty()) {
        clTabInfo::Ptr_t t = tabs.at(0);
        int width = t->GetWidth();
        tabs.erase(tabs.begin() + 0);

        for(size_t i = 0; i < tabs.size(); ++i) {
            clTabInfo::Ptr_t t = tabs.at(i);
            t->GetRect().SetX(t->GetRect().x - width + clTabInfo::MAJOR_CURVE_WIDTH);
        }
        return true;
    }
    return false;
}

bool clTabCtrl::IsActiveTabInList(const clTabInfo::Vec_t& tabs) const
{
    for(size_t i = 0; i < tabs.size(); ++i) {
        if(tabs.at(i)->IsActive()) return true;
    }
    return false;
}

bool clTabCtrl::IsActiveTabVisible(const clTabInfo::Vec_t& tabs) const
{
    wxRect clientRect(GetClientRect());
    for(size_t i = 0; i < tabs.size(); ++i) {
        clTabInfo::Ptr_t t = tabs.at(i);
        if(t->IsActive() && clientRect.Intersects(t->GetRect())) return true;
    }
    return false;
}

//----------------------------------------------------------
// Notebook header
//----------------------------------------------------------
clTabCtrl::clTabCtrl(wxWindow* notebook, size_t style)
    : wxPanel(notebook)
    , m_height(clTabInfo::TAB_HEIGHT)
    , m_style(style)
    , m_closeButtonClickedIndex(wxNOT_FOUND)
    , m_contextMenu(NULL)
{
    SetSizeHints(wxSize(-1, m_height));
    SetSize(-1, m_height);
    Bind(wxEVT_PAINT, &clTabCtrl::OnPaint, this);
    Bind(wxEVT_ERASE_BACKGROUND, &clTabCtrl::OnEraseBG, this);
    Bind(wxEVT_SIZE, &clTabCtrl::OnSize, this);
    Bind(wxEVT_LEFT_DOWN, &clTabCtrl::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &clTabCtrl::OnLeftUp, this);
    Bind(wxEVT_MOTION, &clTabCtrl::OnMouseMotion, this);
    Bind(wxEVT_MIDDLE_UP, &clTabCtrl::OnMouseMiddleClick, this);
    Bind(wxEVT_CONTEXT_MENU, &clTabCtrl::OnContextMenu, this);
    if(m_style & kNotebook_DarkTabs) {
        m_colours.InitDarkColours();
    } else {
        m_colours.InitLightColours();
    }
}

clTabCtrl::~clTabCtrl()
{
    // wxDELETE(m_contextMenu);
    Unbind(wxEVT_PAINT, &clTabCtrl::OnPaint, this);
    Unbind(wxEVT_ERASE_BACKGROUND, &clTabCtrl::OnEraseBG, this);
    Unbind(wxEVT_SIZE, &clTabCtrl::OnSize, this);
    Unbind(wxEVT_LEFT_DOWN, &clTabCtrl::OnLeftDown, this);
    Unbind(wxEVT_LEFT_UP, &clTabCtrl::OnLeftUp, this);
    Unbind(wxEVT_MOTION, &clTabCtrl::OnMouseMotion, this);
    Unbind(wxEVT_MIDDLE_UP, &clTabCtrl::OnMouseMiddleClick, this);
    Unbind(wxEVT_CONTEXT_MENU, &clTabCtrl::OnContextMenu, this);
}

void clTabCtrl::OnSize(wxSizeEvent& event)
{
    event.Skip();
    m_visibleTabs.clear();
    Refresh();
}

void clTabCtrl::OnEraseBG(wxEraseEvent& e) { wxUnusedVar(e); }

void clTabCtrl::OnPaint(wxPaintEvent& e)
{
    wxBufferedPaintDC dc(this);
    wxRect clientRect(GetClientRect());
    if(clientRect.width <= 3) return;

    m_chevronRect = wxRect();
    wxRect rect(GetClientRect());
    rect.Deflate(0);

    if(GetStyle() & kNotebook_ShowFileListButton) {
        // Reduce the length of the tabs bitmap by 16 pixels (we will draw there the drop down
        // button)
        rect.SetWidth(rect.GetWidth() - 16);
        m_chevronRect = wxRect(rect.GetTopRight(), wxSize(16, rect.GetHeight()));
    }

    // Draw background
    dc.SetPen(m_colours.tabAreaColour);
    dc.SetBrush(m_colours.tabAreaColour);
    dc.DrawRectangle(GetClientRect());

    for(size_t i = 0; i < m_tabs.size(); ++i) {
        m_tabs.at(i)->CalculateOffsets(GetStyle());
    }

    if(rect.GetSize().x > 0) {
        wxBitmap bmpTabs(rect.GetSize());
        wxMemoryDC memDC(bmpTabs);
        wxGCDC gcdc(memDC);

        gcdc.SetPen(m_colours.tabAreaColour);
        gcdc.SetBrush(m_colours.tabAreaColour);
        gcdc.DrawRectangle(rect.GetSize());

        UpdateVisibleTabs();

        int activeTabInex = wxNOT_FOUND;
        for(int i = (m_visibleTabs.size() - 1); i >= 0; --i) {
            clTabInfo::Ptr_t tab = m_visibleTabs.at(i);
            if(tab->IsActive()) {
                activeTabInex = i;
            }
            tab->Draw(gcdc, m_colours, m_style);
        }

        // Redraw the active tab
        if(activeTabInex != wxNOT_FOUND) {
            m_visibleTabs.at(activeTabInex)->Draw(gcdc, m_colours, m_style);
        }

        memDC.SelectObject(wxNullBitmap);
        dc.DrawBitmap(bmpTabs, 0, 0);

        if(activeTabInex != wxNOT_FOUND) {
            clTabInfo::Ptr_t activeTab = m_visibleTabs.at(activeTabInex);
            // Draw 3 lines at the bottom
            dc.SetPen(m_colours.activeTabPenColour);
            dc.SetBrush(m_colours.activeTabBgColour);
            wxPoint topLeft = clientRect.GetBottomLeft();
            wxSize rectSize(clientRect.width, clTabInfo::BOTTOM_AREA_HEIGHT);
            topLeft.y -= rectSize.GetHeight() - 1;
            wxRect bottomRect(topLeft, rectSize);
            // We intentionally move the rect out of the client rect
            // so the left and right lines will be drawn out of screen
            bottomRect.x -= 1;
            bottomRect.width += 2;
            dc.DrawRectangle(bottomRect);

            // Draw a line under the active tab
            // that will erase the line drawn by the above rect
            wxPoint from, to;
            from = activeTab->GetRect().GetBottomLeft();
            to = activeTab->GetRect().GetBottomRight();
            from.y -= clTabInfo::BOTTOM_AREA_HEIGHT - 1;
            from.x += 2;
            to.y -= clTabInfo::BOTTOM_AREA_HEIGHT - 1;
            to.x -= 1;

            dc.SetPen(m_colours.activeTabBgColour);
            dc.DrawLine(from, to);
#ifdef __WXOSX__
            dc.DrawLine(from, to);
            dc.DrawLine(from, to);
            dc.DrawLine(from, to);
#endif
        }

        if(GetStyle() & kNotebook_ShowFileListButton) {
            // Draw the chevron
            wxCoord chevronX =
                m_chevronRect.GetTopLeft().x + ((m_chevronRect.GetWidth() - m_colours.chevronDown.GetWidth()) / 2);
            wxCoord chevronY =
                m_chevronRect.GetTopLeft().y + ((m_chevronRect.GetHeight() - m_colours.chevronDown.GetHeight()) / 2);
            dc.DrawBitmap(m_colours.chevronDown, chevronX, chevronY);
        }
    } else {
        m_visibleTabs.clear();
    }
}

void clTabCtrl::DoUpdateCoordiantes(clTabInfo::Vec_t& tabs)
{
    int xx = 5;
    for(size_t i = 0; i < tabs.size(); ++i) {
        clTabInfo::Ptr_t tab = tabs.at(i);
        tab->GetRect().SetX(xx);
        tab->GetRect().SetY(0);
        tab->GetRect().SetWidth(tab->GetWidth());
        tab->GetRect().SetHeight(tab->GetHeight());
        xx += tab->GetWidth() - clTabInfo::MAJOR_CURVE_WIDTH;
    }
}

void clTabCtrl::UpdateVisibleTabs()
{
    // don't update the list if we don't need to
    if(IsActiveTabInList(m_visibleTabs) && IsActiveTabVisible(m_visibleTabs)) return;

    // set the physical coords for each tab (we do this for all the tabs)
    DoUpdateCoordiantes(m_tabs);

    // Start shifting right tabs until the active tab is visible
    m_visibleTabs = m_tabs;
    while(!IsActiveTabVisible(m_visibleTabs)) {
        if(!ShiftRight(m_visibleTabs)) break;
    }
}

void clTabCtrl::OnLeftDown(wxMouseEvent& event)
{
    event.Skip();
    m_closeButtonClickedIndex = wxNOT_FOUND;

    if((GetStyle() & kNotebook_ShowFileListButton) && m_chevronRect.Contains(event.GetPosition())) {
        // we will handle this later in the "Mouse Up" event
        return;
    }

    // Get list of visible tabs
    bool clickWasOnActiveTab = false;
    for(size_t i = 0; i < m_visibleTabs.size(); ++i) {
        clTabInfo::Ptr_t t = m_visibleTabs.at(i);
        if(t->GetRect().Contains(event.GetPosition()) && !t->IsActive()) {
            SetSelection(DoGetPageIndex(t->GetWindow()));
            return;

        } else if(t->GetRect().Contains(event.GetPosition())) {
            clickWasOnActiveTab = true;
            break;
        }
    }

    {
        int tabHit, realPos;
        TestPoint(event.GetPosition(), realPos, tabHit);
        if(tabHit != wxNOT_FOUND) {
            if((GetStyle() & kNotebook_CloseButtonOnActiveTab) && m_visibleTabs.at(tabHit)->IsActive()) {
                // we clicked on the selected index
                clTabInfo::Ptr_t t = m_visibleTabs.at(tabHit);
                wxRect xRect(t->GetRect().x + t->GetBmpCloseX(), t->GetRect().y + t->GetBmpCloseY(), 16, 16);
                if(xRect.Contains(event.GetPosition())) {
                    m_closeButtonClickedIndex = tabHit;
                }
            }
        }
    }
    // We clicked on the active tab, start DnD operation
    if((m_style & kNotebook_AllowDnD) && clickWasOnActiveTab) {
        // TO-DO :: implement DnD here
    }
}

int clTabCtrl::ChangeSelection(size_t tabIdx)
{
    wxWindowUpdateLocker locker(GetParent());
    int oldSelection = GetSelection();
    if(!IsIndexValid(tabIdx)) return oldSelection;

    for(size_t i = 0; i < m_tabs.size(); ++i) {
        clTabInfo::Ptr_t tab = m_tabs.at(i);
        tab->SetActive((i == tabIdx), GetStyle());
    }

    clTabInfo::Ptr_t activeTab = GetActiveTabInfo();
    if(activeTab) {
        static_cast<Notebook*>(GetParent())->DoChangeSelection(activeTab->GetWindow());
    }
    Refresh();
    return oldSelection;
}

int clTabCtrl::SetSelection(size_t tabIdx)
{
    int oldSelection = GetSelection();
    {
        wxBookCtrlEvent event(wxEVT_BOOK_PAGE_CHANGING);
        event.SetEventObject(this);
        event.SetSelection(oldSelection);
        event.SetOldSelection(wxNOT_FOUND);
        GetParent()->GetEventHandler()->ProcessEvent(event);

        if(!event.IsAllowed()) {
            return oldSelection; // Vetoed by the user
        }
    }
    ChangeSelection(tabIdx);

    // Fire an event
    {
        wxBookCtrlEvent event(wxEVT_BOOK_PAGE_CHANGED);
        event.SetEventObject(this);
        event.SetSelection(GetSelection());
        event.SetOldSelection(oldSelection);
        GetParent()->GetEventHandler()->AddPendingEvent(event);
    }
    return oldSelection;
}

int clTabCtrl::GetSelection() const
{
    for(size_t i = 0; i < m_tabs.size(); ++i) {
        clTabInfo::Ptr_t tab = m_tabs.at(i);
        if(tab->IsActive()) return i;
    }
    return wxNOT_FOUND;
}

clTabInfo::Ptr_t clTabCtrl::GetTabInfo(size_t index)
{
    if(!IsIndexValid(index)) return clTabInfo::Ptr_t(NULL);
    return m_tabs.at(index);
}

clTabInfo::Ptr_t clTabCtrl::GetTabInfo(size_t index) const
{
    if(!IsIndexValid(index)) return clTabInfo::Ptr_t(NULL);
    return m_tabs.at(index);
}

clTabInfo::Ptr_t clTabCtrl::GetTabInfo(wxWindow* page)
{
    for(size_t i = 0; i < m_tabs.size(); ++i) {
        clTabInfo::Ptr_t tab = m_tabs.at(i);
        if(tab->GetWindow() == page) return tab;
    }
    return clTabInfo::Ptr_t(NULL);
}

bool clTabCtrl::SetPageText(size_t page, const wxString& text)
{
    clTabInfo::Ptr_t tab = GetTabInfo(page);
    if(!tab) return false;
    tab->SetLabel(text, GetStyle());
    Refresh();
    return true;
}

clTabInfo::Ptr_t clTabCtrl::GetActiveTabInfo()
{
    for(size_t i = 0; i < m_tabs.size(); ++i) {
        if(m_tabs.at(i)->IsActive()) {
            return m_tabs.at(i);
        }
    }
    return clTabInfo::Ptr_t(NULL);
}

void clTabCtrl::AddPage(clTabInfo::Ptr_t tab) { InsertPage(m_tabs.size(), tab); }

wxSimplebook* clTabCtrl::GetBook() { return reinterpret_cast<Notebook*>(GetParent())->m_book; }

bool clTabCtrl::InsertPage(size_t index, clTabInfo::Ptr_t tab)
{
    if(index > m_tabs.size()) return false;
    m_tabs.insert(m_tabs.begin() + index, tab);

    int tabIndex = index;
    tab->GetWindow()->Reparent(GetBook());
    GetBook()->InsertPage(index, tab->GetWindow(), tab->GetLabel(), tab->IsActive());
    clTabInfo::Ptr_t activeTab = GetActiveTabInfo();
    if(!activeTab) {
        // No active tab??
        ChangeSelection(0);
    }
    if(tab->IsActive()) {
        ChangeSelection(tabIndex);
    }
    Refresh();
    return true;
}

wxString clTabCtrl::GetPageText(size_t page) const
{
    clTabInfo::Ptr_t tab = GetTabInfo(page);
    if(tab) return tab->GetLabel();
    return "";
}

wxBitmap clTabCtrl::GetPageBitmap(size_t index) const
{
    clTabInfo::Ptr_t tab = GetTabInfo(index);
    if(tab) return tab->GetBitmap();
    return wxNullBitmap;
}

void clTabCtrl::SetPageBitmap(size_t index, const wxBitmap& bmp)
{
    clTabInfo::Ptr_t tab = GetTabInfo(index);
    if(tab) {
        tab->SetBitmap(bmp, GetStyle());
        Refresh();
    }
}

void clTabCtrl::OnLeftUp(wxMouseEvent& event)
{
    event.Skip();

    // First check if the chevron was clicked. We do this because the chevron could overlap the buttons drawing
    // area
    if((GetStyle() & kNotebook_ShowFileListButton) && m_chevronRect.Contains(event.GetPosition())) {
        // Show the drop down list
        CallAfter(&clTabCtrl::DoShowTabList);

    } else {
        int tabHit, realPos;
        TestPoint(event.GetPosition(), realPos, tabHit);
        if(tabHit != wxNOT_FOUND) {
            if((GetStyle() & kNotebook_CloseButtonOnActiveTab) && m_visibleTabs.at(tabHit)->IsActive()) {
                // we clicked on the selected index
                clTabInfo::Ptr_t t = m_visibleTabs.at(tabHit);
                wxRect xRect(t->GetRect().x + t->GetBmpCloseX(), t->GetRect().y + t->GetBmpCloseY(), 16, 16);
                xRect.Inflate(2); // don't be picky if we did not click exactly on the 16x16 bitmap...

                if(m_closeButtonClickedIndex == tabHit && xRect.Contains(event.GetPosition())) {
                    CallAfter(&clTabCtrl::DoDeletePage, realPos);
                }
            }
        }
    }
}

void clTabCtrl::OnMouseMotion(wxMouseEvent& event) { event.Skip(); }

void clTabCtrl::TestPoint(const wxPoint& pt, int& realPosition, int& tabHit)
{
    realPosition = wxNOT_FOUND;
    tabHit = wxNOT_FOUND;

    if(m_visibleTabs.empty()) return;

    for(size_t i = 0; i < m_visibleTabs.size(); ++i) {
        clTabInfo::Ptr_t tab = m_visibleTabs.at(i);
        if(tab->GetRect().Contains(pt)) {
            tabHit = i;
            realPosition = DoGetPageIndex(tab->GetWindow());
            return;
        }
    }
}

void clTabCtrl::SetStyle(size_t style)
{
    this->m_style = style;
    if(style & kNotebook_DarkTabs) {
        m_colours.InitDarkColours();
    } else {
        m_colours.InitLightColours();
    }

    for(size_t i = 0; i < m_tabs.size(); ++i) {
        m_tabs.at(i)->CalculateOffsets(GetStyle());
    }
    m_visibleTabs.clear();
    Refresh();
}

wxWindow* clTabCtrl::GetPage(size_t index) const
{
    clTabInfo::Ptr_t tab = GetTabInfo(index);
    if(tab) return tab->GetWindow();
    return NULL;
}

bool clTabCtrl::IsIndexValid(size_t index) const { return (index < m_tabs.size()); }

clTabInfo::Colours::Colours() { InitDarkColours(); }

void clTabInfo::Colours::InitDarkColours()
{
    activeTabTextColour = "WHITE";
    activeTabBgColour = wxColour("#211e1e");
    activeTabPenColour = wxColour("#0e0d0d");
    activeTabInnerPenColour = wxColour("#343131");

    inactiveTabTextColour = wxColour("rgb(200, 200, 200)");
    inactiveTabBgColour = wxColour("#393838");
    inactiveTabPenColour = wxColour("#100f0f");
    inactiveTabInnerPenColour = wxColour("#535252");

    tabAreaColour = wxColour("#131111");
    // 12x12 bitmap
    closeButton = wxXmlResource::Get()->LoadBitmap("notebook-dark-x");
    chevronDown = wxXmlResource::Get()->LoadBitmap("chevron-down-grey");
}

void clTabInfo::Colours::InitLightColours()
{
    activeTabTextColour = "#444444";
    activeTabBgColour = "#f0f0f0";
    activeTabPenColour = "#b9b9b9";
    activeTabInnerPenColour = "#ffffff";

    inactiveTabTextColour = "#444444";
    inactiveTabBgColour = "#e5e5e5";
    inactiveTabPenColour = "#b9b9b9";
    inactiveTabInnerPenColour = "#ffffff";

    tabAreaColour = "#dcdcdc"; // wxColour("rgb(64, 64, 64)");
    // 12x12 bitmap
    closeButton = wxXmlResource::Get()->LoadBitmap("notebook-light-x");
    chevronDown = wxXmlResource::Get()->LoadBitmap("chevron-down-black");
}

int clTabCtrl::FindPage(wxWindow* page) const
{
    for(size_t i = 0; i < m_tabs.size(); ++i) {
        if(m_tabs.at(i)->GetWindow() == page) {
            return i;
        }
    }
    return wxNOT_FOUND;
}

bool clTabCtrl::RemovePage(size_t page, bool notify, bool deletePage)
{
    int nextSelection = wxNOT_FOUND;
    if(!IsIndexValid(page)) return false;
    bool deletingSelection = ((int)page == GetSelection());

    if(notify) {
        wxBookCtrlEvent event(wxEVT_BOOK_PAGE_CLOSING);
        event.SetEventObject(this);
        event.SetSelection(page);
        GetParent()->GetEventHandler()->ProcessEvent(event);
        if(!event.IsAllowed()) {
            // Vetoed
            return false;
        }
    }

    // Remove the tab from the "all-tabs" list
    clTabInfo::Ptr_t tab = m_tabs.at(page);
    m_tabs.erase(m_tabs.begin() + page);

    // Remove the tabs from the visible tabs list
    clTabInfo::Vec_t::iterator iter =
        std::find_if(m_visibleTabs.begin(), m_visibleTabs.end(), [&](clTabInfo::Ptr_t t) {
            if(t->GetWindow() == tab->GetWindow()) {
                return true;
            }
            return false;
        });
    if(iter != m_visibleTabs.end()) {
        iter = m_visibleTabs.erase(iter);

        for(; iter != m_visibleTabs.end(); ++iter) {
            // update the remainding tabs coordinates
            (*iter)->GetRect().SetX((*iter)->GetRect().GetX() - tab->GetWidth() + clTabInfo::MAJOR_CURVE_WIDTH);
        }
    }

    // Choose a new selection, but only if we are deleting the active tab
    nextSelection = wxNOT_FOUND;
    if(deletingSelection) {
        nextSelection = page;
        if(!m_tabs.empty()) {
            if(nextSelection >= (int)m_tabs.size()) {
                --nextSelection;
            }

            // Ensure that the next selection is always valid
            if(nextSelection >= (int)m_tabs.size()) {
                nextSelection = 0;
            }
        } else {
            nextSelection = wxNOT_FOUND;
        }
    }

    // Now remove the page from the notebook. We will delete the page
    // ourself, so there is no need to call DeletePage
    int where = GetBook()->FindPage(tab->GetWindow());
    if(where != wxNOT_FOUND) {
        GetBook()->RemovePage(where);
    }

    if(deletePage) {
        // Destory the page
        tab->GetWindow()->Destroy();
    }

    if(notify) {
        wxBookCtrlEvent event(wxEVT_BOOK_PAGE_CLOSED);
        event.SetEventObject(this);
        event.SetSelection(GetSelection());
        GetParent()->GetEventHandler()->ProcessEvent(event);
        if(!event.IsAllowed()) {
            // Vetoed
            return false;
        }
    }

    // Choose the next page
    if(nextSelection != wxNOT_FOUND) {
        ChangeSelection(nextSelection);
        if(notify) {
            wxBookCtrlEvent event(wxEVT_BOOK_PAGE_CHANGED);
            event.SetEventObject(this);
            event.SetSelection(GetSelection());
            GetParent()->GetEventHandler()->AddPendingEvent(event);
        }
    } else {
        Refresh();
    }
    return true;
}

int clTabCtrl::DoGetPageIndex(wxWindow* win) const
{
    for(size_t i = 0; i < m_tabs.size(); ++i) {
        if(m_tabs.at(i)->GetWindow() == win) return i;
    }
    return wxNOT_FOUND;
}

bool clTabCtrl::DeleteAllPages()
{
    if(GetBook()->DeleteAllPages()) {
        m_tabs.clear();
        m_visibleTabs.clear();
    }
    Refresh();
    return true;
}

void clTabCtrl::OnMouseMiddleClick(wxMouseEvent& event)
{
    event.Skip();
    if(GetStyle() & kNotebook_MouseMiddleClickClosesTab) {
        int realPos, tabHit;
        TestPoint(event.GetPosition(), realPos, tabHit);
        if(realPos != wxNOT_FOUND) {
            CallAfter(&clTabCtrl::DoDeletePage, realPos);
        }
    }
}

void clTabCtrl::GetAllPages(std::vector<wxWindowMSW*>& pages)
{
    std::for_each(
        m_tabs.begin(), m_tabs.end(), [&](clTabInfo::Ptr_t tabInfo) { pages.push_back(tabInfo->GetWindow()); });
}

void clTabCtrl::SetMenu(wxMenu* menu) { m_contextMenu = menu; }

void clTabCtrl::OnContextMenu(wxContextMenuEvent& event)
{
    event.Skip();
    if(!m_contextMenu) return;

    wxPoint pt = ::wxGetMousePosition();
    pt = ScreenToClient(pt);
    int realPos, tabHit;
    TestPoint(pt, realPos, tabHit);

    if((realPos != wxNOT_FOUND) && (realPos == GetSelection())) {
        // Show context menu for active tabs only
        PopupMenu(m_contextMenu);
    }
}

void clTabCtrl::DoShowTabList()
{
    if(m_tabs.empty()) return;
    
    int curselection = GetSelection();
    wxMenu menu;
    const int firstTabPageID = 13457;
    int pageMenuID = firstTabPageID;
    for(size_t i = 0; i < m_tabs.size(); ++i) {
        clTabInfo::Ptr_t tab = m_tabs.at(i);
        wxMenuItem* item = new wxMenuItem(&menu, pageMenuID, tab->GetLabel(), "", wxITEM_NORMAL);
        menu.Append(item);
        if(tab->GetBitmap().IsOk()) {
            item->SetBitmap(tab->GetBitmap());
        }
        pageMenuID++;
    }

    int selection = GetPopupMenuSelectionFromUser(menu, m_chevronRect.GetBottomLeft());
    if(selection != wxID_NONE) {
        selection -= firstTabPageID;
        // don't change the selection unless the selection is really changing
        if(curselection != selection) {
            SetSelection(selection);
        }
    }
}

bool clTabCtrl::SetPageToolTip(size_t page, const wxString& tooltip)
{
    clTabInfo::Ptr_t tab = GetTabInfo(page);
    if(tab) {
        tab->SetTooltip(tooltip);
        return true;
    }
    return false;
}
