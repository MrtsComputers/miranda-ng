/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (C) 2012-14 Miranda NG project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "..\..\core\commonheaders.h"
#include "database.h"

static int stringCompare(const char* p1, const char* p2)
{
	return strcmp(p1, p2);
}

static int compareGlobals(const DBCachedGlobalValue* p1, const DBCachedGlobalValue* p2)
{
	return strcmp(p1->name, p2->name);
}

MDatabaseCache::MDatabaseCache() :
	m_lSettings(100, stringCompare),
	m_lContacts(50, HandleKeySortT),
	m_lGlobalSettings(50, compareGlobals)
{
	m_hCacheHeap = HeapCreate(0, 0, 0);
	InitializeCriticalSection(&m_cs);
}

MDatabaseCache::~MDatabaseCache()
{
	HeapDestroy(m_hCacheHeap);
	DeleteCriticalSection(&m_cs);
}

/////////////////////////////////////////////////////////////////////////////////////////

DBCachedContact* MDatabaseCache::AddContactToCache(HANDLE hContact)
{
	mir_cslock lck(m_cs);

	int index = m_lContacts.getIndex((DBCachedContact*)&hContact);
	if (index == -1) {
		DBCachedContact* VL = (DBCachedContact*)HeapAlloc(m_hCacheHeap, HEAP_ZERO_MEMORY, sizeof(DBCachedContact));
		VL->hContact = hContact;
		m_lContacts.insert(VL);
		return VL;
	}

	return m_lContacts[ index ];
}

DBCachedContact* MDatabaseCache::GetCachedContact(HANDLE hContact)
{
	mir_cslock lck(m_cs);

	int index = m_lContacts.getIndex((DBCachedContact*)&hContact);
	return (index == -1) ? NULL : m_lContacts[index];
}

void MDatabaseCache::FreeCachedContact(HANDLE hContact)
{
	mir_cslock lck(m_cs);

	int index = m_lContacts.getIndex((DBCachedContact*)&hContact);
	if (index == -1)
		return;

	DBCachedContact* VL = m_lContacts[index];
	DBCachedContactValue* V = VL->first;
	while (V != NULL) {
		DBCachedContactValue* V1 = V->next;
		FreeCachedVariant(&V->value);
		HeapFree( m_hCacheHeap, 0, V );
		V = V1;
	}
	HeapFree( m_hCacheHeap, 0, VL );

	m_lContacts.remove(index);

	for (int i=0; i < m_lContacts.getCount(); i++) {
		DBCachedContact *cc = m_lContacts[i];
		if (cc->hNext == hContact)
			cc->hNext = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

char* MDatabaseCache::InsertCachedSetting(const char* szName, int cbLen)
{
	char* newValue = (char*)HeapAlloc(m_hCacheHeap, 0, cbLen);
	*newValue++ = 0;
	strcpy(newValue, szName);
	m_lSettings.insert(newValue);
	return newValue;
}

char* MDatabaseCache::GetCachedSetting(const char *szModuleName,const char *szSettingName, int moduleNameLen, int settingNameLen)
{
	char szFullName[512];
	const char *szKey;
	if (szModuleName != NULL) {
		strcpy(szFullName, szModuleName);
		szFullName[moduleNameLen] = '/';
		strcpy(szFullName+moduleNameLen+1,szSettingName);
		szKey = szFullName;
	}
	else szKey = szSettingName;

	if (m_lastSetting && !strcmp(szKey, m_lastSetting))
		return m_lastSetting;

	int index = m_lSettings.getIndex((char*)szKey);
	if (index != -1)
		m_lastSetting = m_lSettings[index];
	else
		m_lastSetting = InsertCachedSetting(szKey, settingNameLen+moduleNameLen+3);

	return m_lastSetting;
}

void MDatabaseCache::SetCachedVariant(DBVARIANT* s /* new */, DBVARIANT* d /* cached */ )
{
	char* szSave = ( d->type == DBVT_UTF8 || d->type == DBVT_ASCIIZ ) ? d->pszVal : NULL;

	memcpy( d, s, sizeof( DBVARIANT ));
	if (( s->type == DBVT_UTF8 || s->type == DBVT_ASCIIZ ) && s->pszVal != NULL ) {
		if ( szSave != NULL )
			d->pszVal = (char*)HeapReAlloc(m_hCacheHeap,0,szSave,strlen(s->pszVal)+1);
		else
			d->pszVal = (char*)HeapAlloc(m_hCacheHeap,0,strlen(s->pszVal)+1);
		strcpy(d->pszVal,s->pszVal);
	}
	else if ( szSave != NULL ) {
		HeapFree(m_hCacheHeap,0,szSave);
		d->pszVal = NULL;
	}
}

void MDatabaseCache::FreeCachedVariant(DBVARIANT* V)
{
	if (( V->type == DBVT_ASCIIZ || V->type == DBVT_UTF8 ) && V->pszVal != NULL )
		HeapFree(m_hCacheHeap,0,V->pszVal);
}

STDMETHODIMP_(DBVARIANT*) MDatabaseCache::GetCachedValuePtr(HANDLE hContact, char *szSetting, int bAllocate)
{
	// a global setting
	if ( hContact == 0 ) {
		DBCachedGlobalValue Vtemp, *V;
		Vtemp.name = szSetting;
		int index = m_lGlobalSettings.getIndex(&Vtemp);
		if (index != -1) {
			V = m_lGlobalSettings[index];
			if ( bAllocate == -1 ) {
				FreeCachedVariant( &V->value );
				m_lGlobalSettings.remove(index);
				HeapFree(m_hCacheHeap,0,V);
				return NULL;
			}
		}
		else {
			if ( bAllocate != 1 )
				return NULL;

			V = (DBCachedGlobalValue*)HeapAlloc(m_hCacheHeap,HEAP_ZERO_MEMORY,sizeof(DBCachedGlobalValue));
			V->name = szSetting;
			m_lGlobalSettings.insert(V);
		}

		return &V->value;
	}

	// a contact setting
	DBCachedContactValue *V, *V1;
	DBCachedContact VLtemp,*VL;

	VLtemp.hContact = hContact;

	int index = m_lContacts.getIndex(&VLtemp);
	if (index == -1) {
		if ( bAllocate != 1 )
			return NULL;

		VL = AddContactToCache(hContact);
	}
	else VL = m_lContacts[index];

	m_lastVL = VL;

	for ( V = VL->first; V != NULL; V = V->next)
		if (V->name == szSetting)
			break;

	if ( V == NULL ) {
		if ( bAllocate != 1 )
			return NULL;

		V = (DBCachedContactValue *)HeapAlloc(m_hCacheHeap, HEAP_ZERO_MEMORY, sizeof(DBCachedContactValue));
		if (VL->last)
			VL->last->next = V;
		else
			VL->first = V;
		VL->last = V;
		V->name = szSetting;
	}
	else if ( bAllocate == -1 ) {
		m_lastVL = NULL;
		FreeCachedVariant(&V->value);
		if ( VL->first == V ) {
			VL->first = V->next;
			if (VL->last == V)
				VL->last = V->next; // NULL
		}
		else
			for ( V1 = VL->first; V1 != NULL; V1 = V1->next )
				if ( V1->next == V ) {
					V1->next = V->next;
					if (VL->last == V)
						VL->last = V1;
					break;
				}
		HeapFree(m_hCacheHeap,0,V);
		return NULL;
	}

	return &V->value;
}
