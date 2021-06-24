#include "cbGitStateScannerThread.h"
#include "cbGitStates.h"
#include <cppgit2/repository.hpp>
//#include <sdk.h>
#include <iostream>
#include <cbproject.h>
#include <wx/dir.h>
using namespace cppgit2;

wxDEFINE_EVENT( wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

class wxDirTraverserSimple2 : public wxDirTraverser
{
  public:
    wxDirTraverserSimple2(wxArrayString& files) : m_files(files){}
    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        wxString temp = filename.substr(filename.size()-3);

        return wxDIR_CONTINUE;
    }
    virtual wxDirTraverseResult OnDir(const wxString& dirname)
    {
        wxString temp = dirname.substr(dirname.size()-3);
        if(temp == "git")
        {
            temp = dirname.substr(0,dirname.size()-5);
            m_files.Add(temp );

        }
      return wxDIR_CONTINUE;
    }
  private:
    wxArrayString& m_files;
};

cbGitStateScannerThread::cbGitStateScannerThread(wxEvtHandler* pHandler, cbProject *prj) :
    wxThread(wxTHREAD_DETACHED),
    prj_(prj),
    m_pHandler(pHandler)
    {}

void *cbGitStateScannerThread::Entry()
{
    wxString wxpath = prj_->GetBasePath();
    std::string path = wxpath.ToStdString();
    auto prjmap = new std::map <std::string, cbGitFileState>;

    wxArrayString files;
    wxDirTraverserSimple2 traverser(files);
    wxDir dir(path);
    dir.Traverse(traverser);

    while(!files.empty())
    {
        std::string repopath = files.back().ToStdString();
        auto repo2 = repository::open(repopath);
        GetFileStates(repo2, prjmap);
        files.pop_back();
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
        additonalpath = repopath.substr(prjpath.size(),repopath.size());

    status::options optionobj;
    optionobj.set_flags(status::options::flag::include_unmodified | status::options::flag::include_untracked | status::options::flag::include_ignored | status::options::flag::recurse_ignored_dirs | status::options::flag::recurse_untracked_dirs  );
    repo.for_each_status( optionobj,
        [&prjpath, &prjmap, &additonalpath](const std::string &path, status::status_type status_flags)
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

            (*prjmap)[prjpath+additonalpath+path] =  statusofFile; // Subrepos need nonrelative paths

        });

}


cbGitStateScannerThread::~cbGitStateScannerThread()
{
}
