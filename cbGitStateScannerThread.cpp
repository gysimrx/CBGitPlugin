#include "cbGitStateScannerThread.h"
#include "cbGitStates.h"
#include <cppgit2/repository.hpp>
//#include <sdk.h>
#include <iostream>
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


    auto prjmap = new std::map <std::string, cbGitFileState>;
    GetFileStates(repo, prjmap);
    auto submoduleURLs = new std::vector<std::string>;

    repo.for_each_submodule(
        [&submoduleURLs](const submodule &module, const std::string &path) {
            submoduleURLs->push_back(module.url());
        });
    while( !submoduleURLs->empty())
    {
        auto repo2 = repository::open(submoduleURLs->back());

        GetFileStates(repo2, prjmap);
        submoduleURLs->pop_back();
    }




    wxCommandEvent *evt = new wxCommandEvent(wxEVT_SCANNER_THREAD_UPDATE, GetId());

    evt->SetClientData(new std::pair<cbProject*, prjmap_t*> {prj_, prjmap});
    wxQueueEvent(m_pHandler, evt);
    return 0;
}

void cbGitStateScannerThread::GetFileStates(cppgit2::repository &repo, std::map <std::string, cbGitFileState>*  prjmap )
{
    wxString wxpath = prj_->GetBasePath();
    std::string prjpath = wxpath.ToStdString();
    wxpath = repo.path(repository::item::workdir);
    std::string repopath = wxpath.ToStdString();
    std::string additonalpath;
    if(prjpath != repopath)
        additonalpath = repopath.replace(repopath.find(prjpath), prjpath.size() , "");

    status::options optionobj;
    optionobj.set_flags(status::options::flag::include_unmodified | status::options::flag::include_untracked | status::options::flag::include_ignored | status::options::flag::recurse_ignored_dirs | status::options::flag::recurse_untracked_dirs  );
    repo.for_each_status( optionobj,
        [&prjmap, &additonalpath](const std::string &path, status::status_type status_flags)
        {

            cbGitFileState statusofFile;
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

            (*prjmap)[additonalpath+path] =  statusofFile; // Submodules need nonrelative paths

        });


}

cbGitStateScannerThread::~cbGitStateScannerThread()
{
}
