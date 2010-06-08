//
// NeighboursWithG2.h
//
// This file is part of PeerProject (peerproject.org) � 2008-2010
// Portions Copyright Shareaza Development Team, 2002-2007.
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

// Adds 2 methods helpful for Gnutella2 that look at the list of neighbours
// http://sourceforge.net/apps/mediawiki/shareaza/index.php?title=Developers.Code.CNeighboursWithG2

#pragma once

#include "NeighboursWithG1.h"

class CG2Neighbour;
class CG2Packet;

// Add methods helpful for Gnutella that need to look at the list of computers we're connected to
class CNeighboursWithG2 : public CNeighboursWithG1 // Continue the inheritance column CNeighbours : CNeighboursWithConnect : Routing : ED2K : G2 : G1 : CNeighboursBase
{
protected:
	// Constructor/destructor don't do anything: Nothing that CNeighboursWithG2 adds to CNeighbours needs to be set up or put away
	CNeighboursWithG2();
	virtual ~CNeighboursWithG2();

public:
	// Methods implimented by several classes in the CNeighbours inheritance column
	// Set the ping route duration and setup the hub horizon pool
	virtual void Connect();

public:
	// Make and return a query web packet with IP addresses from the neighbours list and the Gnutella2 host cache
	CG2Packet* CreateQueryWeb(const Hashes::Guid& oGUID, bool bWithHubs, CNeighbour* pExcept = NULL);

	// Return a random Gnutella2 hub neighbour that isn't pExcept and doesn't know about pGUID
	CG2Neighbour* GetRandomHub(CG2Neighbour* pExcept, const Hashes::Guid& oGUID);
};
