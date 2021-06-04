#ifndef CBGITSTATESCANNERTHREAD_H
#define CBGITSTATESCANNERTHREAD_H

#include <wx/thread.h>
#include <wx/event.h>
#include <sdk.h>
#include <map>

#include <cbproject.h>
#include "cbGitStates.h"

class cbProject;

wxDECLARE_EVENT( wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

class cbGitStateScannerThread : public wxThread
{
    private:
        std::string testString ;
        std::map <std::string, cbGitFileState> prjmap_ ;
        typedef std::map <std::string, cbGitFileState> prjmap_t ;
    public:
        cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj);
        typedef std::pair< cbProject*, prjmap_t*> return_t;
        virtual ~cbGitStateScannerThread();
    private:
        cbProject *prj_;
        prjmap_t *statemap_;
        void* Entry();

    protected:
        wxEvtHandler* m_pHandler;
};



#endif // CBGITSTATESCANNERTHREAD_CPP_INCLUDED
