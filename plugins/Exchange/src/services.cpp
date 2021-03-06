/*
Exchange notifier plugin for Miranda IM

Copyright � 2006 Cristian Libotean, Attila Vajda

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

#include "services.h"

CRITICAL_SECTION csCheck;

int InitServices()
{
	CreateServiceFunction(MS_EXCHANGE_CHECKEMAIL, (MIRANDASERVICE) CheckEmailService);
	InitializeCriticalSection(&csCheck);
	
	return 0;
}

int DestroyServices()
{
	DestroyServiceFunction(MS_EXCHANGE_CHECKEMAIL);
	DeleteCriticalSection(&csCheck);
	
	return 0;
}

int CheckEmailService(WPARAM wParam, LPARAM lParam)
{
/*
	if (!exchangeServer.IsConnected())
		{
			exchangeServer.Connect(1); //force connection attempt
		}
	return exchangeServer.Check(TRUE); */
	return ThreadCheckEmail(TRUE);
}