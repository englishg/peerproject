//
// SaveFilterAsDlg.h
//
// This file is part of PeerProject (peerproject.org) � 2008-2010
// Portions copyright Shareaza Development Team, 2002-2007.
//
// PeerProject is free software. You may redistribute and/or modify it
// under the terms of the GNU Affero General Public License
// as published by the Free Software Foundation (fsf.org);
// version 3 or later at your option. (AGPLv3)
//
// PeerProject is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Affero General Public License 3.0 for details:
// (http://www.gnu.org/licenses/agpl.html)
//
// Original Author: roo_koo_too@yahoo.com
//

#pragma once

#include "DlgSkinDialog.h"
// CSaveFilterAsDlg dialog

class CSaveFilterAsDlg : public CSkinDialog
{
public:
	CSaveFilterAsDlg(CWnd* pParent = NULL);
	virtual ~CSaveFilterAsDlg();

	enum { IDD = IDD_FILTER_SAVE_AS };

// Dialog Data
public:
	CString m_sName;	// The current filter name

// Implementation
protected:
	//{{AFX_MSG(CSaveFilterAsDlg)
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);
	//}}AFX_MSG

public:
	afx_msg void OnEnChangeName();
	void OnOK();

protected:
	DECLARE_MESSAGE_MAP()
};
