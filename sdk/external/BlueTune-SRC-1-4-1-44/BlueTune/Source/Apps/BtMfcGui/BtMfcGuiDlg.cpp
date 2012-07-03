// BtMfcGuiDlg.cpp : implementation file
//

#include "stdafx.h"

#include <stdio.h>

#include "BtMfcGui.h"
#include "BtMfcGuiDlg.h"
#include "BlueTune.h"
#include "NptWin32MessageQueue.h"
#include "BtMfcGuiDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

/*----------------------------------------------------------------------
|   MfcPlayer
+---------------------------------------------------------------------*/
class MfcPlayer : public BLT_Player
{
public:
    MfcPlayer(CBtMfcGuiDlg* dialog);
    ~MfcPlayer();

    // message handlers
    virtual void OnDecoderStateNotification(BLT_DecoderServer::State state);
    virtual void OnVolumeNotification(float volume);
    virtual void OnStreamTimeCodeNotification(BLT_TimeCode timecode);
    virtual void OnStreamPositionNotification(BLT_StreamPosition& position);
    virtual void OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info);
    virtual void OnPropertyNotification(BLT_PropertyScope        scope,
                                        const char*              source,
                                        const char*              name,
                                        const ATX_PropertyValue& value);

    // members
    CBtMfcGuiDlg* m_Dialog;
    bool          m_Scrolling; // true is we are dragging the seek thumb
};

/*----------------------------------------------------------------------
|   MfcPlayer::MfcPlayer
+---------------------------------------------------------------------*/
MfcPlayer::MfcPlayer(CBtMfcGuiDlg* dialog) : 
        // We use an instance of NPT_Win32WindowMessageQueue
        // so that player messages will be dispatched just 
        // like other windows messages, when the main loop
        // calls DispatchMessage()
        BLT_Player(new NPT_Win32WindowMessageQueue()),
        m_Dialog(dialog), 
        m_Scrolling(false)
{
}

/*----------------------------------------------------------------------
|   MfcPlayer::~MfcPlayer
+---------------------------------------------------------------------*/
MfcPlayer::~MfcPlayer()
{
    // ensure that we won't be receiving any more messages on the queue
    Shutdown();

    // cleanup;
    delete m_Queue;
}

/*----------------------------------------------------------------------
|   MfcPlayer::OnDecoderStateNotification
+---------------------------------------------------------------------*/
void 
MfcPlayer::OnDecoderStateNotification(BLT_DecoderServer::State state)
{
    const char* state_string;
    switch (state) {
        case BLT_DecoderServer::STATE_STOPPED: state_string = "[STOPPED]"; break;
        case BLT_DecoderServer::STATE_PLAYING: state_string = "[PLAYING]"; break;
        case BLT_DecoderServer::STATE_PAUSED: state_string = "[PAUSED]"; break;
        case BLT_DecoderServer::STATE_EOS: state_string = "[END-OF-STREAM]"; break;
        default: state_string = "";
    }
    m_Dialog->SetDlgItemText(IDC_PLAYER_STATUS_LABEL, state_string);
}

/*----------------------------------------------------------------------
|   MfcPlayer::OnStreamTimeCodeNotification
+---------------------------------------------------------------------*/
void
MfcPlayer::OnStreamTimeCodeNotification(BLT_TimeCode timecode)
{
    char timecode_string[16];
    NPT_FormatString(timecode_string, sizeof(timecode_string),
                     "%02d:%02d:%02d", 
                     timecode.h, timecode.m, timecode.s);
    m_Dialog->SetDlgItemText(IDC_PLAYER_TIMECODE_LABEL, timecode_string);
}

/*----------------------------------------------------------------------
|   MfcPlayer::OnStreamPositionNotification
+---------------------------------------------------------------------*/
void 
MfcPlayer::OnStreamPositionNotification(BLT_StreamPosition& position)
{
    // do nothing if we're dragging the scroll bar
    if (m_Scrolling) return;

    // show the position on the scroll bar
    ATX_Int64 range = m_Dialog->m_TrackSlider.GetRangeMax()-m_Dialog->m_TrackSlider.GetRangeMin();
    ATX_Int64 pos = (position.offset*range)/position.range;
    m_Dialog->m_TrackSlider.SetPos((unsigned int)pos);
}

/*----------------------------------------------------------------------
|   MfcPlayer::OnStreamInfoNotification
+---------------------------------------------------------------------*/
void
MfcPlayer::OnStreamInfoNotification(BLT_Mask update_mask, BLT_StreamInfo& info)
{
    char value[16];
    if (update_mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
        NPT_FormatString(value, sizeof(value), "%d", info.nominal_bitrate);
        m_Dialog->UpdateInfo("Nominal Bitrate", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
        NPT_FormatString(value, sizeof(value), "%d", info.instant_bitrate);
        m_Dialog->UpdateInfo("Instant Bitrate", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
        NPT_FormatString(value, sizeof(value), "%d", info.average_bitrate);
        m_Dialog->UpdateInfo("Average Bitrate", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
        NPT_FormatString(value, sizeof(value), "%d", info.channel_count);
        m_Dialog->UpdateInfo("Channels", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
        NPT_FormatString(value, sizeof(value), "%d", info.sample_rate);
        m_Dialog->UpdateInfo("Sample Rate", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_DURATION) {
        NPT_FormatString(value, sizeof(value), "%d", info.duration);
        m_Dialog->UpdateInfo("Duration", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_SIZE) {
        NPT_FormatString(value, sizeof(value), "%d", info.size);
        m_Dialog->UpdateInfo("Size", value);
    }
    if (update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
        m_Dialog->UpdateInfo("Data Type", info.data_type);
    }
}

/*----------------------------------------------------------------------
|   MfcPlayer::OnStreamPropertyNotification
+---------------------------------------------------------------------*/
void
MfcPlayer:: OnPropertyNotification(BLT_PropertyScope        scope,
                                   const char*              source,
                                   const char*              name,
                                   const ATX_PropertyValue& value)
{
    char text[16];
    switch (value.type) {
        case ATX_PROPERTY_VALUE_TYPE_INTEGER:
            NPT_FormatString(text, sizeof(text), "%d", value.data.integer);
            m_Dialog->UpdateInfo(name, text);
            break;

        case ATX_PROPERTY_VALUE_TYPE_STRING:
            m_Dialog->UpdateInfo(name, value.data.string);
            break;
    }
}

/*----------------------------------------------------------------------
|   MfcPlayer::OnVolumeNotification
+---------------------------------------------------------------------*/
void
MfcPlayer::OnVolumeNotification(float volume)
{
    m_Dialog->m_VolumeSlider.SetPos((unsigned int)(volume*100));
}

CBtMfcGuiDlg::CBtMfcGuiDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBtMfcGuiDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_Player = NULL;
}

void CBtMfcGuiDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TRACK_SLIDER,     m_TrackSlider);
    DDX_Control(pDX, IDC_VOLUME_SLIDER,    m_VolumeSlider);
    DDX_Control(pDX, IDC_STREAM_INFO_LIST, m_StreamInfoList);
}

BEGIN_MESSAGE_MAP(CBtMfcGuiDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
    ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
    ON_BN_CLICKED(IDC_OPEN_BUTTON, OnBnClickedOpenButton)
    ON_BN_CLICKED(IDC_PLAY_BUTTON, OnBnClickedPlayButton)
    ON_BN_CLICKED(IDC_PAUSE_BUTTON, OnBnClickedPauseButton)
    ON_BN_CLICKED(IDC_STOP_BUTTON, OnBnClickedStopButton)
    ON_WM_HSCROLL()
    ON_BN_CLICKED(IDC_SET_INPUT_BUTTON, &CBtMfcGuiDlg::OnBnClickedSetInputButton)
END_MESSAGE_MAP()


// CBtMfcGuiDlg message handlers

BOOL CBtMfcGuiDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

    // create the BlueTune player
    m_Player = new MfcPlayer(this);
    
	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    // setup the sliders
	m_TrackSlider.SetRange(0, 500);
    m_VolumeSlider.SetRange(0, 100);

    // setup the stream info list
    m_StreamInfoList.InsertColumn(0, "Name", LVCFMT_LEFT, 100);
    m_StreamInfoList.InsertColumn(1, "Value", LVCFMT_LEFT, 200, 0);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CBtMfcGuiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CBtMfcGuiDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CBtMfcGuiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CBtMfcGuiDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    OnCancel();
    delete m_Player;
    m_Player = NULL;
}

void CBtMfcGuiDlg::OnBnClickedOpenButton()
{
    // open a file
	CFileDialog *dialog;

	dialog = new CFileDialog(TRUE, 
							"mp3",
							TEXT(""),
							OFN_FILEMUSTEXIST    | 
							OFN_HIDEREADONLY     |
							OFN_EXPLORER,
							TEXT("Audio Files|*.mpg;*.mp1;*.mp2;*.mp3;*.flac;*.ogg;*.wav;*.aif;*.aiff;*.mp4;*.m4a;*.wma|All Files|*.*||"));


	INT_PTR ret;
	ret = dialog->DoModal();
	if (ret == IDOK) {
        // a file was selected
        ClearInfo();
		m_Player->SetInput(dialog->GetPathName());
	}

	delete dialog;
	
}

void CBtMfcGuiDlg::OnBnClickedPlayButton()
{
    m_Player->Play();
}

void CBtMfcGuiDlg::OnBnClickedPauseButton()
{
    m_Player->Pause();
}

void CBtMfcGuiDlg::OnBnClickedStopButton()
{
    m_Player->Stop();
}

void CBtMfcGuiDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
    if (pScrollBar->m_hWnd == m_TrackSlider.m_hWnd) {
        switch (nSBCode) {
          case SB_LINELEFT:
          case SB_LINERIGHT:
          case SB_PAGELEFT:
          case SB_PAGERIGHT:
            if (m_Player->m_Scrolling == FALSE) {
                m_Player->m_Scrolling = TRUE;
                //m_Player->Pause();
            }
            break;

          case SB_ENDSCROLL: 
            m_Player->SeekToPosition(m_TrackSlider.GetPos(), m_TrackSlider.GetRangeMax()-m_TrackSlider.GetRangeMin());
            if (m_Player->m_Scrolling) {
                m_Player->m_Scrolling = FALSE; 
                //if (m_Player->m_State == XA_PLAYER_STATE_PLAYING) {
                //    m_Player->Play();
                //}
            }
            break;

          case SB_THUMBTRACK:
            if (m_Player->m_Scrolling == FALSE) {
                m_Player->m_Scrolling = TRUE;
                //m_Player->Pause();
            }
            break;

          case SB_THUMBPOSITION: 
            //m_Player->Seek(nPos, 400);
            break;
        }
    } else if (pScrollBar->m_hWnd == m_VolumeSlider.m_hWnd) {
        switch (nSBCode) {
          case SB_THUMBTRACK:
              m_Player->SetVolume((float)m_VolumeSlider.GetPos()/((float)m_VolumeSlider.GetRangeMax()-(float)m_VolumeSlider.GetRangeMin()));
              break;

          case SB_THUMBPOSITION: 
              m_Player->SetVolume((float)nPos/((float)m_VolumeSlider.GetRangeMax()-(float)m_VolumeSlider.GetRangeMin()));
              break;
        }
    }

    CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CBtMfcGuiDlg::OnBnClickedSetInputButton()
{
    char input_name[1024];
    GetDlgItemText(IDC_INPUT_NAME_EDIT, input_name, sizeof(input_name));
    ClearInfo();
    m_Player->SetInput(input_name);
}

void
CBtMfcGuiDlg::ClearInfo()
{
    m_StreamInfoList.DeleteAllItems();
}

void
CBtMfcGuiDlg::UpdateInfo(const char* name, const char* value)
{
    unsigned int nbi = m_StreamInfoList.GetItemCount();
    CString this_name = name;
    bool    item_set = false;
    for (unsigned int i=0; i<nbi; i++) {
        CString item_name = m_StreamInfoList.GetItemText(i, 0);
        if (this_name == item_name) {
            m_StreamInfoList.SetItemText(i, 1, value);
            item_set = true;
            break;
        }
    }
    if (!item_set) {
        m_StreamInfoList.InsertItem(nbi, name);
        m_StreamInfoList.SetItemText(nbi, 1, value);
    }
}

