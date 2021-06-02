#include "cbGitStateScannerThread.h"
//#include <sdk.h>

#include <cbproject.h>

//DEFINE_EVENT_TYPE(wxEVT_SCANNER_THREAD_UPDATE)
//wxDEFINE_EVENT(wxEVT_SCANNER_7THREAD_UPDATE, wxCommandEvent);

wxDEFINE_EVENT( wxEVT_SCANNER_7THREAD_UPDATE, wxCommandEvent);

cbGitStateScannerThread::cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj) :
    wxThread(wxTHREAD_DETACHED),
    prj_(prj),
    m_pHandler(pHandler)

    {}

void *cbGitStateScannerThread::Entry()
{

    testString = "Hallo";
    wxCommandEvent *evt = new wxCommandEvent(wxEVT_SCANNER_7THREAD_UPDATE, GetId());
    evt->SetClientData(new std::string(testString));
    wxQueueEvent(m_pHandler, evt);



    return 0;
}

cbGitStateScannerThread::~cbGitStateScannerThread()
{
}
