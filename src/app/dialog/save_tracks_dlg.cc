/*
 * InfraRecorder - CD/DVD burning software
 * Copyright (C) 2006-2012 Christian Kindahl
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.hh"
#include <ckcore/directory.hh>
#include "save_tracks_dlg.hh"
#include "settings.hh"
#include "string_table.hh"
#include "lang_util.hh"

CSaveTracksDlg::CSaveTracksDlg()
{
    m_pEncoder = NULL;
}

CSaveTracksDlg::~CSaveTracksDlg()
{
}

bool CSaveTracksDlg::Translate()
{
    if (g_LanguageSettings.m_pLngProcessor == NULL)
        return false;

    CLngProcessor *pLng = g_LanguageSettings.m_pLngProcessor;
    
    // Make sure that there is a edittrack translation section.
    if (!pLng->EnterSection(_T("savetracks")))
        return false;

    // Translate.
    TCHAR *szStrValue;

    if (pLng->GetValuePtr(IDOK,szStrValue))
        SetDlgItemText(IDOK,szStrValue);
    if (pLng->GetValuePtr(IDCANCEL,szStrValue))
        SetDlgItemText(IDCANCEL,szStrValue);
    if (pLng->GetValuePtr(IDD_SAVETRACKSDLG,szStrValue))			// Title.
        SetWindowText(szStrValue);
    if (pLng->GetValuePtr(IDC_TARGETSTATIC,szStrValue))
        SetDlgItemText(IDC_TARGETSTATIC,szStrValue);
    if (pLng->GetValuePtr(IDC_AUDIOFORMATSTATIC,szStrValue))
        SetDlgItemText(IDC_AUDIOFORMATSTATIC,szStrValue);
    if (pLng->GetValuePtr(IDC_AUDIOFORMATBUTTON,szStrValue))
        SetDlgItemText(IDC_AUDIOFORMATBUTTON,szStrValue);

    return true;
}

LRESULT CSaveTracksDlg::OnInitDialog(UINT uMsg,WPARAM wParam,LPARAM lParam,BOOL &bHandled)
{
    CenterWindow(GetParent());

    // Setup the target edit control.
    ::SendMessage(GetDlgItem(IDC_TARGETEDIT),EM_SETLIMITTEXT,MAX_PATH - 1,0);
    SetDlgItemText(IDC_TARGETEDIT,g_SaveTracksSettings.m_szTarget);

    // Setup the audio format combo box.
    m_AudioFormatCombo = GetDlgItem(IDC_AUDIOFORMATCOMBO);
    m_AudioFormatCombo.ResetContent();

    // The first item is always wave. Not the wave encoder, but the cdrtools
    // default encoder.
    m_AudioFormatCombo.AddString(_T("Wave"));

    for (unsigned int i = 0; i < g_CodecManager.m_Codecs.size(); i++)
    {
        // Skip any installed wave codecs since cdrtools has its own.
        if (!lstrcmp(g_CodecManager.m_Codecs[i]->irc_string(IRC_STR_FILEEXT),_T(".wav")))
            continue;

        if (g_CodecManager.m_Codecs[i]->irc_capabilities() & IRC_HAS_ENCODER)
        {
            m_AudioFormatCombo.AddString(g_CodecManager.m_Codecs[i]->irc_string(IRC_STR_ENCODER));
            m_AudioFormatCombo.SetItemData(m_AudioFormatCombo.GetCount() - 1,(DWORD_PTR)g_CodecManager.m_Codecs[i]);
        }
    }

    m_AudioFormatCombo.SetCurSel(0);
    ::EnableWindow(GetDlgItem(IDC_AUDIOFORMATBUTTON),FALSE);

    // Translate the window.
    Translate();

    return TRUE;
}

LRESULT CSaveTracksDlg::OnOK(WORD wNotifyCode,WORD wID,HWND hWndCtl,BOOL &bHandled)
{
    // Target folder.
    TCHAR szFolderPath[MAX_PATH];
    GetDlgItemText(IDC_TARGETEDIT,szFolderPath,MAX_PATH - 1);

    if (szFolderPath == NULL || lstrlen(szFolderPath) < 3 || szFolderPath[1] != ':')
    {
        MessageBox(lngGetString(ERROR_TARGETFOLDER),lngGetString(GENERAL_ERROR),MB_OK | MB_ICONERROR);
        return FALSE;
    }

    // Check if the target path exist.
    const ckcore::Path Path(szFolderPath);
    if (!ckcore::Directory::exist(Path))
    {
        if (MessageBox(lngSlowFormatStr(CONFIRM_CREATE_DIR_PATH,szFolderPath).c_str(),
                       lngGetString(GENERAL_QUESTION),
                       MB_YESNO | MB_ICONQUESTION) != IDYES)
        {
            return FALSE;
        }

        if (!ckcore::Directory::create(Path))
        {
            MessageBox(lngSlowFormatStr(CANNOT_CREATE_DIR_PATH,szFolderPath).c_str(),
                       lngGetString(GENERAL_ERROR),MB_OK | MB_ICONERROR);
            return FALSE;
        }
    }

    lstrcpy(g_SaveTracksSettings.m_szTarget,szFolderPath);

    // Encoder.
    if (m_AudioFormatCombo.GetCurSel() == 0)
        m_pEncoder = NULL;
    else
        m_pEncoder = (CCodec *)m_AudioFormatCombo.GetItemData(m_AudioFormatCombo.GetCurSel());

    EndDialog(wID);
    return FALSE;
}

LRESULT CSaveTracksDlg::OnCancel(WORD wNotifyCode,WORD wID,HWND hWndCtl,BOOL &bHandled)
{
    EndDialog(wID);
    return FALSE;
}

LRESULT CSaveTracksDlg::OnAudioFormatChange(WORD wNotifyCode,WORD wID,HWND hWndCtl,BOOL &bHandled)
{
    if (m_AudioFormatCombo.GetCurSel() == 0)
    {
        ::EnableWindow(GetDlgItem(IDC_AUDIOFORMATBUTTON),FALSE);
    }
    else
    {
        CCodec *pEncoder = (CCodec *)m_AudioFormatCombo.GetItemData(m_AudioFormatCombo.GetCurSel());
        ::EnableWindow(GetDlgItem(IDC_AUDIOFORMATBUTTON),pEncoder->irc_capabilities() & IRC_HAS_CONFIG);
    }

    return 0;
}

LRESULT CSaveTracksDlg::OnClickedAudioFormatButton(WORD wNotifyCode,WORD wID,HWND hWndCtl,BOOL &bHandled)
{
    // This should never happen.
    if (m_AudioFormatCombo.GetCurSel() == 0)
        return 0;

    CCodec *pEncoder = (CCodec *)m_AudioFormatCombo.GetItemData(m_AudioFormatCombo.GetCurSel());
    pEncoder->irc_encode_config();

    return 0;
}

LRESULT CSaveTracksDlg::OnClickedBrowseButton(WORD wNotifyCode,WORD wID,HWND hWndCtl,BOOL &bHandled)
{
    CFolderDialog FolderDialog(m_hWnd,lngGetString(MISC_SPECIFYTRACKFOLDER),BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS);
    
    if (FolderDialog.DoModal() == IDOK)
        SetDlgItemText(IDC_TARGETEDIT,FolderDialog.GetFolderPath());

    return 0;
}

CCodec *CSaveTracksDlg::GetEncoder()
{
    return m_pEncoder;
}
