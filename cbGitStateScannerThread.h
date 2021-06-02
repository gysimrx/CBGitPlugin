#ifndef CBGITSTATESCANNERTHREAD_H
#define CBGITSTATESCANNERTHREAD_H

#include <wx/thread.h>
#include <wx/event.h>
#include <sdk.h>

//#include <cbproject.h>

class cbProject;
//???????????????????????????????????????????????????????????????
//BEGIN_DECLARE_EVENT_TYPES()
//    DECLARE_EVENT_TYPE(wxEVT_SCANNER_THREAD_UPDATE, wxID_ANY)
//END_DECLARE_EVENT_TYPES()

//wxDECLARE_EVENT(MY_NEW_TYPE, wxCommandEvent);
wxDECLARE_EVENT( wxEVT_SCANNER_7THREAD_UPDATE, wxCommandEvent);

class cbGitStateScannerThread : public wxThread
{
    private:
        std::string testString ;

    public:
        cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj);
//        cbGitStateScannerThread(wxEvtHandler* pHandler);
        virtual ~cbGitStateScannerThread();

//        typedef std::pair< cbProject*,  string_t> prjstr_t;
    private:
        cbProject *prj_;
        void* Entry();

    protected:
        wxEvtHandler* m_pHandler;
};



#endif // CBGITSTATESCANNERTHREAD_CPP_INCLUDED
