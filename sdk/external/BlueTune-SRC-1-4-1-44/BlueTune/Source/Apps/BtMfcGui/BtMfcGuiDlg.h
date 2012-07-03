// BtMfcGuiDlg.h : header file
//

#pragma once
#include "afxcmn.h"

// forward declarations
class MfcPlayer;

// CBtMfcGuiDlg dialog
class CBtMfcGuiDlg : public CDialog
{
// Construction
public:
	CBtMfcGuiDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_BTMFCGUI_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
public:
    // methods
    void ClearInfo();
    void UpdateInfo(const char* name, const char* value);

protected:
    MfcPlayer* m_Player;

    HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedOpenButton();
    afx_msg void OnBnClickedPlayButton();
    afx_msg void OnBnClickedPauseButton();
    afx_msg void OnBnClickedStopButton();
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    afx_msg void OnBnClickedSetInputButton();

protected:
    CSliderCtrl m_TrackSlider;
    CSliderCtrl m_VolumeSlider;
    CListCtrl   m_StreamInfoList;

friend class MfcPlayer;
};
