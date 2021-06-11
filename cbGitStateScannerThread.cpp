#include "cbGitStateScannerThread.h"
#include "cbGitStates.h"
#include <cppgit2/repository.hpp>
//#include <sdk.h>

#include <cbproject.h>
using namespace cppgit2;

wxDEFINE_EVENT( wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

cbGitStateScannerThread::cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj) :
    wxThread(wxTHREAD_DETACHED),
    prj_(prj),
    m_pHandler(pHandler)
    {}

void *cbGitStateScannerThread::Entry()
{
    wxString wxpath = prj_->GetBasePath();
    std::string path = wxpath.ToStdString();
    auto repo = repository::open(path);

    cbGitFileState statusofFile;

    status::options optionobj;
    optionobj.set_flags(status::options::flag::include_unmodified | status::options::flag::include_untracked | status::options::flag::include_ignored | status::options::flag::recurse_ignored_dirs | status::options::flag::recurse_untracked_dirs  );

    std::map <std::string, cbGitFileState>*  prjmap ;
    prjmap = new std::map <std::string, cbGitFileState>;

    repo.for_each_status( optionobj,
        [&statusofFile, &prjmap](const std::string &path, status::status_type status_flags)
        {
            statusofFile.state = cbGitFileState::unmodified;
            if ((status_flags & status::status_type::index_deleted) == status::status_type::index_deleted)
                statusofFile.state = cbGitFileState::deleted;

            if ((status_flags & status::status_type::wt_deleted) == status::status_type::wt_deleted)
                statusofFile.state = cbGitFileState::deleted;

            if ((status_flags & status::status_type::index_new) == status::status_type::index_new)
                statusofFile.state = cbGitFileState::newfile;

            if ((status_flags & status::status_type::wt_new) == status::status_type::wt_new)
                statusofFile.state = cbGitFileState::newfile;

            if ((status_flags & status::status_type::index_modified) == status::status_type::index_modified)
                statusofFile.state = cbGitFileState::modified;

            if ((status_flags & status::status_type::wt_modified) == status::status_type::wt_modified)
                statusofFile.state = cbGitFileState::modified;

            if ((status_flags & status::status_type::ignored) == status::status_type::ignored)
            statusofFile.state = cbGitFileState::ignored;

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
