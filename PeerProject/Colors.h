//
// Colors.h
//
// This file is part of PeerProject (peerproject.org) � 2009-2010
//
// PeerProject is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 3
// of the License, or later version (at your option).
//
// PeerProject is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License 3.0
// along with PeerProject; if not, write to Free Software Foundation, Inc.
// 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA  (www.fsf.org)
//

#pragma once

#include "Skin.h"

class CColors
{
// Construction
public:
	CColors();
	virtual ~CColors();

public:
	BOOL		m_bCustom;

	COLORREF	m_crWindow;
	COLORREF	m_crMidtone;
	COLORREF	m_crText;
	COLORREF	m_crHiText;
	COLORREF	m_crHiBorder;
	COLORREF	m_crHiBorderIn;
	COLORREF	m_crHighlight;
	COLORREF	m_crBackNormal;
	COLORREF	m_crBackSel;
	COLORREF	m_crBackCheck;
	COLORREF	m_crBackCheckSel;
	COLORREF	m_crMargin;
	COLORREF	m_crBorder;
	COLORREF	m_crShadow;
	COLORREF	m_crCmdText;
	COLORREF	m_crCmdTextSel;
	COLORREF	m_crDisabled;

	CBrush		m_brDialog;
	COLORREF	m_crDialog;
	COLORREF	m_crPanelText;
	COLORREF	m_crPanelBorder;
	COLORREF	m_crPanelBack;
	COLORREF	m_crBannerText;
	COLORREF	m_crBannerBack;
	COLORREF	m_crSchemaRow[2];

	COLORREF	m_crTipBack;
	COLORREF	m_crTipText;
	COLORREF	m_crTipBorder;
	COLORREF	m_crTipWarnings;

	COLORREF	m_crTaskPanelBack;
	COLORREF	m_crTaskBoxClient;
	COLORREF	m_crTaskBoxCaptionBack;
	COLORREF	m_crTaskBoxPrimaryBack;
	COLORREF	m_crTaskBoxCaptionText;
	COLORREF	m_crTaskBoxPrimaryText;
	COLORREF	m_crTaskBoxCaptionHover;

	COLORREF	m_crMediaWindowBack;
	COLORREF	m_crMediaWindowText;
	COLORREF	m_crMediaStatusBack;
	COLORREF	m_crMediaStatusText;
	COLORREF	m_crMediaPanelBack;
	COLORREF	m_crMediaPanelText;
	COLORREF	m_crMediaPanelActiveBack;
	COLORREF	m_crMediaPanelActiveText;
	COLORREF	m_crMediaPanelCaptionBack;
	COLORREF	m_crMediaPanelCaptionText;

	COLORREF	m_crTrafficWindowBack;
	COLORREF	m_crTrafficWindowText;
	COLORREF	m_crTrafficWindowGrid;

	COLORREF	m_crMonitorHistoryBack;
	COLORREF	m_crMonitorHistoryBackMax;
	COLORREF	m_crMonitorHistoryText;
	COLORREF	m_crMonitorDownloadLine;
	COLORREF	m_crMonitorUploadLine;
	COLORREF	m_crMonitorDownloadBar;
	COLORREF	m_crMonitorUploadBar;

	COLORREF	m_crNavBarText;
	COLORREF	m_crNavBarTextUp;
	COLORREF	m_crNavBarTextDown;
	COLORREF	m_crNavBarTextHover;
	COLORREF	m_crNavBarTextChecked;
	COLORREF	m_crNavBarShadow;
	COLORREF	m_crNavBarShadowUp;
	COLORREF	m_crNavBarShadowDown;
	COLORREF	m_crNavBarShadowHover;
	COLORREF	m_crNavBarShadowChecked;
	COLORREF	m_crNavBarOutline;
	COLORREF	m_crNavBarOutlineUp;
	COLORREF	m_crNavBarOutlineDown;
	COLORREF	m_crNavBarOutlineHover;
	COLORREF	m_crNavBarOutlineChecked;

	COLORREF	m_crRatingNull;
	COLORREF	m_crRating0;
 	COLORREF	m_crRating1;
 	COLORREF	m_crRating2;
 	COLORREF	m_crRating3;
 	COLORREF	m_crRating4;
 	COLORREF	m_crRating5;

 	COLORREF	m_crRichdocBack;
 	COLORREF	m_crRichdocText;
	COLORREF	m_crRichdocHeading;
	COLORREF	m_crTextAlert;
	COLORREF	m_crTextStatus;
	COLORREF	m_crTextLink;
 	COLORREF	m_crTextLinkHot;

 	COLORREF	m_crChatIn;
	COLORREF	m_crChatOut;
	COLORREF	m_crChatNull;
	COLORREF	m_crSearchNull;
	COLORREF	m_crSearchExists;
	COLORREF	m_crSearchExistsHit;
	COLORREF	m_crSearchExistsSelected;
	COLORREF	m_crSearchQueued;
	COLORREF	m_crSearchQueuedHit;
	COLORREF	m_crSearchQueuedSelected;
	COLORREF	m_crSearchGhostrated;
	COLORREF	m_crSearchHighrated;
	COLORREF	m_crSearchCollection;
	COLORREF	m_crSearchTorrent;
	COLORREF	m_crTransferSource;
	COLORREF	m_crTransferRanges;
	COLORREF	m_crTransferCompleted;
	COLORREF	m_crTransferVerifyPass;
	COLORREF	m_crTransferVerifyFail;
	COLORREF	m_crTransferCompletedSelected;
	COLORREF	m_crTransferVerifyPassSelected;
	COLORREF	m_crTransferVerifyFailSelected;
	COLORREF	m_crNetworkNull;
	COLORREF	m_crNetworkG1;
	COLORREF	m_crNetworkG2;
	COLORREF	m_crNetworkED2K;
	COLORREF	m_crNetworkUp;
	COLORREF	m_crNetworkDown;
	COLORREF	m_crSecurityAllow;
	COLORREF	m_crSecurityDeny;

	COLORREF	m_crDropdownBox;
	COLORREF	m_crDropdownText;
 	COLORREF	m_crResizebarEdge;
	COLORREF	m_crResizebarFace;
	COLORREF	m_crResizebarShadow;
	COLORREF	m_crResizebarHighlight;
	COLORREF	m_crFragmentShaded;
	COLORREF	m_crFragmentComplete;
	COLORREF	m_crFragmentSource1;
	COLORREF	m_crFragmentSource2;
	COLORREF	m_crFragmentSource3;
	COLORREF	m_crFragmentSource4;
	COLORREF	m_crFragmentSource5;
	COLORREF	m_crFragmentSource6;
	COLORREF	m_crFragmentPass;
	COLORREF	m_crFragmentFail;
	COLORREF	m_crFragmentRequest;
	COLORREF	m_crFragmentBorder;
	COLORREF	m_crFragmentBorderSelected;
	COLORREF	m_crFragmentBorderSimpleBar;
	COLORREF	m_crFragmentBorderSimpleBarSelected;

	COLORREF	m_crSysWindow;
	COLORREF	m_crSysBtnFace;
	COLORREF	m_crSysBorders;
	COLORREF	m_crSys3DShadow;
	COLORREF	m_crSys3DHighlight;
	COLORREF	m_crSysActiveCaption;

	void		Load();
	void		OnSysColorChange();
	void		CalculateColors(BOOL bCustom = FALSE);

	static COLORREF	CalculateColor(COLORREF crFore, COLORREF crBack, int nAlpha);
};

extern CColors Colors;