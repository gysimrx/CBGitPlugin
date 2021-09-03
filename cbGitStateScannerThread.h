#ifndef CBGITSTATESCANNERTHREAD_H
#define CBGITSTATESCANNERTHREAD_H

#include <wx/thread.h>
#include <wx/event.h>
#include <sdk.h>
#include <map>
#include <vector>

#include <git2.h>
#include "cbGitStates.h"

class cbProject;

wxDECLARE_EVENT(wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

class cbGitStateScannerThread : public wxThread
{
    public:
        cbGitStateScannerThread(wxEvtHandler *pHandler, cbProject *prj);
        typedef std::map<std::string, cbGitFileState> prjmap_t;
        typedef std::pair<cbProject*, prjmap_t*> return_t;
        virtual ~cbGitStateScannerThread(){}
    private:
        cbProject *prj_;
        void* Entry();
        wxEvtHandler* m_pHandler;
        std::string prjPath_;

        std::vector<std::string> files_;
};

#endif


