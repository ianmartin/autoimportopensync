#include "KNotesIface.h"
#include "KNotesIface_stub.h"
#include <stdio.h>
#include <qmap.h>
#include <qtimer.h>

#include <kapplication.h>

#include <dcopclient.h>
#include <qstring.h>
#include <qstringlist.h>

typedef QString KNoteID_t;

bool isRunning(const QCString &n)
{
        DCOPClient *dcop = KApplication::kApplication()->dcopClient();
        
        if (!dcop->attach())
        	printf("nix is\n");
        QCStringList apps = dcop->registeredApplications();
        
        QStringList lst;
        printf("asd %i, %s", lst.count(), (const char*)lst.join("\n").local8Bit());
        return apps.contains(n);
}


int main(void)
{
	printf("test \n");
	
	QMap <KNoteID_t,QString> fNotes;
	

	/*DCOPClient *fdp = new DCOPClient;
	fdp = KApplication::kApplication()->dcopClient();

	if (!fdp) {
		printf("No DCOP connection could be made. The conduit cannot function without DCOP.\n");
		return 1;
	}*/

	QCString knotesAppname = "knotes" ;
	if (!isRunning(knotesAppname)) {
		knotesAppname = "kontact" ;
		if (!isRunning(knotesAppname)) {
			printf("KNotes is not running.\n");
			return 1;
		}
	}

	
	
        fNotes = fKNotes->notes();
       if (fKNotes->status() != DCOPStub::CallSucceeded)
        {
             printf("Could not retrieve list of notes from KNotes.\n");
        return 1;

}

        QMap<KNoteID_t,QString>::ConstIterator i = fNotes.begin();
        while (i != fNotes.end())
        {
	printf("%s -> %s %s\n", (const char*)i.key().local8Bit(), (const char*)i.data().local8Bit(), (fKNotes->isNew("opensync",i.key())) ? " (new)" : "");
                i++;
        }
        
	QString id = fKNotes->newNote("test1", "test2");
	if (fKNotes->status() != DCOPStub::CallSucceeded) {
		printf("Could not add note to KNotes.\n");
		return 1;
	}
	
	fKNotes->hideNote(id);
	if (fKNotes->status() != DCOPStub::CallSucceeded) {
		printf("Could not hide note to KNotes.\n");
		return 1;
	}
	
	return 0;
}
