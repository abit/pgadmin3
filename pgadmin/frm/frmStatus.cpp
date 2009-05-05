//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// RCS-ID:      $Id$
// Copyright (C) 2002 - 2009, The pgAdmin Development Team
// This software is released under the BSD Licence
//
// frmStatus.cpp - Status Screen
//
//////////////////////////////////////////////////////////////////////////

#include "pgAdmin3.h"

// wxWindows headers
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#include <wx/image.h>
#include <wx/textbuf.h>
#include <wx/clipbrd.h>

// wxAUI
#include <wx/aui/aui.h>
#include <wx/aui/auibook.h>

// App headers
#include "frm/frmStatus.h"
#include "frm/frmHint.h"
#include "frm/frmMain.h"
#include "utils/pgfeatures.h"
#include "schema/pgServer.h"
#include "ctl/ctlMenuToolbar.h"

// Icons
#include "images/clip_copy.xpm"
#include "images/readdata.xpm"
#include "images/query_cancel.xpm"
#include "images/terminate_backend.xpm"
#include "images/delete.xpm"
#include "images/storedata.xpm"


BEGIN_EVENT_TABLE(frmStatus, pgFrame)
    EVT_MENU(MNU_EXIT,                               frmStatus::OnExit)
    
    EVT_MENU(MNU_COPY,                               frmStatus::OnCopy)

    EVT_MENU(MNU_HELP,                               frmStatus::OnHelp)
    EVT_MENU(MNU_STATUSPAGE,                        frmStatus::OnToggleStatusPane)
    EVT_MENU(MNU_LOCKPAGE,                            frmStatus::OnToggleLockPane)
    EVT_MENU(MNU_XACTPAGE,                             frmStatus::OnToggleXactPane)
    EVT_MENU(MNU_LOGPAGE,                             frmStatus::OnToggleLogPane)
    EVT_MENU(MNU_TOOLBAR,                             frmStatus::OnToggleToolBar)
    EVT_MENU(MNU_DEFAULTVIEW,                        frmStatus::OnDefaultView)

    EVT_AUI_PANE_CLOSE(                             frmStatus::OnPaneClose)
    
    EVT_COMBOBOX(CTL_RATECBO,                       frmStatus::OnRateChange)
    EVT_MENU(MNU_REFRESH,                             frmStatus::OnRefresh)
    EVT_MENU(MNU_CANCEL,                             frmStatus::OnCancelBtn)
    EVT_MENU(MNU_TERMINATE,                              frmStatus::OnTerminateBtn)
    EVT_MENU(MNU_COMMIT,                            frmStatus::OnCommit)
    EVT_MENU(MNU_ROLLBACK,                            frmStatus::OnRollback)
    EVT_COMBOBOX(CTL_LOGCBO,                        frmStatus::OnLoadLogfile)
    EVT_BUTTON(CTL_ROTATEBTN,                       frmStatus::OnRotateLogfile)
    
    EVT_TIMER(TIMER_REFRESHUI_ID,                    frmStatus::OnRefreshUITimer)

    EVT_TIMER(TIMER_STATUS_ID,                        frmStatus::OnRefreshStatusTimer)
    EVT_LIST_ITEM_SELECTED(CTL_STATUSLIST,            frmStatus::OnSelStatusItem)
    EVT_LIST_ITEM_DESELECTED(CTL_STATUSLIST,        frmStatus::OnSelStatusItem)
    
    EVT_TIMER(TIMER_LOCKS_ID,                        frmStatus::OnRefreshLocksTimer)
    EVT_LIST_ITEM_SELECTED(CTL_LOCKLIST,            frmStatus::OnSelLockItem)
    EVT_LIST_ITEM_DESELECTED(CTL_LOCKLIST,            frmStatus::OnSelLockItem)
    
    EVT_TIMER(TIMER_XACT_ID,                        frmStatus::OnRefreshXactTimer)
    EVT_LIST_ITEM_SELECTED(CTL_XACTLIST,            frmStatus::OnSelXactItem)
    EVT_LIST_ITEM_DESELECTED(CTL_XACTLIST,            frmStatus::OnSelXactItem)
    
    EVT_TIMER(TIMER_LOG_ID,                            frmStatus::OnRefreshLogTimer)
    EVT_LIST_ITEM_SELECTED(CTL_LOGLIST,                frmStatus::OnSelLogItem)
    EVT_LIST_ITEM_DESELECTED(CTL_LOGLIST,            frmStatus::OnSelLogItem)

    EVT_CLOSE(                                        frmStatus::OnClose)
END_EVENT_TABLE();


int frmStatus::cboToRate()
{
    int rate = 0;

    if (cbRate->GetValue() == _("Don't refresh"))
        rate = 0;
    if (cbRate->GetValue() == _("1 second"))
        rate = 1;
    if (cbRate->GetValue() == _("5 seconds"))
        rate = 5;
    if (cbRate->GetValue() == _("10 seconds"))
        rate = 10;
    if (cbRate->GetValue() == _("30 seconds"))
        rate = 30;
    if (cbRate->GetValue() == _("1 minute"))
        rate = 60;
    if (cbRate->GetValue() == _("5 minutes"))
        rate = 300;
    if (cbRate->GetValue() == _("10 minutes"))
        rate = 600;
    if (cbRate->GetValue() == _("30 minutes"))
        rate = 1800;
    if (cbRate->GetValue() == _("1 hour"))
        rate = 3600;

    return rate;
}


wxString frmStatus::rateToCboString(int rate)
{
    wxString rateStr;

    if (rate == 0)
        rateStr = _("Don't refresh");
    if (rate == 1)
        rateStr = _("1 second");
    if (rate == 5)
        rateStr = _("5 seconds");
    if (rate == 10)
        rateStr = _("10 seconds");
    if (rate == 30)
        rateStr = _("30 seconds");
    if (rate == 60)
        rateStr = _("1 minute");
    if (rate == 300)
        rateStr = _("5 minutes");
    if (rate == 600)
        rateStr = _("10 minutes");
    if (rate == 1800)
        rateStr = _("30 minutes");
    if (rate == 3600)
        rateStr = _("1 hour");

    return rateStr;
}


frmStatus::frmStatus(frmMain *form, const wxString& _title, pgConn *conn) : pgFrame(NULL, _title)
{
    dlgName = wxT("frmStatus");
    
    loaded = false;

    mainForm = form;
    connection = conn;

    statusTimer = 0;
    locksTimer = 0;
    xactTimer = 0;
    logTimer = 0;
    
    logHasTimestamp = false;
    logFormatKnown = false;

    // Notify wxAUI which frame to use
    manager.SetManagedWindow(this);
    manager.SetFlags(wxAUI_MGR_DEFAULT | wxAUI_MGR_TRANSPARENT_DRAG | wxAUI_MGR_ALLOW_ACTIVE_PANE);

    // Set different window's attributes
    SetTitle(_title);
    appearanceFactory->SetIcons(this);
    RestorePosition(-1, -1, 700, 500, 700, 500);
    SetMinSize(wxSize(700,500));
    wxWindowBase::SetFont(settings->GetSystemFont());

    // Build menu bar
    menuBar = new wxMenuBar();

    fileMenu = new wxMenu();
    fileMenu->Append(MNU_EXIT, _("E&xit\tAlt-F4"), _("Exit query window"));

    menuBar->Append(fileMenu, _("&File"));

    editMenu = new wxMenu();
    editMenu->Append(MNU_COPY, _("&Copy\tCtrl-C"), _("Copy selected text to clipboard"), wxITEM_NORMAL);

    menuBar->Append(editMenu, _("&Edit"));

    viewMenu = new wxMenu();
    viewMenu->Append(MNU_STATUSPAGE, _("&Activity\tCtrl-Alt-A"), _("Show or hide the activity tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_LOCKPAGE, _("&Locks\tCtrl-Alt-L"), _("Show or hide the locks tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_XACTPAGE, _("&Transactions\tCtrl-Alt-T"), _("Show or hide the transactions tab."), wxITEM_CHECK);
    viewMenu->Append(MNU_LOGPAGE, _("Log&file\tCtrl-Alt-F"), _("Show or hide the logfile tab."), wxITEM_CHECK);
    viewMenu->AppendSeparator();
    viewMenu->Append(MNU_TOOLBAR, _("Tool&bar\tCtrl-Alt-B"), _("Show or hide the toolbar."), wxITEM_CHECK);
    viewMenu->AppendSeparator();
    viewMenu->Append(MNU_DEFAULTVIEW, _("&Default view\tCtrl-Alt-V"), _("Restore the default view."));

    menuBar->Append(viewMenu, _("&View"));

    wxMenu *helpMenu=new wxMenu();
    helpMenu->Append(MNU_CONTENTS, _("&Help"), _("Open the helpfile."));

    menuBar->Append(helpMenu, _("&Help"));
    
    // Setup edit menu
    editMenu->Enable(MNU_COPY, false);

    // Finish menu bar
    SetMenuBar(menuBar);
    
    // Set statusBar
    statusBar = CreateStatusBar(1);
    SetStatusBarPane(-1);
    
    // Set up toolbar
    toolBar = new ctlMenuToolbar(this, -1, wxDefaultPosition, wxDefaultSize, wxTB_FLAT | wxTB_NODIVIDER);
    toolBar->SetToolBitmapSize(wxSize(16, 16));
    toolBar->AddTool(MNU_REFRESH, _("Refresh"), wxBitmap(readdata_xpm), _("Refresh"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    toolBar->AddTool(MNU_COPY, _("Copy"), wxBitmap(clip_copy_xpm), _("Copy selected text to clipboard"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    toolBar->AddTool(MNU_CANCEL, _("Cancel"), wxBitmap(query_cancel_xpm), _("Cancel query"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_TERMINATE, _("Terminate"), wxBitmap(terminate_backend_xpm), _("Terminate backend"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_COMMIT, _("Commit"), wxBitmap(storedata_xpm), _("Commit transaction"), wxITEM_NORMAL);
    toolBar->AddTool(MNU_ROLLBACK, _("Rollback"), wxBitmap(delete_xpm), _("Rollback transaction"), wxITEM_NORMAL);
    toolBar->AddSeparator();
    cbLogfiles = new wxComboBox(toolBar, CTL_LOGCBO, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL,
    wxCB_READONLY|wxCB_DROPDOWN);
    toolBar->AddControl(cbLogfiles);
    btnRotateLog = new wxButton(toolBar, CTL_ROTATEBTN, _("Rotate"));
    toolBar->AddControl(btnRotateLog);
    toolBar->AddSeparator();
    cbRate = new ctlComboBoxFix(toolBar, CTL_RATECBO, wxDefaultPosition, wxSize(-1, -1), wxCB_READONLY|wxCB_DROPDOWN);
    toolBar->AddControl(cbRate);
    toolBar->Realize();

    // Append items to cbo
    cbRate->Append(_("Don't refresh"));
    cbRate->Append(_("1 second"));
    cbRate->Append(_("5 seconds"));
    cbRate->Append(_("10 seconds"));
    cbRate->Append(_("30 seconds"));
    cbRate->Append(_("1 minute"));
    cbRate->Append(_("5 minutes"));
    cbRate->Append(_("10 minutes"));
    cbRate->Append(_("30 minutes"));
    cbRate->Append(_("1 hour"));
    
    // Disable toolbar's items
    toolBar->EnableTool(MNU_CANCEL, false);
    toolBar->EnableTool(MNU_TERMINATE, false);
    toolBar->EnableTool(MNU_COMMIT, false);
    toolBar->EnableTool(MNU_ROLLBACK, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);
    
    // Create panel
    AddStatusPane();
    AddLockPane();
    AddXactPane();
    AddLogPane();
    manager.AddPane(toolBar, wxAuiPaneInfo().Name(wxT("toolBar")).Caption(_("Tool bar")).ToolbarPane().Top().LeftDockable(false).RightDockable(false));

    // Now load the layout
    wxString perspective;
    settings->Read(wxT("frmStatus/Perspective-") + VerFromRev(FRMSTATUS_PERSPECTIVE_VER), &perspective, FRMSTATUS_DEFAULT_PERSPECTIVE);
    manager.LoadPerspective(perspective, true);

    // Reset the captions for the current language
    manager.GetPane(wxT("toolBar")).Caption(_("Tool bar"));
    manager.GetPane(wxT("Activity")).Caption(_("Activity"));
    manager.GetPane(wxT("Locks")).Caption(_("Locks"));
    manager.GetPane(wxT("Transactions")).Caption(_("Transactions"));
    manager.GetPane(wxT("Logfile")).Caption(_("Logfile"));

    // XACT and LOG panes must be hidden once again if server doesn't deal with them
    if (!(connection->BackendMinimumVersion(8, 0) && 
         connection->HasFeature(FEATURE_FILEREAD)))
    {
        manager.GetPane(wxT("Logfile")).Show(false);
    }
    if (!connection->BackendMinimumVersion(8, 1))
    {
        manager.GetPane(wxT("Transactions")).Show(false);
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();

    // Sync the View menu options
    viewMenu->Check(MNU_STATUSPAGE, manager.GetPane(wxT("Activity")).IsShown());
    viewMenu->Check(MNU_LOCKPAGE, manager.GetPane(wxT("Locks")).IsShown());
    viewMenu->Check(MNU_XACTPAGE, manager.GetPane(wxT("Transactions")).IsShown());
    viewMenu->Check(MNU_LOGPAGE, manager.GetPane(wxT("Logfile")).IsShown());
    viewMenu->Check(MNU_TOOLBAR, manager.GetPane(wxT("toolBar")).IsShown());

    // Get our PID
    backend_pid = connection->GetBackendPID();
    
    // Create the refresh timer (quarter of a second)
    // This is a horrible hack to get around the lack of a 
    // PANE_ACTIVATED event in wxAUI.
    refreshUITimer = new wxTimer(this, TIMER_REFRESHUI_ID);
    refreshUITimer->Start(250);

    // We're good now
    loaded = true;
}


frmStatus::~frmStatus()
{
    // Delete the refresh timer
    delete refreshUITimer;
    
    // If the status window wasn't launched in standalone mode...
    if (mainForm)
        mainForm->RemoveFrame(this);

    // Save the window's position
    settings->Write(wxT("frmStatus/Perspective-") + VerFromRev(FRMSTATUS_PERSPECTIVE_VER), manager.SavePerspective());
    manager.UnInit();
    SavePosition();
    
    // For each current page, save the slider's position and delete the timer
    settings->Write(wxT("frmStatus/RefreshStatusRate"), statusRate);
    delete statusTimer;
    settings->Write(wxT("frmStatus/RefreshLockRate"), locksRate);
    delete locksTimer;
    if (viewMenu->IsEnabled(MNU_XACTPAGE))
    {
        settings->Write(wxT("frmStatus/RefreshXactRate"), xactRate);
        delete xactTimer;
    }
    if (viewMenu->IsEnabled(MNU_LOGPAGE))
    {
        settings->Write(wxT("frmStatus/RefreshLogRate"), logRate);
        emptyLogfileCombo();
        delete logTimer;
    }

    // If connection is still available, delete it
    if (connection)
        delete connection;
}


void frmStatus::Go()
{
    // Show the window
    Show(true);
    
    // Send RateChange event to launch each timer
    wxScrollEvent nullScrollEvent;
    if (viewMenu->IsChecked(MNU_STATUSPAGE))
    {
        currentPane = PANE_STATUS;
        cbRate->SetValue(rateToCboString(statusRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsChecked(MNU_LOCKPAGE))
    {
        currentPane = PANE_LOCKS;
        cbRate->SetValue(rateToCboString(locksRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsEnabled(MNU_XACTPAGE) && viewMenu->IsChecked(MNU_XACTPAGE))
    {
        currentPane = PANE_XACT;
        cbRate->SetValue(rateToCboString(xactRate));
        OnRateChange(nullScrollEvent);
    }
    if (viewMenu->IsEnabled(MNU_LOGPAGE) && viewMenu->IsChecked(MNU_LOGPAGE))
    {
        currentPane = PANE_LOG;
        cbRate->SetValue(rateToCboString(logRate));
        OnRateChange(nullScrollEvent);
    }

    // Refresh all pages
    wxCommandEvent nullEvent;
    OnRefresh(nullEvent);
}


void frmStatus::OnClose(wxCloseEvent &event)
{
    Destroy();
}


void frmStatus::OnExit(wxCommandEvent& event)
{
    Destroy();
}


void frmStatus::AddStatusPane()
{
    // Create panel
    wxPanel *pnlActivity = new wxPanel(this);
    
    // Create flex grid
    wxFlexGridSizer *grdActivity = new wxFlexGridSizer(1, 1, 5, 5);
    grdActivity->AddGrowableCol(0);
    grdActivity->AddGrowableRow(0);

    // Add the list control
    wxListCtrl *lstStatus = new wxListCtrl(pnlActivity, CTL_STATUSLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER);
    grdActivity->Add(lstStatus, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlActivity,
      wxAuiPaneInfo().
      Name(wxT("Activity")).Caption(_("Activity")).
      CaptionVisible(true).CloseButton(true).MaximizeButton(true).
      Dockable(true).Movable(true));
    
    // Auto-sizing
    pnlActivity->SetSizer(grdActivity);
    grdActivity->Fit(pnlActivity);
    
    // Add each column to the list control
    statusList = (ctlListView*)lstStatus;
    statusList->AddColumn(wxT("PID"), 35);
    statusList->AddColumn(_("Database"), 70);
    statusList->AddColumn(_("User"), 70);
    if (connection->BackendMinimumVersion(8, 1))
    {
        statusList->AddColumn(_("Client"), 70);
        statusList->AddColumn(_("Client start"), 80);
    }
    if (connection->BackendMinimumVersion(7, 4))
        statusList->AddColumn(_("Query start"), 50);
    if (connection->BackendMinimumVersion(8, 3))
        statusList->AddColumn(_("TX start"), 50);
    statusList->AddColumn(_("Blocked by"), 35);
    statusList->AddColumn(_("Query"), 500);

    // Read statusRate configuration
    settings->Read(wxT("frmStatus/RefreshStatusRate"), &statusRate, 10);
    
    // Create the timer
    statusTimer = new wxTimer(this, TIMER_STATUS_ID);
}


void frmStatus::AddLockPane()
{
    // Create panel
    wxPanel *pnlLock = new wxPanel(this);
    
    // Create flex grid
    wxFlexGridSizer *grdLock = new wxFlexGridSizer(1, 1, 5, 5);
    grdLock->AddGrowableCol(0);
    grdLock->AddGrowableRow(0);

    // Add the list control
    wxListCtrl *lstLocks = new wxListCtrl(pnlLock, CTL_LOCKLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER);
    grdLock->Add(lstLocks, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlLock,
      wxAuiPaneInfo().
      Name(wxT("Locks")).Caption(_("Locks")).
      CaptionVisible(true).CloseButton(true).MaximizeButton(true).
      Dockable(true).Movable(true));
    
    // Auto-sizing
    pnlLock->SetSizer(grdLock);
    grdLock->Fit(pnlLock);

    // Add each column to the list control
    lockList = (ctlListView*)lstLocks;
    lockList->AddColumn(wxT("PID"), 35);
    lockList->AddColumn(_("Database"), 50);
    lockList->AddColumn(_("Relation"), 50);
    lockList->AddColumn(_("User"), 50);
    if (connection->BackendMinimumVersion(8, 3))
        lockList->AddColumn(_("XID"), 50);
    lockList->AddColumn(_("TX"), 50);
    lockList->AddColumn(_("Mode"), 50);
    lockList->AddColumn(_("Granted"), 50);
    if (connection->BackendMinimumVersion(7, 4))
        lockList->AddColumn(_("Start"), 50);
    lockList->AddColumn(_("Query"), 500);

    // Read locksRate configuration
    settings->Read(wxT("frmStatus/RefreshLockRate"), &locksRate, 10);
    
    // Create the timer
    locksTimer = new wxTimer(this, TIMER_LOCKS_ID);
}


void frmStatus::AddXactPane()
{
    // Create panel
    wxPanel *pnlXacts = new wxPanel(this);
    
    // Create flex grid
    wxFlexGridSizer *grdXacts = new wxFlexGridSizer(1, 1, 5, 5);
    grdXacts->AddGrowableCol(0);
    grdXacts->AddGrowableRow(0);

    // Add the list control
    wxListCtrl *lstXacts = new wxListCtrl(pnlXacts, CTL_LOCKLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER);
    grdXacts->Add(lstXacts, 0, wxGROW, 3);

    // Add the panel to the notebook
    manager.AddPane(pnlXacts,
      wxAuiPaneInfo().
      Name(wxT("Transactions")).Caption(_("Transactions")).
      CaptionVisible(true).CloseButton(true).MaximizeButton(true).
      Dockable(true).Movable(true));

    // Auto-sizing
    pnlXacts->SetSizer(grdXacts);
    grdXacts->Fit(pnlXacts);

    // Add the xact list
    xactList = (ctlListView*)lstXacts;

    // We don't need this report if server release is less than 8.1
    if (!connection->BackendMinimumVersion(8, 1))
    {
        // Uncheck and disable menu        
        viewMenu->Enable(MNU_XACTPAGE, false);
        viewMenu->Check(MNU_XACTPAGE, false);
        manager.GetPane(wxT("Transactions")).Show(false);
        
        // We're done
        return;
    }

    // Add each column to the list control
    xactList->AddColumn(wxT("XID"), 50);
    xactList->AddColumn(_("Global ID"), 200);
    xactList->AddColumn(_("Time"), 100);
    xactList->AddColumn(_("Owner"), 50);
    xactList->AddColumn(_("Database"), 50);

    // Read xactRate configuration
    settings->Read(wxT("frmStatus/RefreshXactRate"), &xactRate, 10);
    
    // Create the timer
    xactTimer = new wxTimer(this, TIMER_XACT_ID);
}


void frmStatus::AddLogPane()
{
    // Create panel
    wxPanel *pnlLog = new wxPanel(this);

    // Create flex grid
    wxFlexGridSizer *grdLog = new wxFlexGridSizer(1, 1, 5, 5);
    grdLog->AddGrowableCol(0);
    grdLog->AddGrowableRow(0);

    // Add the list control
    wxListCtrl *lstLog = new wxListCtrl(pnlLog, CTL_LOGLIST, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER);
    grdLog->Add(lstLog, 0, wxGROW, 3);
    
    // Add the panel to the notebook
    manager.AddPane(pnlLog,
      wxAuiPaneInfo().Center().
      Name(wxT("Logfile")).Caption(_("Logfile")).
      CaptionVisible(true).CloseButton(true).MaximizeButton(true).
      Dockable(true).Movable(true));

    // Auto-sizing
    pnlLog->SetSizer(grdLog);
    grdLog->Fit(pnlLog);

    // Add the log list
    logList = (ctlListView*)lstLog;

    // We don't need this report (but we need the pane)
    // if server release is less than 8.0 or if server has no adminpack
    if (!(connection->BackendMinimumVersion(8, 0) && 
         connection->HasFeature(FEATURE_FILEREAD)))
    {
        if (connection->BackendMinimumVersion(8, 0))
            frmHint::ShowHint(this, HINT_INSTRUMENTATION);
        
        logList->AddColumn(_("Log entry"), 800);

        // We're done
        return;
    }

    // Add each column to the list control
    logFormat = connection->ExecuteScalar(wxT("SHOW log_line_prefix"));
    if (logFormat == wxT("unset"))
        logFormat = wxEmptyString;
    logFmtPos=logFormat.Find('%', true);

    if (logFmtPos < 0)
        logFormatKnown = true;  // log_line_prefix not specified.
    else if (!logFmtPos && logFormat.Mid(logFmtPos, 2) == wxT("%t") && logFormat.Length() > 2)  // Timestamp at end of log_line_prefix?
    {
        logFormatKnown = true;
        logHasTimestamp = true;
        logList->AddColumn(_("Timestamp"), 100);
    }
    else if (connection->GetIsGreenplum())
    {
        logFormatKnown = true;
        logHasTimestamp = true;
        logList->AddColumn(_("Timestamp"), 100);
    }

    if (logFormatKnown)
        logList->AddColumn(_("Level"), 35);

    logList->AddColumn(_("Log entry"), 800);

    if (!connection->HasFeature(FEATURE_ROTATELOG))
        btnRotateLog->Disable();

    // Re-initialize variables
    logfileLength = 0;
        
    // Read logRate configuration
    settings->Read(wxT("frmStatus/RefreshLogRate"), &logRate, 10);
        
    // Create the timer
    logTimer = new wxTimer(this, TIMER_LOG_ID);
}


void frmStatus::OnCopy(wxCommandEvent& ev)
{
    ctlListView *list;
    int row, col;
    wxString text;
    
    switch(currentPane)
    {
        case PANE_STATUS:
            list = statusList;
            break;
        case PANE_LOCKS:
            list = lockList;
            break;
        case PANE_XACT:
            list = xactList;
            break;
        case PANE_LOG:
            list = logList;
            break;
        default:
            // This shouldn't happen.
            // If it does, it's no big deal, we just need to get out.
            return;
            break;
    }
    
    row = list->GetFirstSelected();
    
    while (row >= 0)
    {
        for (col = 0; col < list->GetColumnCount(); col++)
        {
            text.Append(list->GetText(row, col) + wxT("\t"));
        }
#ifdef __WXMSW__
        text.Append(wxT("\r\n"));
#else
        text.Append(wxT("\n"));
#endif
        row = list->GetNextSelected(row);
    }
    
    if (text.Length() > 0 && wxTheClipboard->Open())
    {
        wxTheClipboard->SetData(new wxTextDataObject(text));
        wxTheClipboard->Close();
    }
}


void frmStatus::OnPaneClose(wxAuiManagerEvent& evt)
{
    if (evt.pane->name == wxT("Activity"))
    {
        viewMenu->Check(MNU_STATUSPAGE, false);
        statusTimer->Stop();
    }
    if (evt.pane->name == wxT("Locks"))
    {
        viewMenu->Check(MNU_LOCKPAGE, false);
        locksTimer->Stop();
    }
    if (evt.pane->name == wxT("Transactions"))
    {
        viewMenu->Check(MNU_XACTPAGE, false);
        xactTimer->Stop();
    }
    if (evt.pane->name == wxT("Logfile"))
    {
        viewMenu->Check(MNU_LOGPAGE, false);
        logTimer->Stop();
    }
}


void frmStatus::OnToggleStatusPane(wxCommandEvent& event)
{
    if (viewMenu->IsChecked(MNU_STATUSPAGE))
    {
        manager.GetPane(wxT("Activity")).Show(true);
        cbRate->SetValue(rateToCboString(statusRate));
        if (statusRate > 0)
            statusTimer->Start(statusRate*1000L);
    }
    else
    {
        manager.GetPane(wxT("Activity")).Show(false);
        statusTimer->Stop();
    }

    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleLockPane(wxCommandEvent& event)
{
    if (viewMenu->IsChecked(MNU_LOCKPAGE))
    {
        manager.GetPane(wxT("Locks")).Show(true);
        cbRate->SetValue(rateToCboString(locksRate));
        if (locksRate > 0)
            locksTimer->Start(locksRate*1000L);
    }
    else
    {
        manager.GetPane(wxT("Locks")).Show(false);
        locksTimer->Stop();
    }
    
    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleXactPane(wxCommandEvent& event)
{
    if (viewMenu->IsEnabled(MNU_XACTPAGE) && viewMenu->IsChecked(MNU_XACTPAGE))
    {
        manager.GetPane(wxT("Transactions")).Show(true);
        cbRate->SetValue(rateToCboString(xactRate));
        if (xactRate > 0)
            xactTimer->Start(xactRate*1000L);
    }
    else
    {
        manager.GetPane(wxT("Transactions")).Show(false);
        xactTimer->Stop();
    }
    
    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleLogPane(wxCommandEvent& event)
{
    if (viewMenu->IsEnabled(MNU_LOGPAGE) && viewMenu->IsChecked(MNU_LOGPAGE))
    {
        manager.GetPane(wxT("Logfile")).Show(true);
        cbRate->SetValue(rateToCboString(logRate));
        if (logRate > 0)
            logTimer->Start(logRate*1000L);
    }
    else
    {
        manager.GetPane(wxT("Logfile")).Show(false);
        logTimer->Stop();
    }
    
    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnToggleToolBar(wxCommandEvent& event)
{
    if (viewMenu->IsChecked(MNU_TOOLBAR))
    {
        manager.GetPane(wxT("toolBar")).Show(true);
    }
    else
    {
        manager.GetPane(wxT("toolBar")).Show(false);
    }
    
    // Tell the manager to "commit" all the changes just made
    manager.Update();
}


void frmStatus::OnDefaultView(wxCommandEvent& event)
{
    manager.LoadPerspective(FRMSTATUS_DEFAULT_PERSPECTIVE, true);

    // Reset the captions for the current language
    manager.GetPane(wxT("toolBar")).Caption(_("Tool bar"));
    manager.GetPane(wxT("Activity")).Caption(_("Activity"));
    manager.GetPane(wxT("Locks")).Caption(_("Locks"));
    manager.GetPane(wxT("Transactions")).Caption(_("Transactions"));
    manager.GetPane(wxT("Logfile")).Caption(_("Logfile"));

    // tell the manager to "commit" all the changes just made
    manager.Update();

    // Sync the View menu options
    viewMenu->Check(MNU_TOOLBAR, manager.GetPane(wxT("toolBar")).IsShown());
    viewMenu->Check(MNU_STATUSPAGE, manager.GetPane(wxT("Activity")).IsShown());
    viewMenu->Check(MNU_LOCKPAGE, manager.GetPane(wxT("Locks")).IsShown());
    viewMenu->Check(MNU_XACTPAGE, manager.GetPane(wxT("Transactions")).IsShown());
    viewMenu->Check(MNU_LOGPAGE, manager.GetPane(wxT("Logfile")).IsShown());
}


void frmStatus::OnHelp(wxCommandEvent& event)
{
    wxString page;
    if (page.IsEmpty())
        page=wxT("sql-commands");

    if (connection->GetIsEdb())
        DisplayHelp(page, HELP_ENTERPRISEDB);
    else
        DisplayHelp(page, HELP_POSTGRESQL);
}


void frmStatus::OnRateChange(wxCommandEvent &event)
{
    wxTimer *timer;
    int rate;
    
    switch(currentPane)
    {
        case PANE_STATUS:
            timer = statusTimer;
            rate = cboToRate();
            statusRate = rate;
            break;
        case PANE_LOCKS:
            timer = locksTimer;
            rate = cboToRate();
            locksRate = rate;
            break;
        case PANE_XACT:
            timer = xactTimer;
            rate = cboToRate();
            xactRate = rate;
            break;
        case PANE_LOG:
            timer = logTimer;
            rate = cboToRate();
            logRate = rate;
            break;
        default:
            // This shouldn't happen.
            // If it does, it's no big deal, we just need to get out.
            return;
            break;
    }
    
    timer->Stop();
    if (rate > 0)
        timer->Start(rate*1000L);
    OnRefresh(event);
}


void frmStatus::OnRefreshUITimer(wxTimerEvent &event)
{
    wxListEvent evt;
    
    refreshUITimer->Stop();
    
    for (unsigned int i = 0; i < manager.GetAllPanes().GetCount(); i++)
    {
        wxAuiPaneInfo& pane = manager.GetAllPanes()[i];

        if (pane.HasFlag(wxAuiPaneInfo::optionActive))
        {
            if (pane.name == wxT("Activity") && currentPane != PANE_STATUS)
            {
                OnSelStatusItem(evt);
            }
            if (pane.name == wxT("Locks") && currentPane != PANE_LOCKS)
            {
                OnSelLockItem(evt);
            }
            if (pane.name == wxT("Transactions") && currentPane != PANE_XACT)
            {
                OnSelXactItem(evt);
            }
            if (pane.name == wxT("Logfile") && currentPane != PANE_LOG)
            {
                OnSelLogItem(evt);
            }
        }
    }
    
    refreshUITimer->Start(250);
}


void frmStatus::OnRefreshStatusTimer(wxTimerEvent &event)
{
    long pid=0;
    
    if (! viewMenu->IsChecked(MNU_STATUSPAGE))
        return;
    
    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        logTimer->Stop();
        statusBar->SetStatusText(wxT("Connection broken."));
        return;
    }

    checkConnection();
    if (!connection)
        return;
    
    wxCriticalSectionLocker lock(gs_critsect);

    connection->ExecuteVoid(wxT("SET log_statement='none';"));

    long row=0;
    pgSet *dataSet1=connection->ExecuteSet(wxT("SELECT *,(SELECT min(pid) FROM pg_locks l1 WHERE GRANTED AND relation IN (SELECT relation FROM pg_locks l2 WHERE l2.pid=procpid AND NOT granted)) AS blockedby FROM pg_stat_activity ORDER BY procpid"));
    if (dataSet1)
    {
        statusList->Freeze();
        statusBar->SetStatusText(_("Refreshing status list."));
        while (!dataSet1->Eof())
        {
            pid=dataSet1->GetLong(wxT("procpid"));

            if (pid != backend_pid)
            {
                long itempid=0;

                while (row < statusList->GetItemCount())
                {
                    itempid=StrToLong(statusList->GetItemText(row));
                    if (itempid && itempid < pid)
                        statusList->DeleteItem(row);
                    else
                        break;
                }

                if (!itempid || itempid > pid)
                {
                    statusList->InsertItem(row, NumToStr(pid), 0);
                }
                wxString qry=dataSet1->GetVal(wxT("current_query"));

                int colpos=1;
                statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("datname")));
                statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("usename")));

                if (connection->BackendMinimumVersion(8, 1))
                {
                     wxString client=dataSet1->GetVal(wxT("client_addr")) + wxT(":") + dataSet1->GetVal(wxT("client_port"));
                     if (client == wxT(":-1"))
                         client = _("local pipe");
                     statusList->SetItem(row, colpos++, client);

                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("backend_start")));
                }
                if (connection->BackendMinimumVersion(7, 4))
                {
                    if (qry.IsEmpty() || qry == wxT("<IDLE>"))
                        statusList->SetItem(row, colpos++, wxEmptyString);
                    else
                        statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("query_start")));
                }

                if (connection->BackendMinimumVersion(8, 3))
                    statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("xact_start")));

                statusList->SetItem(row, colpos++, dataSet1->GetVal(wxT("blockedby")));
                   statusList->SetItem(row, colpos, qry.Left(250));
                row++;
            }
            dataSet1->MoveNext();
        }
        delete dataSet1;
            
        while (row < statusList->GetItemCount())
            statusList->DeleteItem(row);

        statusList->Thaw();
        statusBar->SetStatusText(_("Done."));
    }
    else
        checkConnection();

    row=0;
    while (row < statusList->GetItemCount())
    {
        long itempid=StrToLong(statusList->GetItemText(row));
        if (itempid && itempid > pid)
            statusList->DeleteItem(row);
        else
            row++;
    }
}


void frmStatus::OnRefreshLocksTimer(wxTimerEvent &event)
{
    long pid=0;

    if (! viewMenu->IsChecked(MNU_LOCKPAGE))
        return;

    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        logTimer->Stop();
        statusBar->SetStatusText(wxT("Connection broken."));
        return;
    }

    checkConnection();
    if (!connection)
        return;
    
    wxCriticalSectionLocker lock(gs_critsect);

    connection->ExecuteVoid(wxT("SET log_statement='none';"));

    long row=0;
    wxString sql;
    if (connection->BackendMinimumVersion(8, 3)) 
    {
        sql = wxT("SELECT ")
              wxT("(SELECT datname FROM pg_database WHERE oid = pgl.database) AS dbname, ")
              wxT("pgl.relation::regclass AS class, ")
              wxT("pg_get_userbyid(pg_stat_get_backend_userid(svrid)) as user, ")
              wxT("pgl.virtualxid, pgl.virtualtransaction AS transaction, pg_stat_get_backend_pid(svrid) AS pid, pgl.mode, pgl.granted, ")
              wxT("pg_stat_get_backend_activity(svrid) AS current_query, ")
              wxT("pg_stat_get_backend_activity_start(svrid) AS query_start ")
              wxT("FROM pg_stat_get_backend_idset() svrid, pg_locks pgl ")
              wxT("WHERE pgl.pid = pg_stat_get_backend_pid(svrid) ")
              wxT("ORDER BY pid;");
    } 
    else if (connection->BackendMinimumVersion(7, 4)) 
    {
           sql = wxT("SELECT ")
              wxT("(SELECT datname FROM pg_database WHERE oid = pgl.database) AS dbname, ")
              wxT("pgl.relation::regclass AS class, ")
              wxT("pg_get_userbyid(pg_stat_get_backend_userid(svrid)) as user, ")
              wxT("pgl.transaction, pg_stat_get_backend_pid(svrid) AS pid, pgl.mode, pgl.granted, ")
              wxT("pg_stat_get_backend_activity(svrid) AS current_query, ")
              wxT("pg_stat_get_backend_activity_start(svrid) AS query_start ")
              wxT("FROM pg_stat_get_backend_idset() svrid, pg_locks pgl ")
              wxT("WHERE pgl.pid = pg_stat_get_backend_pid(svrid) ")
              wxT("ORDER BY pid;");
    } 
    else 
    {
        sql = wxT("SELECT ")
              wxT("(SELECT datname FROM pg_database WHERE oid = pgl.database) AS dbname, ")
              wxT("pgl.relation::regclass AS class, ")
              wxT("pg_get_userbyid(pg_stat_get_backend_userid(svrid)) as user, ")
              wxT("pgl.transaction, pg_stat_get_backend_pid(svrid) AS pid, pgl.mode, pgl.granted, ")
              wxT("pg_stat_get_backend_activity(svrid) AS current_query ")
              wxT("FROM pg_stat_get_backend_idset() svrid, pg_locks pgl ")
              wxT("WHERE pgl.pid = pg_stat_get_backend_pid(svrid) ")
              wxT("ORDER BY pid;");
    }

    pgSet *dataSet2=connection->ExecuteSet(sql);
    if (dataSet2)
    {
        statusBar->SetStatusText(_("Refreshing locks list."));
        lockList->Freeze();

        while (!dataSet2->Eof())
        {
            pid=dataSet2->GetLong(wxT("pid"));

            if (pid != backend_pid)
            {
                long itempid=0;

                while (row < lockList->GetItemCount())
                {
                    itempid=StrToLong(lockList->GetItemText(row));
                    if (itempid && itempid < pid)
                        lockList->DeleteItem(row);
                    else
                        break;
                }

                if (!itempid || itempid > pid)
                {
                    lockList->InsertItem(row, NumToStr(pid), 0);
                }

                int colpos=1;
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("dbname")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("class")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("user")));
                if (connection->BackendMinimumVersion(8, 3)) 
                    lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("virtualxid")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("transaction")));
                lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("mode")));
                    
                if (dataSet2->GetVal(wxT("granted")) == wxT("t"))
                    lockList->SetItem(row, colpos++, _("Yes"));
                else
                    lockList->SetItem(row, colpos++, _("No"));

                wxString qry=dataSet2->GetVal(wxT("current_query"));

                if (connection->BackendMinimumVersion(7, 4))
                {
                    if (qry.IsEmpty() || qry == wxT("<IDLE>"))
                        lockList->SetItem(row, colpos++, wxEmptyString);
                    else
                        lockList->SetItem(row, colpos++, dataSet2->GetVal(wxT("query_start")));
                }
                lockList->SetItem(row, colpos++, qry.Left(250));

                row++;
            }
            dataSet2->MoveNext();
        }

        delete dataSet2;
        
        while (row < lockList->GetItemCount())
            lockList->DeleteItem(row);

        lockList->Thaw();
        statusBar->SetStatusText(_("Done."));
    }
    else
        checkConnection();

    row=0;
    while (row < lockList->GetItemCount())
    {
        long itempid=StrToLong(lockList->GetItemText(row));
        if (itempid && itempid > pid)
            lockList->DeleteItem(row);
        else
            row++;
    }
}


void frmStatus::OnRefreshXactTimer(wxTimerEvent &event)
{
    long pid=0;

    if (! viewMenu->IsEnabled(MNU_XACTPAGE) || ! viewMenu->IsChecked(MNU_XACTPAGE))
        return;

    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        logTimer->Stop();
        statusBar->SetStatusText(wxT("Connection broken."));
        return;
    }

    checkConnection();
    if (!connection)
        return;
    
    wxCriticalSectionLocker lock(gs_critsect);

    connection->ExecuteVoid(wxT("SET log_statement='none';"));

    long row=0;
    wxString sql = wxT("SELECT * FROM pg_prepared_xacts");

    pgSet *dataSet3=connection->ExecuteSet(sql);
    if (dataSet3)
    {
        statusBar->SetStatusText(_("Refreshing transactions list."));
        xactList->Freeze();

        while (!dataSet3->Eof())
        {
            long xid=dataSet3->GetLong(wxT("transaction"));

            long itemxid=0;

            while (row < xactList->GetItemCount())
            {
                itemxid=StrToLong(xactList->GetItemText(row));
                if (itemxid && itemxid < xid)
                    xactList->DeleteItem(row);
                else
                    break;
            }

            if (!itemxid || itemxid > xid)
            {
                xactList->InsertItem(row, NumToStr(xid), 0);
            }

            int colpos=1;
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("gid")));
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("prepared")));
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("owner")));
            xactList->SetItem(row, colpos++, dataSet3->GetVal(wxT("database")));

            row++;
               dataSet3->MoveNext();
        }
        delete dataSet3;
            
        while (row < xactList->GetItemCount())
            xactList->DeleteItem(row);

        xactList->Thaw();
        statusBar->SetStatusText(_("Done."));
    }
    else
        checkConnection();

    row=0;
    while (row < xactList->GetItemCount())
    {
    long itempid=StrToLong(lockList->GetItemText(row));
    if (itempid && itempid > pid)
        xactList->DeleteItem(row);
    else
        row++;
    }
}


void frmStatus::OnRefreshLogTimer(wxTimerEvent &event)
{
    if (! viewMenu->IsEnabled(MNU_LOGPAGE) || ! viewMenu->IsChecked(MNU_LOGPAGE))
        return;

    if (!connection)
    {
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        logTimer->Stop();
        statusBar->SetStatusText(wxT("Connection broken."));
        return;
    }
    
    checkConnection();
    if (!connection)
        return;
    
    wxCriticalSectionLocker lock(gs_critsect);

    connection->ExecuteVoid(wxT("SET log_statement='none';"));

    long newlen=0;

    if (logDirectory.IsEmpty())
    {
        // freshly started
        logDirectory = connection->ExecuteScalar(wxT("SHOW log_directory"));
        if (fillLogfileCombo())
        {
            cbLogfiles->SetSelection(0);
            wxCommandEvent ev;
            OnLoadLogfile(ev);
            return;
        }
        else 
        {
            logDirectory = wxT("-");
            if (connection->BackendMinimumVersion(8, 3))
                logList->AppendItem(-1, _("logging_collector not enabled or log_filename misconfigured"));
            else
                logList->AppendItem(-1, _("redirect_stderr not enabled or log_filename misconfigured"));
            cbLogfiles->Disable();
            btnRotateLog->Disable();
        }
    }
        
    if (logDirectory == wxT("-"))
        return;

    if (isCurrent)
    {
        // check if the current logfile changed
        pgSet *set = connection->ExecuteSet(wxT("SELECT pg_file_length(") + connection->qtDbString(logfileName) + wxT(") AS len"));
        if (set)
        {
            newlen = set->GetLong(wxT("len"));
            delete set;
        }
        else
        {
            checkConnection();
            return;
        }
        if (newlen > logfileLength)
        {
            statusBar->SetStatusText(_("Refreshing log list."));
            addLogFile(logfileName, logfileTimestamp, newlen, logfileLength, false);
            statusBar->SetStatusText(_("Done."));

            // as long as there was new data, the logfile is probably the current
            // one so we don't need to check for rotation
            return;
        }
    }        

    // 
    wxString newDirectory = connection->ExecuteScalar(wxT("SHOW log_directory"));

    int newfiles=0;
    if (newDirectory != logDirectory)
        cbLogfiles->Clear();

    newfiles = fillLogfileCombo();

    if (newfiles)
    {
        if (!showCurrent)
            isCurrent=false;
        
        if (isCurrent)
        {
            int pos = cbLogfiles->GetCount() - newfiles;
            bool skipFirst = true;

            while (newfiles--)
            {
                addLogLine(_("pgadmin:Logfile rotated."), false);
                wxDateTime *ts=(wxDateTime*)cbLogfiles->GetClientData(pos++);
                wxASSERT(ts != 0);
                    
                addLogFile(ts, skipFirst);
                skipFirst = false;

                pos++;
            }
        }
    }
}


void frmStatus::OnRefresh(wxCommandEvent &event)
{
    wxTimerEvent evt;
    
    OnRefreshStatusTimer(evt);
    OnRefreshLocksTimer(evt);
    OnRefreshXactTimer(evt);
    OnRefreshLogTimer(evt);
}


void frmStatus::checkConnection()
{
    if (!connection->IsAlive())
    {
        delete connection;
        connection=0;
        statusTimer->Stop();
        locksTimer->Stop();
        xactTimer->Stop();
        logTimer->Stop();
        statusBar->SetStatusText(_("Connection broken."));
    }
}


void frmStatus::addLogFile(wxDateTime *dt, bool skipFirst)
{
    pgSet *set=connection->ExecuteSet(
        wxT("SELECT filetime, filename, pg_file_length(filename) AS len ")
        wxT("  FROM pg_logdir_ls() AS A(filetime timestamp, filename text) ")
        wxT(" WHERE filetime = '") + DateToAnsiStr(*dt) + wxT("'::timestamp"));
    if (set)
    {
        logfileName = set->GetVal(wxT("filename"));
        logfileTimestamp = set->GetDateTime(wxT("filetime"));
        long len=set->GetLong(wxT("len"));

        logfileLength = 0;
        addLogFile(logfileName, logfileTimestamp, len, logfileLength, skipFirst);

        delete set;
    }
}


void frmStatus::addLogFile(const wxString &filename, const wxDateTime timestamp, long len, long &read, bool skipFirst)
{
    wxString line;
    
    if (skipFirst)
    {
        long maxServerLogSize = settings->GetMaxServerLogSize();

        if (!logfileLength && maxServerLogSize && logfileLength > maxServerLogSize)
        {
            long maxServerLogSize=settings->GetMaxServerLogSize();
            len = read-maxServerLogSize;
        }
        else
            skipFirst=false;
    }


    while (len > read)
    {
        pgSet *set=connection->ExecuteSet(wxT("SELECT pg_file_read(") + 
            connection->qtDbString(filename) + wxT(", ") + NumToStr(read) + wxT(", 50000)"));
        if (!set)
        {
            connection->IsAlive();
            return;
        }
        char *raw=set->GetCharPtr(0);

        if (!raw || !*raw)
        {
            delete set;
            break;
        }
        read += strlen(raw);

        wxString str;
        if (wxString(wxString(raw,wxConvLibc),wxConvUTF8).Len() > 0)
            str = line + wxString(wxString(raw,wxConvLibc),wxConvUTF8);
        else
            str = line + wxTextBuffer::Translate(wxString(raw, set->GetConversion()), wxTextFileType_Unix);

        delete set;

        if (str.Len() == 0)
        {
            wxString msgstr = _("The server log contains entries in multiple encodings and cannot be displayed by pgAdmin.");
            wxMessageBox(msgstr);
            return;
        }

        bool hasCr = (str.Right(1) == wxT("\n"));

        wxStringTokenizer tk(str, wxT("\n"));

        logList->Freeze();
        while (tk.HasMoreTokens())
        {
            str = tk.GetNextToken();
            if (skipFirst)
            {
                // could be truncated
                skipFirst = false;
                continue;
            }
            if (tk.HasMoreTokens() || hasCr)
                addLogLine(str.Trim());
            else
                line = str;
        }
        logList->Thaw();
    }
    if (!line.IsEmpty())
        addLogLine(line.Trim());
}


void frmStatus::addLogLine(const wxString &str, bool formatted)
{
    int row=logList->GetItemCount();
    if (!logFormatKnown)
        logList->AppendItem(-1, str);
    else
    {
        if (connection->GetIsGreenplum() && connection->BackendMinimumVersion(8, 2, 13))
        {
            // Greenplum 3.3 release and later:  log is in CSV format

            if (!formatted)
            {
                logList->InsertItem(row, wxEmptyString, -1);
                logList->SetItem(row, 1, str.BeforeFirst(':'));
                logList->SetItem(row, 2, str.AfterFirst(':').Mid(2));
            }
            else
            {
                wxString ts = str.BeforeFirst(',');
                if (ts.Length() < 20 || ts[0] != wxT('2') || ts[1] != wxT('0'))
                {
                    // Log line not a timestamp... Must be a continuation of the previous line.
                    logList->InsertItem(row, wxEmptyString, -1);
                    logList->SetItem(row, 2, str);
                }
                else
                {
                    logList->InsertItem(row, ts, -1);   // Insert timestamp
                    // Find loglevel... This needs work.  Better than nothing for beta 3.
                    wxString loglevel = str.AfterFirst('\"').BeforeFirst('\"');
                    if (str.Find(wxT(",\"LOG\",")) > 0)
                        loglevel = wxT("LOG");
                    if (str.Find(wxT(",\"NOTICE\",")) > 0)
                        loglevel = wxT("NOTICE");
                    if (str.Find(wxT(",\"WARNING\",")) > 0)
                        loglevel = wxT("WARNING");
                    if (str.Find(wxT(",\"ERROR\",")) > 0)
                        loglevel = wxT("ERROR");
                    if (str.Find(wxT(",\"FATAL\",")) > 0)
                        loglevel = wxT("FATAL");
                    if (str.Find(wxT(",\"PANIC\",")) > 0)
                        loglevel = wxT("PANIC");
                    if (str.Find(wxT(",\"DEBUG")) > 0)
                        loglevel = wxT("DEBUG");
                    if (loglevel[0] >= wxT('D') && loglevel[0] <= wxT('W'))
                    {
                        logList->SetItem(row, 1, loglevel);
                        logList->SetItem(row, 2, str.AfterFirst(','));
                    }
                    else
                        logList->SetItem(row, 2, str.AfterFirst(','));
                }
            }
        }
        else if (connection->GetIsGreenplum())
        {
            // Greenplum 3.2 and before.

            if (str.Find(':') < 0)
            {
                // Must be a continuation of a previous line.
                logList->InsertItem(row, wxEmptyString, -1);
                logList->SetItem(row, 2, str);
            }
            else
            {
                wxString rest;
                if (formatted)
                {
                    wxString ts = str.BeforeFirst(logFormat.c_str()[logFmtPos+2]);
                    if (ts.Length() < 20  || ts.Left(2) != wxT("20") || str.Find(':') < 0)
                    {
                        // No Timestamp?  Must be a continuation of a previous line.
                        logList->InsertItem(row, wxEmptyString, -1);
                        logList->SetItem(row, 2, str);
                    }
                    else
                    {
                        logList->InsertItem(row, ts, -1);
                        rest = str.Mid(logFmtPos + ts.Length()).AfterFirst(':');
                        logList->SetItem(row, 2, rest);   
                    }
                }
                else
                {
                    logList->InsertItem(row, wxEmptyString, -1);
                    logList->SetItem(row, 1, str.BeforeFirst(':'));
                    logList->SetItem(row, 2, str.AfterFirst(':').Mid(2));
                }
            }
            
        }
        else
        {
            // All Non-GPDB PostgreSQL systems.

            if (str.Find(':') < 0)
            {
                logList->InsertItem(row, wxEmptyString, -1);
                logList->SetItem(row, (logHasTimestamp ? 2 : 1), str);
            }
            else
            {
                wxString rest;

                if (logHasTimestamp)
                {
                    if (formatted)
                    {
                        rest = str.Mid(logFmtPos + 22).AfterFirst(':');
                        wxString ts=str.Mid(logFmtPos, str.Length()-rest.Length() - logFmtPos -1);

                        int pos = ts.Find(logFormat.c_str()[logFmtPos+2], true);
                        logList->InsertItem(row, ts.Left(pos), -1);
                        logList->SetItem(row, 1, ts.Mid(pos + logFormat.Length() - logFmtPos -2));
                        logList->SetItem(row, 2, rest.Mid(2));
                    }
                    else
                    {
                        logList->InsertItem(row, wxEmptyString, -1);
                        logList->SetItem(row, 1, str.BeforeFirst(':'));
                        logList->SetItem(row, 2, str.AfterFirst(':').Mid(2));
                    }
                }
                else
                {
                    if (formatted)
                        rest = str.Mid(logFormat.Length());
                    else
                        rest = str;

                    int pos = rest.Find(':');

                    if (pos < 0)
                        logList->InsertItem(row, rest, -1);
                    else
                    {
                        logList->InsertItem(row, rest.BeforeFirst(':'), -1);
                        logList->SetItem(row, 1, rest.AfterFirst(':').Mid(2));
                    }
                }
            }
        }
    }
}


void frmStatus::emptyLogfileCombo()
{
    if (cbLogfiles->GetCount()) // first entry has no client data
        cbLogfiles->Delete(0);

    while (cbLogfiles->GetCount())
    {
        wxDateTime *dt=(wxDateTime*)cbLogfiles->GetClientData(0);
        if (dt)
            delete dt;
        cbLogfiles->Delete(0);
    }
}


int frmStatus::fillLogfileCombo()
{
    int count=cbLogfiles->GetCount();
    if (!count)
        cbLogfiles->Append(_("Current log"));
    else
        count--;

    pgSet *set=connection->ExecuteSet(
        wxT("SELECT filename, filetime\n")
        wxT("  FROM pg_logdir_ls() AS A(filetime timestamp, filename text)\n")
        wxT(" ORDER BY filetime ASC"));
    if (set)
    {
        if (set->NumRows() <= count)
            return 0;

        set->Locate(count+1);
        count=0;

        while (!set->Eof())
        {
            count++;
            wxString fn= set->GetVal(wxT("filename"));
            wxDateTime ts=set->GetDateTime(wxT("filetime"));

            cbLogfiles->Append(DateToAnsiStr(ts), (void*)new wxDateTime(ts));

            set->MoveNext();
        }

        delete set;
    }

    return count;
}


void frmStatus::OnLoadLogfile(wxCommandEvent &event)
{
    int pos=cbLogfiles->GetCurrentSelection();
    int lastPos = cbLogfiles->GetCount()-1;
    if (pos >= 0)
    {
        showCurrent = (pos == 0);
        isCurrent = showCurrent || (pos == lastPos);

        wxDateTime *ts=(wxDateTime*)cbLogfiles->GetClientData(showCurrent ? lastPos : pos);
        wxASSERT(ts != 0);

        if (!logfileTimestamp.IsValid() || *ts != logfileTimestamp)
        {
            logList->DeleteAllItems();
            addLogFile(ts, true);
        }
    }
}


void frmStatus::OnRotateLogfile(wxCommandEvent &event)
{
    if (wxMessageBox(_("Are you sure the logfile should be rotated?"), _("Logfile rotation"), wxYES_NO|wxNO_DEFAULT | wxICON_QUESTION) == wxYES)
        connection->ExecuteVoid(wxT("select pg_logfile_rotate()"));
}


void frmStatus::OnCancelBtn(wxCommandEvent &event)
{
    switch(currentPane)
    {
        case PANE_STATUS:
            OnStatusCancelBtn(event);
            break;
        case PANE_LOCKS:
            OnLocksCancelBtn(event);
            break;
        default:
            // This shouldn't happen. If it does, it's no big deal
            break;
    }
}


void frmStatus::OnStatusCancelBtn(wxCommandEvent &event)
{
    long item = statusList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to cancel the selected query(s)?"), _("Cancel query?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxNO)
        return;

    while  (item >= 0)
    {
        wxString pid = statusList->GetItemText(item);
        wxString sql = wxT("SELECT pg_cancel_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = statusList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A cancel signal was sent to the selected server process(es)."), _("Cancel query"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelStatusItem(ev);
}


void frmStatus::OnLocksCancelBtn(wxCommandEvent &event)
{
    long item = lockList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to cancel the selected query(s)?"), _("Cancel query?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxNO)
        return;

    while  (item >= 0)
    {
        wxString pid = lockList->GetItemText(item);
        wxString sql = wxT("SELECT pg_cancel_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = lockList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A cancel signal was sent to the selected server process(es)."), _("Cancel query"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelLockItem(ev);
}


void frmStatus::OnTerminateBtn(wxCommandEvent &event)
{
    switch(currentPane)
    {
        case PANE_STATUS:
            OnStatusTerminateBtn(event);
            break;
        case PANE_LOCKS:
            OnLocksTerminateBtn(event);
            break;
        default:
            // This shouldn't happen. If it does, it's no big deal
            break;
    }
}


void frmStatus::OnStatusTerminateBtn(wxCommandEvent &event)
{
    long item = statusList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to terminate the selected server process(es)?"), _("Terminate process?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxNO)
        return;

    while  (item >= 0)
    {
        wxString pid = statusList->GetItemText(item);
        wxString sql = wxT("SELECT pg_terminate_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = statusList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A terminate signal was sent to the selected server process(es)."), _("Terminate process"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelStatusItem(ev);
}


void frmStatus::OnLocksTerminateBtn(wxCommandEvent &event)
{
    long item = lockList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to terminate the selected server process(es)?"), _("Terminate process?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxNO)
        return;

    while  (item >= 0)
    {
        wxString pid = lockList->GetItemText(item);
        wxString sql = wxT("SELECT pg_terminate_backend(") + pid + wxT(");");
        connection->ExecuteScalar(sql);

        item = lockList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    wxMessageBox(_("A terminate signal was sent to the selected server process(es)."), _("Terminate process"), wxOK | wxICON_INFORMATION);
    OnRefresh(event);
    wxListEvent ev;
    OnSelLockItem(ev);
}


void frmStatus::OnCommit(wxCommandEvent &event)
{
    long item = xactList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to commit the selected prepared transactions?"), _("Commit transaction?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxNO)
        return;

    while  (item >= 0)
    {
        wxString xid = xactList->GetText(item, 1);
        wxString sql = wxT("COMMIT PREPARED ") + connection->qtDbString(xid);

        // We must execute this in the database in which the prepared transation originated.
        if (connection->GetDbname() != xactList->GetText(item, 4))
        {
            pgConn *tmpConn=new pgConn(connection->GetHost(), 
                                       xactList->GetText(item, 4), 
                                       connection->GetUser(), 
                                       connection->GetPassword(), 
                                       connection->GetPort(), 
                                       connection->GetSslMode());
            if (tmpConn)
            {
                if (tmpConn->GetStatus() != PGCONN_OK)
                {
                    wxMessageBox(wxT("Connection failed: ") + tmpConn->GetLastError());
                    return ;
                }
                tmpConn->ExecuteScalar(sql);

                tmpConn->Close();
                delete tmpConn;
            }
        }
        else
            connection->ExecuteScalar(sql);

        item = xactList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    OnRefresh(event);
    wxListEvent ev;
    OnSelXactItem(ev);
}


void frmStatus::OnRollback(wxCommandEvent &event)
{
    long item = xactList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (item < 0)
        return;

    if (wxMessageBox(_("Are you sure you wish to rollback the selected prepared transactions?"), _("Rollback transaction?"), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION) == wxNO)
        return;

    while  (item >= 0)
    {
        wxString xid = xactList->GetText(item, 1);
        wxString sql = wxT("ROLLBACK PREPARED ") + connection->qtDbString(xid);

        // We must execute this in the database in which the prepared transation originated.
        if (connection->GetDbname() != xactList->GetText(item, 4))
        {
            pgConn *tmpConn=new pgConn(connection->GetHost(), 
                                       xactList->GetText(item, 4), 
                                       connection->GetUser(), 
                                       connection->GetPassword(), 
                                       connection->GetPort(), 
                                       connection->GetSslMode());
            if (tmpConn)
            {
                if (tmpConn->GetStatus() != PGCONN_OK)
                {
                    wxMessageBox(wxT("Connection failed: ") + tmpConn->GetLastError());
                    return ;
                }
                tmpConn->ExecuteScalar(sql);

                tmpConn->Close();
                delete tmpConn;
            }
        }
        else
            connection->ExecuteScalar(sql);

        item = xactList->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    }

    OnRefresh(event);
    wxListEvent ev;
    OnSelXactItem(ev);
}


void frmStatus::OnSelStatusItem(wxListEvent &event)
{
#ifdef __WXGTK__
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_STATUS;
    cbRate->SetValue(rateToCboString(statusRate));
    if (connection && connection->BackendMinimumVersion(8, 0))
    {
        if(statusList->GetSelectedItemCount() > 0) 
        {
            toolBar->EnableTool(MNU_CANCEL, true);
            if (connection->HasFeature(FEATURE_TERMINATE_BACKEND))
                toolBar->EnableTool(MNU_TERMINATE, true);
        } 
        else 
        {
            toolBar->EnableTool(MNU_CANCEL, false);
            toolBar->EnableTool(MNU_TERMINATE, false);
        }
    }
    toolBar->EnableTool(MNU_COMMIT, false);
    toolBar->EnableTool(MNU_ROLLBACK, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);
    
    editMenu->Enable(MNU_COPY, statusList->GetFirstSelected()>=0);
}


void frmStatus::OnSelLockItem(wxListEvent &event)
{
#ifdef __WXGTK__
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_LOCKS;
    cbRate->SetValue(rateToCboString(locksRate));
    if (connection && connection->BackendMinimumVersion(8, 0))
    {
        if(lockList->GetSelectedItemCount() > 0) 
        {
            toolBar->EnableTool(MNU_CANCEL, true);
            if (connection->HasFeature(FEATURE_TERMINATE_BACKEND))
                toolBar->EnableTool(MNU_TERMINATE, true);
        } 
        else 
        {
            toolBar->EnableTool(MNU_CANCEL, false);
            toolBar->EnableTool(MNU_TERMINATE, false);
        }
    }
    toolBar->EnableTool(MNU_COMMIT, false);
    toolBar->EnableTool(MNU_ROLLBACK, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);
    
    editMenu->Enable(MNU_COPY, lockList->GetFirstSelected()>=0);
}


void frmStatus::OnSelXactItem(wxListEvent &event)
{
#ifdef __WXGTK__
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.Update();
#endif
    currentPane = PANE_XACT;
    cbRate->SetValue(rateToCboString(xactRate));
    if(xactList->GetSelectedItemCount() > 0) 
    {
        toolBar->EnableTool(MNU_COMMIT, true);
        toolBar->EnableTool(MNU_ROLLBACK, true);
    } 
    else 
    {
        toolBar->EnableTool(MNU_COMMIT, false);
        toolBar->EnableTool(MNU_ROLLBACK, false);
    }
    toolBar->EnableTool(MNU_CANCEL, false);
    toolBar->EnableTool(MNU_TERMINATE, false);
    cbLogfiles->Enable(false);
    btnRotateLog->Enable(false);
    
    editMenu->Enable(MNU_COPY, xactList->GetFirstSelected()>=0);
}


void frmStatus::OnSelLogItem(wxListEvent &event)
{
#ifdef __WXGTK__
    manager.GetPane(wxT("Activity")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Locks")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Transactions")).SetFlag(wxAuiPaneInfo::optionActive, false);
    manager.GetPane(wxT("Logfile")).SetFlag(wxAuiPaneInfo::optionActive, true);
    manager.Update();
#endif
    currentPane = PANE_LOG;
    cbRate->SetValue(rateToCboString(logRate));
    
    // if there's no log, don't enable items
    if (logDirectory != wxT("-")) 
    {
        cbLogfiles->Enable(true);
        btnRotateLog->Enable(true);
        toolBar->EnableTool(MNU_CANCEL, false);
        toolBar->EnableTool(MNU_TERMINATE, false);
        toolBar->EnableTool(MNU_COMMIT, false);
        toolBar->EnableTool(MNU_ROLLBACK, false);
    }
    
    editMenu->Enable(MNU_COPY, logList->GetFirstSelected()>=0);
}


serverStatusFactory::serverStatusFactory(menuFactoryList *list, wxMenu *mnu, ctlMenuToolbar *toolbar) : actionFactory(list)
{
    mnu->Append(id, _("&Server Status"), _("Displays the current database status."));
}


wxWindow *serverStatusFactory::StartDialog(frmMain *form, pgObject *obj)
{

    pgServer *server=obj->GetServer();

    pgConn *conn = server->CreateConn();
    if (conn)
    {
        wxString txt = _("Server Status - ") + server->GetDescription() 
            + wxT(" (") + server->GetName() + wxT(":") + NumToStr((long)server->GetPort()) + wxT(")");

        frmStatus *status = new frmStatus(form, txt, conn);
        status->Go();
        return status;
    }
    return 0;
}


bool serverStatusFactory::CheckEnable(pgObject *obj)
{
    return obj && obj->GetServer() != 0;
}
