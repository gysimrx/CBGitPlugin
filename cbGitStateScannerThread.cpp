#include "cbGitStateScannerThread.h"
#include "cbGitStates.h"
//#include <sdk.h>
#include <iostream>
#include <cbproject.h>
#include <fstream>

wxDEFINE_EVENT(wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

cbGitStateScannerThread::cbGitStateScannerThread(wxEvtHandler *pHandler, cbProject *prj):
    wxThread(wxTHREAD_DETACHED),
    prj_(prj),
    m_pHandler(pHandler)
{
    prjPath_ = prj_->GetBasePath().ToStdString();

    for (int idx = 0 ; idx < prj_->GetFilesCount() ; ++idx)
    {
        ProjectFile *prjFile = prj_->GetFile(idx);
        files_.push_back(prjFile->file.GetFullPath().ToStdString());
    }
}

void *cbGitStateScannerThread::Entry()
{
    auto prjmap = new prjmap_t;

    for (auto file: files_)
    {
        wxFileName fname(file.c_str());
        git_repository *repo = NULL;
        /* Open repository, walking up from given directory to find root */
        int error = git_repository_open_ext(&repo, fname.GetPath(false).c_str(), 0, NULL);
        if (error < 0)
            continue;

        wxString repoPath = git_repository_path(repo);
        if (repoPath.EndsWith(".git/"))
            repoPath = repoPath.Mid(0, repoPath.length()-5);
        fname.MakeRelativeTo(repoPath);

        unsigned int statusFlags;
        error = git_status_file(&statusFlags, repo, fname.GetFullPath().c_str());
        git_repository_free(repo);
        if (error < 0)
            continue;

        cbGitFileState statusOfFile;
        if (statusFlags & (GIT_STATUS_INDEX_MODIFIED | GIT_STATUS_WT_MODIFIED))
            statusOfFile.state = cbGitFileState::modified;

        if (statusFlags & (GIT_STATUS_INDEX_DELETED | GIT_STATUS_WT_DELETED))
            statusOfFile.state = cbGitFileState::deleted;

        if (statusFlags & (GIT_STATUS_INDEX_NEW | GIT_STATUS_WT_NEW))
            statusOfFile.state = cbGitFileState::newfile;

        if (statusFlags & (GIT_STATUS_INDEX_RENAMED | GIT_STATUS_WT_RENAMED))
            statusOfFile.state = cbGitFileState::renamed;

        if (statusFlags & GIT_STATUS_IGNORED)
            statusOfFile.state = cbGitFileState::ignored;

        if (statusFlags & GIT_STATUS_CONFLICTED)
            statusOfFile.state = cbGitFileState::conflicted;

        (*prjmap)[file] = statusOfFile;
    }

    if (!prjmap->size())
    {
        delete prjmap;
        return 0;
    }

    wxCommandEvent *evt = new wxCommandEvent(wxEVT_SCANNER_THREAD_UPDATE, GetId());
    evt->SetClientData(new return_t{prj_, prjmap});
    wxQueueEvent(m_pHandler, evt);
    return 0;
}

