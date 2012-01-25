//
// HostCache.cpp
//
// This file is part of PeerProject (peerproject.org) � 2008-2012
// Portions copyright Shareaza Development Team, 2002-2008.
//
// PeerProject is free software; you can redistribute it and/or
// modify it under the terms of the GNU Affero General Public License
// as published by the Free Software Foundation (fsf.org);
// either version 3 of the License, or later version at your option.
//
// PeerProject is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Affero General Public License 3.0 (AGPLv3) for details:
// (http://www.gnu.org/licenses/agpl.html)
//

#include "StdAfx.h"
#include "Settings.h"
#include "PeerProject.h"
#include "HostCache.h"
#include "DiscoveryServices.h"
#include "Buffer.h"
#include "EDPacket.h"
#include "Kademlia.h"
#include "Neighbours.h"
#include "Network.h"
#include "Security.h"
#include "VendorCache.h"
#include "XML.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif	// Filename

CHostCache HostCache;


//////////////////////////////////////////////////////////////////////
// CHostCache construction

CHostCache::CHostCache()
	: Gnutella2	( PROTOCOL_G2 )
	, Gnutella1	( PROTOCOL_G1 )
	, G1DNA 	( PROTOCOL_G1 )
	, eDonkey	( PROTOCOL_ED2K )
	, BitTorrent( PROTOCOL_BT )
	, Kademlia	( PROTOCOL_KAD )
	, DC		( PROTOCOL_DC )
	, m_tLastPruneTime ( 0 )
{
	m_pList.AddTail( &Gnutella1 );
	m_pList.AddTail( &Gnutella2 );
	m_pList.AddTail( &eDonkey );
	m_pList.AddTail( &G1DNA );
	m_pList.AddTail( &BitTorrent );
	m_pList.AddTail( &Kademlia );
	m_pList.AddTail( &DC );
}

//////////////////////////////////////////////////////////////////////
// CHostCache core operations

void CHostCache::Clear()
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		pCache->Clear();
	}
}

BOOL CHostCache::Load()
{
	const CString strFile = Settings.General.UserPath + _T("\\Data\\HostCache.dat");

	BOOL bSuccess = FALSE;

	CFile pFile;
	if ( pFile.Open( strFile, CFile::modeRead | CFile::shareDenyWrite | CFile::osSequentialScan ) )
	{
		try
		{
			CArchive ar( &pFile, CArchive::load, 262144 );	// 256 KB buffer
			try
			{
				CQuickLock oLock( m_pSection );

				Clear();

				Serialize( ar );
				ar.Close();

				bSuccess = TRUE;	// Success
			}
			catch ( CException* pException )
			{
				ar.Abort();
				pFile.Abort();
				pException->Delete();
			}
		}
		catch ( CException* pException )
		{
			pFile.Abort();
			pException->Delete();
		}

		pFile.Close();
	}

	if ( eDonkey.GetNewest() == NULL )
		CheckMinimumServers();

	if ( bSuccess )
		return TRUE;

	theApp.Message( MSG_ERROR, _T("Failed to load host cache: %s"), strFile );
	return FALSE;
}

BOOL CHostCache::Save()
{
	const CString strFile = Settings.General.UserPath + _T("\\Data\\HostCache.dat");
	const CString strTemp = Settings.General.UserPath + _T("\\Data\\HostCache.tmp");

	CFile pFile;
	if ( ! pFile.Open( strTemp, CFile::modeWrite | CFile::modeCreate | CFile::shareExclusive | CFile::osSequentialScan ) )
	{
		DeleteFile( strTemp );
		theApp.Message( MSG_ERROR, _T("Failed to save host cache: %s"), strTemp );
		return FALSE;
	}

	try
	{
		CArchive ar( &pFile, CArchive::store, 262144 );	// 256 KB buffer
		try
		{
			CQuickLock oLock( m_pSection );

			Serialize( ar );
			ar.Close();
		}
		catch ( CException* pException )
		{
			ar.Abort();
			pFile.Abort();
			pException->Delete();
			DeleteFile( strTemp );
			theApp.Message( MSG_ERROR, _T("Failed to save host cache: %s"), strTemp );
			return FALSE;
		}
		pFile.Close();
	}
	catch ( CException* pException )
	{
		pFile.Abort();
		pException->Delete();
		DeleteFile( strTemp );
		theApp.Message( MSG_ERROR, _T("Failed to save host cache: %s"), strTemp );
		return FALSE;
	}

	if ( ! MoveFileEx( strTemp, strFile, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING ) )
	{
		DeleteFile( strTemp );
		theApp.Message( MSG_ERROR, _T("Failed to save host cache: %s"), strFile );
		return FALSE;
	}

	return TRUE;
}

#define HOSTCACHE_SER_VERSION		1000	// 19
// nVersion History:
// 14 - Added m_sCountry
// 15 - Added m_bDHT and m_oBtGUID (Ryo-oh-ki)
// 16 - Added m_nUDPPort, m_oGUID and m_nKADVersion (Ryo-oh-ki)
// 17 - Added m_tConnect (Ryo-oh-ki)
// 18 - Added m_sUser and m_sPass (Ryo-oh-ki)
// 19 - Added m_sAddress (Ryo-oh-ki)
// 1000 - (PeerProject 1.0) (19)

void CHostCache::Serialize(CArchive& ar)
{
	int nVersion = HOSTCACHE_SER_VERSION;	// ToDo: INTERNAL_VERSION

	if ( ar.IsStoring() )
	{
		ar << nVersion;
		ar.WriteCount( m_pList.GetCount() );

		for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
		{
			CHostCacheList* pCache = m_pList.GetNext( pos );
			ar << pCache->m_nProtocol;
			pCache->Serialize( ar, nVersion );
		}
	}
	else // Loading
	{
		ar >> nVersion;
		if ( nVersion < 14 ) return;

		for ( DWORD_PTR nCount = ar.ReadCount() ; nCount > 0 ; nCount-- )
		{
			PROTOCOLID nProtocol;
			ar >> nProtocol;

			for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
			{
				CHostCacheList* pCache = m_pList.GetNext( pos );
				if ( pCache->m_nProtocol == nProtocol )
				{
					pCache->Serialize( ar, nVersion );
					break;
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCache prune old hosts

void CHostCache::PruneOldHosts()
{
	const DWORD tNow = static_cast< DWORD >( time( NULL ) );
	if ( tNow > m_tLastPruneTime + 90 )		// Every minute+
	{
		for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
		{
			m_pList.GetNext( pos )->PruneOldHosts( tNow );
		}
		m_tLastPruneTime = tNow;
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCache forwarding operations

CHostCacheHostPtr CHostCache::Find(const IN_ADDR* pAddress) const
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		if ( CHostCacheHostPtr pHost = pCache->Find( pAddress ) )
			return pHost;
	}
	return NULL;
}

CHostCacheHostPtr CHostCache::Find(LPCTSTR szAddress) const
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		if ( CHostCacheHostPtr pHost = pCache->Find( szAddress ) )
			return pHost;
	}
	return NULL;
}

BOOL CHostCache::Check(const CHostCacheHostPtr pHost) const
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		if ( pCache->Check( pHost ) )
			return TRUE;
	}
	return FALSE;
}

void CHostCache::Remove(CHostCacheHostPtr pHost)
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		pCache->Remove( pHost );
	}
}

void CHostCache::SanityCheck()
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		pCache->SanityCheck();
	}
}

void CHostCache::OnFailure(const IN_ADDR* pAddress, WORD nPort, PROTOCOLID nProtocol, bool bRemove)
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		if ( nProtocol == PROTOCOL_NULL || nProtocol == pCache->m_nProtocol )
			pCache->OnFailure( pAddress, nPort, bRemove );
	}
}

void CHostCache::OnSuccess(const IN_ADDR* pAddress, WORD nPort, PROTOCOLID nProtocol, bool bUpdate)
{
	for ( POSITION pos = m_pList.GetHeadPosition() ; pos ; )
	{
		CHostCacheList* pCache = m_pList.GetNext( pos );
		if ( nProtocol == PROTOCOL_NULL || nProtocol == pCache->m_nProtocol )
			pCache->OnSuccess( pAddress, nPort, bUpdate );
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList construction

CHostCacheList::CHostCacheList(PROTOCOLID nProtocol)
	: m_nProtocol	( nProtocol )
	, m_nCookie		( 0 )
{
}

CHostCacheList::~CHostCacheList()
{
	Clear();
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList clear

void CHostCacheList::Clear()
{
	CQuickLock oLock( m_pSection );

	for ( CHostCacheMapItr i = m_Hosts.begin() ; i != m_Hosts.end() ; ++i )
	{
		delete (*i).second;
	}
	m_Hosts.clear();
	m_HostsTime.clear();

	m_nCookie++;
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList host add

CHostCacheHostPtr CHostCacheList::Add(const IN_ADDR* pAddress, WORD nPort, DWORD tSeen, LPCTSTR pszVendor, DWORD nUptime, DWORD nCurrentLeaves, DWORD nLeafLimit, LPCTSTR szAddress)
{
	ASSERT( pAddress || szAddress );

	// Don't add invalid addresses
	if ( ! nPort )
		return NULL;

	if ( pAddress && ! pAddress->S_un.S_un_b.s_b1 )
		return NULL;

	// Don't add own firewalled IPs
	if ( pAddress && Network.IsFirewalledAddress( pAddress, TRUE ) )
		return NULL;

	// Check against IANA Reserved addresses
	if ( pAddress && Network.IsReserved( pAddress ) )
		return NULL;

	// Check security settings, don't add blocked IPs
	if ( pAddress && Security.IsDenied( pAddress ) )
		return NULL;

	// Try adding it to the cache. (duplicates will be rejected)
	return AddInternal( pAddress, nPort, tSeen, pszVendor, nUptime, nCurrentLeaves, nLeafLimit, szAddress );
}

BOOL CHostCacheList::Add(LPCTSTR pszHost, DWORD tSeen, LPCTSTR pszVendor, DWORD nUptime, DWORD nCurrentLeaves, DWORD nLeafLimit)
{
	CString strHost( pszHost );
	strHost.Trim();

	int nPos = strHost.ReverseFind( _T(' ') );
	if ( nPos > 0 )
	{
		CString strTime = strHost.Mid( nPos + 1 );
		strHost = strHost.Left( nPos );
		strHost.TrimRight();

		tSeen = TimeFromString( strTime );
	}

	nPos = strHost.Find( _T(':') );
	if ( nPos < 0 ) return FALSE;

	int nPort = GNUTELLA_DEFAULT_PORT;
	if ( _stscanf( strHost.Mid( nPos + 1 ), _T("%i"), &nPort ) != 1 ||
		nPort <= 0 || nPort >= 65536 ) return FALSE;
	strHost = strHost.Left( nPos );

	DWORD nAddress = inet_addr( CT2CA( (LPCTSTR)strHost ) );
	if ( nAddress == INADDR_NONE ) return FALSE;

	return ( Add( (IN_ADDR*)&nAddress, (WORD)nPort, tSeen, pszVendor, nUptime, nCurrentLeaves, nLeafLimit ) != NULL );
}

// This function actually adds the remote client to the host cache.
// Private, but used by the public functions.  No security checking, etc.
CHostCacheHostPtr CHostCacheList::AddInternal(const IN_ADDR* pAddress, WORD nPort, DWORD tSeen, LPCTSTR pszVendor, DWORD nUptime, DWORD nCurrentLeaves, DWORD nLeafLimit, LPCTSTR szAddress)
{
	ASSERT( pAddress || szAddress );
	ASSERT( nPort );

	SOCKADDR_IN saHost;
	if ( ! pAddress )
	{
		// Try to quick resolve dotted IP address
		if ( ! Network.Resolve( szAddress, nPort, &saHost, FALSE ) )
			return FALSE;	// Cannot resolve

		pAddress = &saHost.sin_addr;
		nPort = ntohs( saHost.sin_port );
		if ( pAddress->s_addr != INADDR_ANY )
			szAddress = NULL;
	}

	CQuickLock oLock( m_pSection );

	// Check if we already have the host
	CHostCacheHostPtr pHost = Find( pAddress );
	if ( ! pHost && szAddress )
		pHost = Find( szAddress );
	if ( ! pHost )
	{
		// Create new host
		pHost = new CHostCacheHost( m_nProtocol );
		if ( pHost )
		{
			PruneHosts();

			pHost->m_pAddress = *pAddress;
			if ( szAddress ) pHost->m_sAddress = szAddress;
			pHost->m_sAddress = pHost->m_sAddress.SpanExcluding( _T(":") );

			pHost->Update( nPort, tSeen, pszVendor, nUptime, nCurrentLeaves, nLeafLimit );

			// Add host to map and index
			m_Hosts.insert( CHostCacheMapPair( pHost->m_pAddress, pHost ) );
			m_HostsTime.insert( pHost );

			m_nCookie++;
		}
	}
	else
	{
		if ( szAddress ) pHost->m_sAddress = szAddress;
		pHost->m_sAddress = pHost->m_sAddress.SpanExcluding( _T(":") );

		Update( pHost, nPort, tSeen, pszVendor, nUptime, nCurrentLeaves, nLeafLimit );
	}

	ASSERT( m_Hosts.size() == m_HostsTime.size() );

	return pHost;
}

void CHostCacheList::Update(CHostCacheHostPtr pHost, WORD nPort, DWORD tSeen, LPCTSTR pszVendor, DWORD nUptime, DWORD nCurrentLeaves, DWORD nLeafLimit)
{
	CQuickLock oLock( m_pSection );

	ASSERT( pHost );
	ASSERT( m_Hosts.size() == m_HostsTime.size() );

	// Update host
	if ( pHost->Update( nPort, tSeen, pszVendor, nUptime, nCurrentLeaves, nLeafLimit ) )
	{
		// Remove host from old and now invalid position
		m_HostsTime.erase( std::find( m_HostsTime.begin(), m_HostsTime.end(), pHost ) );

		// Add host to new sorted position
		m_HostsTime.insert( pHost );

		ASSERT( m_Hosts.size() == m_HostsTime.size() );
	}

	m_nCookie++;
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList host remove

CHostCacheMapItr CHostCacheList::Remove(CHostCacheHostPtr pHost)
{
	CQuickLock oLock( m_pSection );

	CHostCacheIterator j = std::find( m_HostsTime.begin(), m_HostsTime.end(), pHost );
	if ( j == m_HostsTime.end() )
		return m_Hosts.end();	// Wrong cache
	m_HostsTime.erase( j );

	CHostCacheMapItr i = std::find_if( m_Hosts.begin(), m_Hosts.end(), std::bind2nd( is_host(), pHost ) );

	ASSERT( i != m_Hosts.end() );
	i = m_Hosts.erase( i );
	ASSERT( m_Hosts.size() == m_HostsTime.size() );

	delete pHost;
	m_nCookie++;

	return i;
}

CHostCacheMapItr CHostCacheList::Remove(const IN_ADDR* pAddress)
{
	CQuickLock oLock( m_pSection );

	CHostCacheMapItr i = m_Hosts.find( *pAddress );
	if ( i == m_Hosts.end() )
		return m_Hosts.end();	// Wrong cache/address

	return Remove( (*i).second );
}

void CHostCacheList::SanityCheck()
{
	CQuickLock oLock( m_pSection );

	for ( CHostCacheMapItr i = m_Hosts.begin() ; i != m_Hosts.end() ; )
	{
		CHostCacheHostPtr pHost = (*i).second;
		if ( Security.IsDenied( &pHost->m_pAddress ) ||
			( pHost->m_pVendor && Security.IsVendorBlocked( pHost->m_pVendor->m_sCode ) ) )
		{
			i = Remove( pHost );
		}
		else
			++i;
	}
}

void CHostCacheList::OnResolve(LPCTSTR szAddress, const IN_ADDR* pAddress, WORD nPort)
{
	CQuickLock oLock( m_pSection );

	CHostCacheHostPtr pHost = Find( szAddress );
	if ( pHost )
	{
		// Remove from old place
		m_Hosts.erase( std::find_if( m_Hosts.begin(), m_Hosts.end(),
			std::bind2nd( is_host(), pHost ) ) );

		pHost->m_pAddress = *pAddress;
		pHost->m_nPort = nPort;
		pHost->m_sCountry = theApp.GetCountryCode( pHost->m_pAddress );

		// Add to new place
		m_Hosts.insert( CHostCacheMapPair( pHost->m_pAddress, pHost ) );

		ASSERT( m_Hosts.size() == m_HostsTime.size() );
	}
	else
	{
		Add( pAddress, nPort, 0, 0, 0, 0, 0, szAddress );
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList failure processor

void CHostCacheList::OnFailure(const IN_ADDR* pAddress, WORD nPort, bool bRemove)
{
	CQuickLock oLock( m_pSection );

	CHostCacheHostPtr pHost = Find( pAddress );
	if ( pHost && ( ! nPort || pHost->m_nPort == nPort ) )
	{
		m_nCookie++;
		pHost->m_nFailures++;

		// Clear current IP address to re-resolve name later
		if ( ! pHost->m_sAddress.IsEmpty() )
			pHost->m_pAddress.s_addr = INADDR_ANY;

		if ( pHost->m_bPriority )
			return;

		if ( bRemove || pHost->m_nFailures > Settings.Connection.FailureLimit )
		{
			Remove( pHost );
		}
		else
		{
			pHost->m_tFailure = static_cast< DWORD >( time( NULL ) );
			pHost->m_bCheckedLocally = TRUE;
		}
	}
}

void CHostCacheList::OnFailure(LPCTSTR szAddress, bool bRemove)
{
	CQuickLock oLock( m_pSection );

	CHostCacheHostPtr pHost = Find( szAddress );
	if ( pHost )
	{
		m_nCookie++;
		pHost->m_nFailures++;
		if ( pHost->m_bPriority )
			return;

		if ( bRemove || pHost->m_nFailures > Settings.Connection.FailureLimit )
		{
			Remove( pHost );
		}
		else
		{
			pHost->m_tFailure = static_cast< DWORD >( time( NULL ) );
			pHost->m_bCheckedLocally = TRUE;
		}
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList failure processor

void CHostCacheList::OnSuccess(const IN_ADDR* pAddress, WORD nPort, bool bUpdate)
{
	CQuickLock oLock( m_pSection );

	CHostCacheHostPtr pHost = Add( const_cast< IN_ADDR* >( pAddress ), nPort );
	if ( pHost && ( ! nPort || pHost->m_nPort == nPort ) )
	{
		m_nCookie++;
		pHost->m_tFailure = 0;
		pHost->m_nFailures = 0;
		pHost->m_bCheckedLocally = TRUE;
		if ( bUpdate )
			Update( pHost, nPort );
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheList query acknowledgment prune (G2)

//void CHostCacheList::PruneByQueryAck()
//{
//	CQuickLock oLock( m_pSection );
//	DWORD tNow = static_cast< DWORD >( time( NULL ) );
//	for ( CHostCacheMap::iterator i = m_Hosts.begin() ; i != m_Hosts.end() ; )
//	{
//		bool bRemoved = false;
//		CHostCacheHostPtr pHost = (*i).second;
//		if ( pHost->m_tAck && tNow - pHost->m_tAck > Settings.Gnutella2.QueryHostDeadline )
//		{
//			pHost->m_tAck = 0;
//			if ( pHost->m_nFailures++ > Settings.Connection.FailureLimit )
//			{
//				m_HostsTime.erase( std::find( m_HostsTime.begin(), m_HostsTime.end(), pHost ) );
//				i = m_Hosts.erase( i );
//				delete pHost;
//				bRemoved = true;
//				m_nCookie++;
//			}
//		}
//		// Don't increment if host was removed
//		if ( ! bRemoved ) i++;
//	}
//}

//////////////////////////////////////////////////////////////////////
// CHostCacheList prune old hosts

void CHostCacheList::PruneOldHosts(DWORD tNow)
{
	CQuickLock oLock( m_pSection );

	for ( CHostCacheMapItr i = m_Hosts.begin() ; i != m_Hosts.end() ; )
	{
		CHostCacheHostPtr pHost = (*i).second;

		// Query acknowledgment prune (G2)
		if ( pHost->m_nProtocol == PROTOCOL_G2 && pHost->m_tAck &&
			tNow > pHost->m_tAck + Settings.Gnutella2.QueryHostDeadline )
		{
			pHost->m_tAck = 0;

			m_nCookie++;
			pHost->m_nFailures++;
		}

		// Discard hosts after repeat failures
		if ( ! pHost->m_bPriority &&
			 ( pHost->m_nFailures > Settings.Connection.FailureLimit ||
			   pHost->IsExpired( tNow ) ) )
		{
			i = Remove( pHost );
		}
		else
			++i;
	}
}


//////////////////////////////////////////////////////////////////////
// Remove several oldest hosts

void CHostCacheList::PruneHosts()
{
	CQuickLock oLock( m_pSection );

	for ( CHostCacheIndex::iterator i = m_HostsTime.end() ;
		m_Hosts.size() > Settings.Gnutella.HostCacheSize && i != m_HostsTime.begin() ; )
	{
		--i;
		CHostCacheHostPtr pHost = (*i);
		if ( ! pHost->m_bPriority )
		{
			i = m_HostsTime.erase( i );
			m_Hosts.erase( std::find_if( m_Hosts.begin(), m_Hosts.end(),
				std::bind2nd( is_host(), pHost ) ) );
			delete pHost;
			m_nCookie++;
		}
	}

	for ( CHostCacheIndex::iterator i = m_HostsTime.end() ;
		m_Hosts.size() > Settings.Gnutella.HostCacheSize && i != m_HostsTime.begin() ; )
	{
		--i;
		CHostCacheHostPtr pHost = (*i);
		i = m_HostsTime.erase( i );
		m_Hosts.erase( std::find_if( m_Hosts.begin(), m_Hosts.end(),
			std::bind2nd( is_host(), pHost ) ) );
		delete pHost;
		m_nCookie++;
	}

	ASSERT( m_Hosts.size() == m_HostsTime.size() );
}


//////////////////////////////////////////////////////////////////////
// CHostCacheList serialize

void CHostCacheList::Serialize(CArchive& ar, int nVersion)
{
	CQuickLock oLock( m_pSection );

	if ( ar.IsStoring() )
	{
		ar.WriteCount( GetCount() );
		for ( CHostCacheMapItr i = m_Hosts.begin() ; i != m_Hosts.end() ; ++i )
		{
			CHostCacheHostPtr pHost = (*i).second;
			pHost->Serialize( ar, nVersion );
		}
	}
	else // Loading
	{
		DWORD_PTR nCount = ar.ReadCount();
		for ( DWORD_PTR nItem = 0 ; nItem < nCount ; nItem++ )
		{
			CHostCacheHostPtr pHost = new CHostCacheHost( m_nProtocol );
			if ( pHost )
			{
				pHost->Serialize( ar, nVersion );
				if ( ! Security.IsDenied( &pHost->m_pAddress ) &&
					 ! Find( &pHost->m_pAddress ) &&
					 ! Find( pHost->m_sAddress ) )
				{
					m_Hosts.insert( CHostCacheMapPair( pHost->m_pAddress, pHost ) );
					m_HostsTime.insert( pHost );
				}
				else
				{
					// Remove bad or duplicated host
					delete pHost;
				}
			}
		}

		PruneHosts();

		m_nCookie++;
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCache root import

int CHostCache::Import(LPCTSTR pszFile, BOOL bFreshOnly)
{
	const LPCTSTR szExt = PathFindExtension( pszFile );

	// Ignore old files (120 days)
	if ( bFreshOnly && ! IsFileNewerThan( pszFile, 120ull * 24 * 60 * 60 * 1000 ) )
		return 0;

	CFile pFile;
	if ( ! pFile.Open( pszFile, CFile::modeRead | CFile::shareDenyWrite | CFile::osSequentialScan ) )
		return 0;

	int nImported = 0;

	if ( ! _tcsicmp( szExt, _T(".met") ) )
	{
		theApp.Message( MSG_NOTICE, _T("Importing MET file: %s"), pszFile );

		nImported = ImportMET( &pFile );
	}
	else if ( ! _tcsicmp( szExt, _T(".bz2") ) )		// hublist.xml.bz2
	{
		theApp.Message( MSG_NOTICE, _T("Importing HubList file: %s"), pszFile );

		nImported = ImportHubList( &pFile );
	}
	else if ( ! _tcsicmp( szExt, _T(".dat") ) )
	{
		theApp.Message( MSG_NOTICE, _T("Importing Nodes file: %s"), pszFile );

		nImported = ImportNodes( &pFile );
	}
//	else if ( ! _tcsicmp( szExt, _T(".xml") ) || ! _tcsicmp( szExt, _T(".dat") ) ) 	// ToDo: G2/Gnutella import/export
//	{
//		theApp.Message( MSG_NOTICE, _T("Importing cache file: %s"), pszFile );
//
//		nImported = ImportCache( &pFile );
//	}


	Save();

	return nImported;
}

//int CHostCache::ImportCache(CFile* pFile)
//{
//	// ToDo: Import/Export G2/Gnutella .xml/.dat
//}

int CHostCache::ImportHubList(CFile* pFile)
{
	const DWORD nSize = pFile->GetLength();

	CBuffer pBuffer;
	if ( ! pBuffer.EnsureBuffer( nSize ) )
		return 0;	// Out of memory

	if ( pFile->Read( pBuffer.GetData(), nSize ) != nSize )
		return 0;	// File error
	pBuffer.m_nLength = nSize;

	if ( ! pBuffer.UnBZip() )
		return 0;	// Decompression error

	CString strEncoding;
	auto_ptr< CXMLElement > pHublist ( CXMLElement::FromString(
		pBuffer.ReadString( pBuffer.m_nLength ), TRUE, &strEncoding ) );
	if ( strEncoding.CompareNoCase( _T("utf-8") ) == 0 )	// Reload as UTF-8
		pHublist.reset( CXMLElement::FromString(
			pBuffer.ReadString( pBuffer.m_nLength, CP_UTF8 ), TRUE ) );
	if ( ! pHublist.get() )
		return FALSE;	// XML decoding error

	if ( ! pHublist->IsNamed( _T("Hublist") ) )
		return FALSE;	// Invalid XML file format

	CXMLElement* pHubs = pHublist->GetFirstElement();
	if ( !  pHubs || ! pHubs->IsNamed( _T("Hubs") ) )
		return FALSE;	// Invalid XML file format

	int nHubs = 0;
	for ( POSITION pos = pHubs->GetElementIterator() ; pos ; )
	{
		CXMLElement* pHub = pHubs->GetNextElement( pos );
		if ( pHub->IsNamed( _T("Hub") ) )
		{
			CString sAddress = pHub->GetAttributeValue( _T("Address") );
			if ( _tcsnicmp( sAddress, _T("dchub://"), 8 ) == 0 )
				sAddress = sAddress.Mid( 8 );
			else if ( _tcsnicmp( sAddress, _T("adc://"), 6 ) == 0 )
				continue;	// Skip ADC-hubs
			else if ( _tcsnicmp( sAddress, _T("adcs://"), 7 ) == 0 )
				continue;	// Skip ADCS-hubs

			const int nUsers	= _tstoi( pHub->GetAttributeValue( _T("Users") ) );
			const int nMaxusers	= _tstoi( pHub->GetAttributeValue( _T("Maxusers") ) );

			CQuickLock oLock( DC.m_pSection );
			CHostCacheHostPtr pServer = DC.Add( NULL, DC_DEFAULT_PORT, 0,
				protocolNames[ PROTOCOL_DC ], 0, nUsers, nMaxusers, sAddress );
			if ( pServer )
			{
				pServer->m_sName = pHub->GetAttributeValue( _T("Name") );
				pServer->m_sDescription = pHub->GetAttributeValue( _T("Description") );
				nHubs++;
			}
		}
	}

	return nHubs;
}

int CHostCache::ImportMET(CFile* pFile)
{
	BYTE nVersion = 0;
	pFile->Read( &nVersion, sizeof(nVersion) );
	if ( nVersion != 0xE0 &&
		 nVersion != ED2K_MET &&
		 nVersion != ED2K_MET_I64TAGS ) return 0;

	int nServers = 0;
	DWORD nCount = 0;

	pFile->Read( &nCount, sizeof(nCount) );

	while ( nCount-- > 0 )
	{
		IN_ADDR pAddress;
		WORD nPort;
		DWORD nTags;

		if ( pFile->Read( &pAddress, sizeof(pAddress) ) != sizeof(pAddress) ) break;
		if ( pFile->Read( &nPort, sizeof(nPort) ) != sizeof(nPort) ) break;
		if ( pFile->Read( &nTags, sizeof(nTags) ) != sizeof(nTags) ) break;

		CQuickLock oLock( eDonkey.m_pSection );
		CHostCacheHostPtr pServer = eDonkey.Add( &pAddress, nPort );

		while ( nTags-- > 0 )
		{
			CEDTag pTag;
			if ( ! pTag.Read( pFile ) ) break;
			if ( pServer == NULL ) continue;

			if ( pTag.Check( ED2K_ST_SERVERNAME, ED2K_TAG_STRING ) )
				pServer->m_sName = pTag.m_sValue;
			else if ( pTag.Check( ED2K_ST_DESCRIPTION, ED2K_TAG_STRING ) )
				pServer->m_sDescription = pTag.m_sValue;
			else if ( pTag.Check( ED2K_ST_MAXUSERS, ED2K_TAG_INT ) )
				pServer->m_nUserLimit = (DWORD)pTag.m_nValue;
			else if ( pTag.Check( ED2K_ST_MAXFILES, ED2K_TAG_INT ) )
				pServer->m_nFileLimit = (DWORD)pTag.m_nValue;
			else if ( pTag.Check( ED2K_ST_UDPFLAGS, ED2K_TAG_INT ) )
				pServer->m_nUDPFlags = (DWORD)pTag.m_nValue;
		}

		nServers++;
	}

	return nServers;
}

int CHostCache::ImportNodes(CFile* pFile)
{
	int nServers = 0;
	DWORD nVersion = 0;

	DWORD nCount;
	if ( pFile->Read( &nCount, sizeof( nCount ) ) != sizeof( nCount ) )
		return 0;
	if ( nCount == 0 )
	{
		// New format
		if ( pFile->Read( &nVersion, sizeof( nVersion ) ) != sizeof( nVersion ) )
			return 0;
		if ( nVersion == 1 )
		{
			if ( pFile->Read( &nCount, sizeof( nCount ) ) != sizeof( nCount ) )
				return 0;
		}
		else
		{
			// Unknown format
			return 0;
		}
	}
	while ( nCount-- > 0 )
	{
		Hashes::Guid oGUID;
		if ( pFile->Read( &oGUID[0], oGUID.byteCount ) != oGUID.byteCount )
			break;
		oGUID.validate();
		IN_ADDR pAddress;
		if ( pFile->Read( &pAddress, sizeof( pAddress ) ) != sizeof( pAddress ) )
			break;
		pAddress.s_addr = ntohl( pAddress.s_addr );
		WORD nUDPPort;
		if ( pFile->Read( &nUDPPort, sizeof( nUDPPort ) ) != sizeof( nUDPPort ) )
			break;
		WORD nTCPPort;
		if ( pFile->Read( &nTCPPort, sizeof( nTCPPort ) ) != sizeof( nTCPPort ) )
			break;
		BYTE nKADVersion = 0;
		BYTE nType = 0;
		if ( nVersion == 1 )
		{
			if ( pFile->Read( &nKADVersion, sizeof( nKADVersion ) ) != sizeof( nKADVersion ) )
				break;
		}
		else
		{
			if ( pFile->Read( &nType, sizeof( nType ) ) != sizeof( nType ) )
				break;
		}
		if ( nType < 4 )
		{
			CQuickLock oLock( Kademlia.m_pSection );
			CHostCacheHostPtr pCache = Kademlia.Add( &pAddress, nTCPPort );
			if ( pCache )
			{
				pCache->m_oGUID = oGUID;
				pCache->m_sDescription = oGUID.toString();
				pCache->m_nUDPPort = nUDPPort;
				pCache->m_nKADVersion = nKADVersion;
				nServers++;
			}
		}
	}

	return nServers;
}

//////////////////////////////////////////////////////////////////////
// CHostCache Check Minimum Servers

bool CHostCache::CheckMinimumServers()
{
	if ( Settings.Experimental.LAN_Mode )
		return true;

	if ( ! EnoughServers() )
	{
		// Load default ed2k server list (if necessary)
		LoadDefaultServers();

		// Get the server list from eMule (mods) if possible
		CString strPrograms( theApp.GetProgramFilesFolder() );
		Import( strPrograms + _T("\\eMule\\config\\server.met"), TRUE );
		Import( strPrograms + _T("\\Neo Mule\\config\\server.met"), TRUE );
		Import( strPrograms + _T("\\hebMule\\config\\server.met"), TRUE );

		CString strAppData( theApp.GetAppDataFolder() );
		Import( strAppData + _T("\\aMule\\server.met"), TRUE );

		// Get server list from Web
		if ( ! EnoughServers() )
			DiscoveryServices.Execute( TRUE, PROTOCOL_ED2K, TRUE );

		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////
// CHostCache Default servers import

int CHostCache::LoadDefaultServers()
{
	const CString strFile = Settings.General.Path + _T("\\Data\\DefaultServers.dat");
	int nServers = 0;

	// Ignore old files (240 days)
	//if ( ! IsFileNewerThan( strFile, 240ull * 24 * 60 * 60 * 1000 ) )
	//	return 0;

	CFile pFile;	// Load default list from file if possible
	if ( ! pFile.Open( strFile, CFile::modeRead | CFile::shareDenyWrite | CFile::osSequentialScan ) )
		return 0;

	theApp.Message( MSG_NOTICE, _T("Loading default server list") );

	try
	{
		CString strLine;
		CBuffer pBuffer;
		TCHAR cType;

		pBuffer.EnsureBuffer( (DWORD)pFile.GetLength() );
		pBuffer.m_nLength = (DWORD)pFile.GetLength();
		pFile.Read( pBuffer.m_pBuffer, pBuffer.m_nLength );
		pFile.Close();

		// Format: P 255.255.255.255:1024	# NameForConvenience

		while ( pBuffer.ReadLine( strLine ) )
		{
			if ( strLine.GetLength() < 16 ) continue;		// Blank or invalid line

			cType = strLine.GetAt( 0 );
			if ( cType == '#' || cType == 'X' ) continue;	// Comment line - ToDo: Handle bad IPs

			CString strServer = strLine.Mid( 2 );			// Remove leading 2 spaces

			if ( strServer.Find( _T("\t"), 14) > 1 )		// Trim at whitespace (remove any comments)
				strServer.Left( strServer.Find( _T("\t"), 14) );
			else if ( strServer.Find( _T(" "), 14) > 1 )
				strServer.Left( strServer.Find( _T(" "), 14) );

			int nIP[4], nPort;

			if ( _stscanf( strServer, _T("%i.%i.%i.%i:%i"), &nIP[0], &nIP[1], &nIP[2], &nIP[3],	&nPort ) == 5 )
			{
				IN_ADDR pAddress;
				pAddress.S_un.S_un_b.s_b1 = (BYTE)nIP[0];
				pAddress.S_un.S_un_b.s_b2 = (BYTE)nIP[1];
				pAddress.S_un.S_un_b.s_b3 = (BYTE)nIP[2];
				pAddress.S_un.S_un_b.s_b4 = (BYTE)nIP[3];

				if ( CHostCacheHostPtr pServer = eDonkey.Add( &pAddress, (WORD)nPort ) )
				{
					pServer->m_bPriority = ( cType == 'P' );
					nServers++;
				}
			}
		}
	}
	catch ( CException* pException )
	{
		if ( pFile.m_hFile != CFile::hFileNull )
			pFile.Close();	// File is still open so close it
		pException->Delete();
	}

	return nServers;
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost construction

CHostCacheHost::CHostCacheHost(PROTOCOLID nProtocol)
	: m_nProtocol	( nProtocol )
	, m_nPort		( 0 )
	, m_nUDPPort	( 0 )
	, m_pVendor 	( NULL )
	, m_bPriority	( FALSE )
	, m_nUserCount	( 0 )
	, m_nUserLimit	( 0 )
	, m_nFileLimit	( 0 )
	, m_nTCPFlags	( 0 )
	, m_nUDPFlags	( 0 )
	, m_tAdded		( GetTickCount() )
	, m_tSeen		( 0 )
	, m_tRetryAfter	( 0 )
	, m_tConnect	( 0 )
	, m_tQuery		( 0 )
	, m_tAck		( 0 )
	, m_tStats		( 0 )
	, m_tFailure	( 0 )
	, m_nFailures	( 0 )
	, m_nDailyUptime( 0 )
	, m_tKeyTime	( 0 )
	, m_nKeyValue	( 0 )
	, m_nKeyHost	( 0 )
	, m_bCheckedLocally ( FALSE )
	, m_bDHT		( FALSE )	// Attributes: DHT
	, m_nKADVersion	( 0 )		// Attributes: Kademlia
{
	m_pAddress.s_addr = INADDR_ANY;

	// 20sec cooldown to avoid neighbor add-remove oscillation
	const DWORD tNow = static_cast< DWORD >( time( NULL ) );
	switch ( m_nProtocol )
	{
	case PROTOCOL_G1:
	case PROTOCOL_G2:
		m_tConnect = tNow - Settings.Gnutella.ConnectThrottle + 20;
		break;
	case PROTOCOL_ED2K:
		m_tConnect = tNow - Settings.eDonkey.QueryThrottle + 20;
		break;
	default:
		break;
	}
}

DWORD CHostCacheHost::Seen() const
{
	return m_tSeen;
}

CString CHostCacheHost::Address() const
{
	if ( m_pAddress.s_addr != INADDR_ANY )
		return CString( inet_ntoa( m_pAddress ) );

	return m_sAddress;
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost serialize

void CHostCacheHost::Serialize(CArchive& ar, int /*nVersion*/)	// HOSTCACHE_SER_VER
{
	if ( ar.IsStoring() )
	{
		ar.Write( &m_pAddress, sizeof(m_pAddress) );
		ar << m_nPort;

		ar << m_tAdded;
		ar << m_tSeen;
		ar << m_tRetryAfter;

		if ( m_pVendor != NULL && m_pVendor->m_sCode.GetLength() == 4 )
		{
			ar << (CHAR)m_pVendor->m_sCode.GetAt( 0 );
			ar << (CHAR)m_pVendor->m_sCode.GetAt( 1 );
			ar << (CHAR)m_pVendor->m_sCode.GetAt( 2 );
			ar << (CHAR)m_pVendor->m_sCode.GetAt( 3 );
		}
		else
		{
			CHAR cZero = 0;
			ar << cZero;
		}

		ar << m_sName;
		if ( ! m_sName.IsEmpty() )
			ar << m_sDescription;

		ar << m_nUserCount;
		ar << m_nUserLimit;
		ar << m_bPriority;

		ar << m_nFileLimit;
		ar << m_nTCPFlags;
		ar << m_nUDPFlags;
		ar << m_tStats;

		ar << m_nKeyValue;
		if ( m_nKeyValue != 0 )
		{
			ar << m_tKeyTime;
			ar << m_nKeyHost;
		}

		ar << m_tFailure;
		ar << m_nFailures;
		ar << m_bCheckedLocally;
		ar << m_nDailyUptime;
		ar << m_sCountry;

		ar << m_bDHT;
		ar.Write( &m_oBtGUID[0], m_oBtGUID.byteCount );

		ar << m_nUDPPort;
		ar.Write( &m_oGUID[0], m_oGUID.byteCount );
		ar << m_nKADVersion;

		ar << m_tConnect;

		ar << m_sUser;
		ar << m_sPass;

		ar << m_sAddress;
	}
	else // Loading
	{
		const DWORD tNow = static_cast< DWORD >( time( NULL ) );

		ReadArchive( ar, &m_pAddress, sizeof(m_pAddress) );
		ar >> m_nPort;

		ar >> m_tAdded;

		ar >> m_tSeen;
		if ( m_tSeen > tNow )
			m_tSeen = tNow;

		ar >> m_tRetryAfter;

		CHAR szaVendor[4] = { 0, 0, 0, 0 };
		ar >> szaVendor[0];

		if ( szaVendor[0] )
		{
			ReadArchive( ar, szaVendor + 1, 3 );
			TCHAR szVendor[5] = { szaVendor[0], szaVendor[1], szaVendor[2], szaVendor[3], 0 };
			m_pVendor = VendorCache.Lookup( szVendor );
		}

		//if ( nVersion >= 10 )
		//{
			ar >> m_sName;
			if ( ! m_sName.IsEmpty() )
				ar >> m_sDescription;

			ar >> m_nUserCount;
			ar >> m_nUserLimit;
			ar >> m_bPriority;

			ar >> m_nFileLimit;
			ar >> m_nTCPFlags;
			ar >> m_nUDPFlags;
			ar >> m_tStats;
		//}
		//else if ( nVersion >= 7 )
		//{
		//	ar >> m_sName;
		//	if ( ! m_sName.IsEmpty() )
		//	{
		//		ar >> m_sDescription;
		//		ar >> m_nUserCount;
		//		if ( nVersion >= 8 )
		//			ar >> m_nUserLimit;
		//		if ( nVersion >= 9 )
		//			ar >> m_bPriority;
		//		if ( nVersion >= 10 )
		//		{
		//			ar >> m_nFileLimit;
		//			ar >> m_nTCPFlags;
		//			ar >> m_nUDPFlags;
		//		}
		//	}
		//}

		ar >> m_nKeyValue;
		if ( m_nKeyValue != 0 )
		{
			ar >> m_tKeyTime;
			ar >> m_nKeyHost;
		}

		//if ( nVersion >= 11 )
		//{
			ar >> m_tFailure;
			ar >> m_nFailures;
		//}

		//if ( nVersion >= 12 )
		//{
			ar >> m_bCheckedLocally;
			ar >> m_nDailyUptime;
		//}

		//if ( nVersion >= 14 )
			ar >> m_sCountry;
		//else
		//	m_sCountry = theApp.GetCountryCode( m_pAddress );

		//if ( nVersion >= 15 )
		//{
			ar >> m_bDHT;
			ReadArchive( ar, &m_oBtGUID[0], m_oBtGUID.byteCount );
			m_oBtGUID.validate();
		//}

		//if ( nVersion >= 16 )
		//{
			ar >> m_nUDPPort;
			ReadArchive( ar, &m_oGUID[0], m_oGUID.byteCount );
			m_oGUID.validate();
			ar >> m_nKADVersion;
		//}

		//if ( nVersion >= 17 )
			ar >> m_tConnect;

		//if ( nVersion >= 18 )
		//{
			ar >> m_sUser;
			ar >> m_sPass;
		//}

		//if ( nVersion >= 19 )
			ar >> m_sAddress;
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost update

bool CHostCacheHost::Update(WORD nPort, DWORD tSeen, LPCTSTR pszVendor, DWORD nUptime, DWORD nCurrentLeaves, DWORD nLeafLimit)
{
	bool bChanged = FALSE;

	if ( nPort )
		m_nUDPPort = m_nPort = nPort;

	const DWORD tNow = static_cast< DWORD >( time( NULL ) );
	if ( ! tSeen || tSeen > tNow )
		tSeen = tNow;

	if ( m_tSeen < tSeen )
	{
		m_tSeen = tSeen;
		bChanged = true;
	}

	if ( nUptime )
		m_nDailyUptime = nUptime;

	if ( nCurrentLeaves )
		m_nUserCount = nCurrentLeaves;

	if ( nLeafLimit )
		m_nUserLimit = nLeafLimit;

	if ( pszVendor != NULL )
	{
		CString strVendorCode( pszVendor );
		strVendorCode.Trim();
		if ( ( m_pVendor == NULL || m_pVendor->m_sCode != strVendorCode ) && ! strVendorCode.IsEmpty() )
			m_pVendor = VendorCache.Lookup( (LPCTSTR)strVendorCode );
	}

	if ( m_sCountry.IsEmpty() )
		m_sCountry = theApp.GetCountryCode( m_pAddress );

	return bChanged;
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost connection setup

bool CHostCacheHost::ConnectTo(BOOL bAutomatic)
{
	m_tConnect = static_cast< DWORD >( time( NULL ) );

	switch( m_nProtocol )
	{
	case PROTOCOL_G1:
	case PROTOCOL_G2:
	case PROTOCOL_ED2K:
	case PROTOCOL_DC:
		if ( m_pAddress.s_addr == INADDR_ANY )
		{
			m_tConnect += 30;	// Throttle for 30 seconds
			return Network.ConnectTo( m_sAddress, m_nPort, m_nProtocol, FALSE ) != FALSE;
		}
		return Neighbours.ConnectTo( m_pAddress, m_nPort, m_nProtocol, bAutomatic ) != NULL;
	case PROTOCOL_KAD:
		{
			SOCKADDR_IN pHost = {};
			pHost.sin_family = AF_INET;
			pHost.sin_addr.s_addr = m_pAddress.s_addr;
			pHost.sin_port = htons( m_nUDPPort );
			Kademlia.Bootstrap( &pHost );
		}
		break;
	default:
		ASSERT( FALSE );
	}

	return false;
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost string

CString CHostCacheHost::ToString(bool bLong) const
{
	CString str;
	if ( bLong )
	{
		time_t tSeen = m_tSeen;
		tm time = {};
		if ( gmtime_s( &time, &tSeen ) == 0 )
		{
			str.Format( _T("%s:%i %.4i-%.2i-%.2iT%.2i:%.2iZ"),
				(LPCTSTR)CString( inet_ntoa( m_pAddress ) ), m_nPort,
				time.tm_year + 1900, time.tm_mon + 1, time.tm_mday,
				time.tm_hour, time.tm_min );	// 2002-04-30T08:30Z

			return str;
		}
	}

	str.Format( _T("%s:%i"),
		(LPCTSTR)CString( inet_ntoa( m_pAddress ) ), m_nPort );

	return str;
}

bool CHostCacheHost::IsExpired(const DWORD tNow) const
{
	switch ( m_nProtocol )
	{
	case PROTOCOL_G1:
		return m_tSeen && ( tNow > m_tSeen + Settings.Gnutella1.HostExpire );
	case PROTOCOL_G2:
		return m_tSeen && ( tNow > m_tSeen + Settings.Gnutella2.HostExpire );
	case PROTOCOL_BT:
		return m_tSeen && ( tNow > m_tSeen + Settings.BitTorrent.HostExpire );
	case PROTOCOL_KAD:
		return m_tSeen && ( tNow > m_tSeen + 24 * 60 * 60 );	// ToDo: Add Kademlia setting
	default:	// Never PROTOCOL_ED2K PROTOCOL_DC
		return false;
	}
}

bool CHostCacheHost::IsThrottled(const DWORD tNow) const
{
	// Don't overload network name resolver
	if ( m_pAddress.s_addr == INADDR_ANY && Network.GetResolveCount() > 3 )
		return true;

	if ( Settings.Connection.ConnectThrottle &&			// 0 default, ~250ms if limited
		 tNow - m_tConnect < Settings.Connection.ConnectThrottle / 1000 )
		return true;

	switch ( m_nProtocol )
	{
	case PROTOCOL_G1:
	case PROTOCOL_G2:
		return ( tNow < m_tConnect + Settings.Gnutella.ConnectThrottle );
	case PROTOCOL_ED2K:
		return ( tNow < m_tConnect + Settings.eDonkey.QueryThrottle );
	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost connection test

bool CHostCacheHost::CanConnect(const DWORD tNow) const
{
	// Don't connect to self
	if ( Settings.Connection.IgnoreOwnIP && Network.IsSelfIP( m_pAddress ) ) return false;

	return
		( ! m_tFailure ||											// Let failed host rest some time
		( tNow > m_tFailure + Settings.Connection.FailurePenalty ) ) &&
		( m_nFailures <= Settings.Connection.FailureLimit ) &&		// and we haven't lost hope on this host
		( m_bPriority || ! IsExpired( tNow ) ) &&					// and host isn't expired
		( ! IsThrottled( tNow ) );									// and don't reconnect too fast
																	// now we can connect.
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost quote test

bool CHostCacheHost::CanQuote(const DWORD tNow) const
{
	// If a host isn't dead and isn't expired, we can tell others about it
	return ( m_nFailures == 0 ) && ( ! IsExpired( tNow ) );
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost query test

// Can we UDP query this host? (G2/ED2K/KAD/BT)
bool CHostCacheHost::CanQuery(const DWORD tNow) const
{
	// Retry After check, for all
	if ( m_tRetryAfter != 0 && tNow < m_tRetryAfter ) return false;

	switch ( m_nProtocol )
	{
	case PROTOCOL_G2:
		// Must support G2
		if ( ! Network.IsConnected() || ! Settings.Gnutella2.Enabled ) return false;

		// Must not be waiting for an ack
		if ( m_tAck == 0 ) return false;

		// Must be a recently seen (current) host
		if ( tNow > m_tSeen + Settings.Gnutella2.HostCurrent ) return false;

		// If haven't queried yet, its ok
		if ( m_tQuery == 0 ) return true;

		// Don't query too fast
		return tNow > m_tQuery + Settings.Gnutella2.QueryThrottle;

	case PROTOCOL_ED2K:
		// Must support ED2K
		if ( ! Network.IsConnected() || ! Settings.eDonkey.Enabled ) return false;
		if ( ! Settings.eDonkey.ServerWalk ) return false;

		// If haven't queried yet, its ok
		if ( m_tQuery == 0 ) return true;

		// Don't query too fast
		return tNow > m_tQuery + Settings.eDonkey.QueryThrottle;

	case PROTOCOL_BT:
		// Must support BT
		if ( ! Settings.BitTorrent.Enabled ) return false;

		// If haven't queried yet, its ok
		if ( m_tQuery == 0 ) return true;

		// Don't query too fast
		return tNow > m_tQuery + 90u;

	case PROTOCOL_DC:
		if ( ! Network.IsConnected() ) return false;
		return true;

	case PROTOCOL_KAD:
		if ( ! Network.IsConnected() ) return false;

		// If haven't queried yet, its ok
		//if ( m_tQuery == 0 ) return true;
		return true;	// ToDo: Support KAD

	default:
		return false;
	}
}

//////////////////////////////////////////////////////////////////////
// CHostCacheHost query key submission

void CHostCacheHost::SetKey(const DWORD nKey, const IN_ADDR* pHost)
{
	if ( ! nKey ) return;

	m_tAck		= 0;
	m_nFailures	= 0;
	m_tFailure	= 0;
	m_nKeyValue	= nKey;
	m_tKeyTime	= static_cast< DWORD >( time( NULL ) );
	m_nKeyHost	= pHost ? pHost->S_un.S_addr : Network.m_pHost.sin_addr.S_un.S_addr;
}
