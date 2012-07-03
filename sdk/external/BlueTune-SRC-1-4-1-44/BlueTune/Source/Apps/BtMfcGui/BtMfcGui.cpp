// BtMfcGui.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "BtMfcGui.h"
#include "BtMfcGuiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBtMfcGuiApp

BEGIN_MESSAGE_MAP(CBtMfcGuiApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// The one and only CBtMfcGuiApp object

CBtMfcGuiApp theApp;


// CBtMfcGuiApp initialization

BOOL CBtMfcGuiApp::InitInstance()
{
	InitCommonControls();

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Standard initialization
	SetRegistryKey(_T("BlueTune MFC Sample Player"));

	CBtMfcGuiDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
