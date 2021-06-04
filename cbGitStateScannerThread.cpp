#include "cbGitStateScannerThread.h"
#include "cbGitStates.h"
#include <cppgit2/repository.hpp>
//#include <sdk.h>

#include <cbproject.h>
using namespace cppgit2;

//DEFINE_EVENT_TYPE(wxEVT_SCANNER_THREAD_UPDATE)
//wxDEFINE_EVENT(wxEVT_SCANNER_7THREAD_UPDATE, wxCommandEvent);

wxDEFINE_EVENT( wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

cbGitStateScannerThread::cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj) :
    wxThread(wxTHREAD_DETACHED),
    prj_(prj),
    statemap_(nullptr),
    m_pHandler(pHandler)
    {}

void *cbGitStateScannerThread::Entry()
{

    testString = "";
    auto path = "/home/marx/TestCppGit/awesome_cpp";
    auto repo = repository::open(path);

    cbGitFileState statusofFile;
    FilesList prjfiles = prj_->GetFilesList();
//    for(int i = 0; i < prjfiles.size() ; i++)
//    {
////       path = prjfiles(i)->GetBaseName();
////    wxFileName name = prjfiles.
//
////    path = prjfiles.begin().;
//
//
//    }
    status::status_type singelFile;
//    for (FilesList::iterator it = prjfiles.begin(); it != prjfiles.end(); it++)
//    {
//        statusofFile.state = cbGitFileState::unmodified;
//        ProjectFile* pf = *it;
//        path          = pf->file.GetFullPath();
//        singelFile    = repo.status_file("/home/marx/BuildCB/trunk/src/plugins/contrib/cbGit/cbGit.cpp");
//        if(singelFile == status::status_type::wt_modified)
//            statusofFile.state = cbGitFileState::modified;
//        prjmap_[path] = statusofFile;
//    }

    status::options optionobj;
    optionobj.set_flags(status::options::flag::include_unmodified | status::options::flag::include_untracked );

    std::map <std::string, cbGitFileState>*  prjmap ;
    prjmap = new std::map <std::string, cbGitFileState>;

    statemap_ = new std::map <std::string, cbGitFileState>;

    repo.for_each_status( optionobj,
        [&statusofFile, &prjmap](const std::string &path, status::status_type status_flags)
        {
            statusofFile.state = cbGitFileState::unmodified;
            if ((status_flags & status::status_type::index_deleted) == status::status_type::index_deleted)
                statusofFile.state = cbGitFileState::deleted;

            if ((status_flags & status::status_type::wt_deleted) == status::status_type::wt_deleted)
                statusofFile.state = cbGitFileState::deleted;

            if ((status_flags & status::status_type::index_new) == status::status_type::index_new);
                statusofFile.state = cbGitFileState::fnew;

            if ((status_flags & status::status_type::wt_new) == status::status_type::wt_new)
                statusofFile.state = cbGitFileState::fnew;

            if ((status_flags & status::status_type::index_modified) == status::status_type::index_modified)
                statusofFile.state = cbGitFileState::modified;

            if ((status_flags & status::status_type::wt_modified) == status::status_type::wt_modified)
                statusofFile.state = cbGitFileState::modified;

            if ((status_flags & status::status_type::wt_modified) == status::status_type::wt_modified)
                statusofFile.state = cbGitFileState::modified;

//             prjmap[path] =  statusofFile;
             (*prjmap)[path] =  statusofFile;

        });

    wxCommandEvent *evt = new wxCommandEvent(wxEVT_SCANNER_THREAD_UPDATE, GetId());

    evt->SetClientData(new std::pair<cbProject*, prjmap_t*> {prj_, prjmap});
    wxQueueEvent(m_pHandler, evt);
    return 0;
}

cbGitStateScannerThread::~cbGitStateScannerThread()
{
}
