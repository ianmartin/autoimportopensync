/***********************************************************************
KCalendar OSyncDataSource class
Copyright (C) 2004 Conectiva S. A.
 
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation;
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 
ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
SOFTWARE IS DISCLAIMED.
*************************************************************************/
/**
 * @author Eduardo Pereira Habkost <ehabkost@conectiva.com.br>
 * @author Andrew Baumann <andrewb@cse.unsw.edu.au>
 */

#include <libkcal/calendarresources.h>
#include <libkcal/incidence.h>
#include <libkcal/icalformat.h>
#include <libkcal/calendarlocal.h>
#include <kdeversion.h>

#include "osyncbase.h"
#include "datasource.h"

class KCalSharedResource
{
	private:
		KCal::CalendarResources *calendar;
		int refcount;
	
		bool report_incidence(OSyncDataSource *dsobj, OSyncPluginInfo *info, OSyncContext *ctx, KCal::Incidence *e, OSyncObjFormat *objformat);
	
	public:
		KCalSharedResource() {calendar = NULL; refcount = 0;};
		bool open(OSyncContext *ctx);
		bool close(OSyncContext *ctx);
		bool get_event_changes(OSyncDataSource *dsobj, OSyncPluginInfo *info, OSyncContext *ctx);
		bool get_todo_changes(OSyncDataSource *dsobj, OSyncPluginInfo *info, OSyncContext *ctx);
		bool commit(OSyncContext *ctx, OSyncChange *chg);
};

class KCalEventDataSource : public OSyncDataSource
{
	private:
		KCalSharedResource *kcal;
	
	public:
		KCalEventDataSource(KCalSharedResource *kcal) : OSyncDataSource("event"), kcal(kcal) {};
		virtual ~KCalEventDataSource() {};

		bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);
		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void disconnect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void get_changes(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void commit(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg);
};

class KCalTodoDataSource : public OSyncDataSource
{
	private:
		KCalSharedResource *kcal;
	
	public:
		KCalTodoDataSource(KCalSharedResource *kcal) : OSyncDataSource("todo"), kcal(kcal) {};
		virtual ~KCalTodoDataSource() {};

		bool initialize(OSyncPlugin *plugin, OSyncPluginInfo *info, OSyncError **error);
		virtual void connect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void disconnect(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void get_changes(OSyncPluginInfo *info, OSyncContext *ctx);
		virtual void commit(OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *chg);
};
