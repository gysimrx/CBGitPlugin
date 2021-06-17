#ifndef CBGITSTATESCANNERTHREAD_H
#define CBGITSTATESCANNERTHREAD_H

#include <wx/thread.h>
#include <wx/event.h>
#include <sdk.h>
#include <map>

#include <cppgit2/repository.hpp>
#include <cbproject.h>
#include "cbGitStates.h"

class cbProject;

wxDECLARE_EVENT( wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

class cbGitStateScannerThread : public wxThread
{
    private:
        std::map <std::string, cbGitFileState> prjmap_ ;
        typedef std::map <std::string, cbGitFileState> prjmap_t ;
    public:
        cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj);
        typedef std::pair< cbProject*, prjmap_t*> return_t;
        virtual ~cbGitStateScannerThread();
    private:
        void GetFileStates(cppgit2::repository &repo, std::map <std::string, cbGitFileState>*  prjmap );
        cbProject *prj_;
        void* Entry();

    protected:
        wxEvtHandler* m_pHandler;
};



#endif // CBGITSTATESCANNERTHREAD_CPP_INCLUDED
