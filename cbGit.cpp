#include <stdlib.h>
#include <sdk.h> // Code::Blocks SDK
#include <configurationpanel.h>
#include "cbGit.h"
#include <cbproject.h>
#include <logmanager.h>
#include <editormanager.h>
#include <cbeditor.h>

#include "cbGitStateScannerThread.h"

#include <wx/dynlib.h>
#include <wx/dir.h>
#include <git2.h>

// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<cbGit> reg(_T("cbGit"));
    auto ID_REVERT         = wxNewId();
    auto ID_REVERT_TO_HEAD = wxNewId();
    auto ID_DIFF           = wxNewId();
    auto ID_DIFF_TO_HEAD   = wxNewId();
    auto ID_FORCEUPDATE    = wxNewId();
}

// events handling
BEGIN_EVENT_TABLE(cbGit, cbPlugin)
    EVT_COMMAND(wxID_ANY, wxEVT_SCANNER_THREAD_UPDATE, cbGit::OnStateScannerThread)
    EVT_MENU(ID_REVERT,                                cbGit::OnRevert)
    EVT_MENU(ID_REVERT_TO_HEAD,                        cbGit::OnRevert)
    EVT_MENU(ID_DIFF,                                  cbGit::OnDiff)
    EVT_MENU(ID_DIFF_TO_HEAD,                          cbGit::OnDiff)
    EVT_MENU(ID_FORCEUPDATE,                           cbGit::StartUpdateThread)
END_EVENT_TABLE()

cbGit::cbGit()
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if(!Manager::LoadResource(_T("cbGit.zip")))
    {
        NotifyMissingFile(_T("cbGit.zip"));
    }
}

void cbGit::OnAttach()
{
    // do whatever initialization you need for your plugin
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be TRUE...
    // You should check for it in other functions, because if it
    // is FALSE, it means that the application did *not* "load"
    // (see: does not need) this plugin...

    Manager* man = Manager::Get();

    if (git_libgit2_init() < 0)
    {
        man->GetLogManager()->Log("unable to load libgit2!");
        return;
    }

    man->RegisterEventSink(cbEVT_PROJECT_NEW, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectOpen));
    man->RegisterEventSink(cbEVT_PROJECT_CLOSE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectClose));
    man->RegisterEventSink(cbEVT_PROJECT_ACTIVATE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectOpen));
    man->RegisterEventSink(cbEVT_PROJECT_FILE_ADDED, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectFileAdded));
    man->RegisterEventSink(cbEVT_PROJECT_FILE_REMOVED, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectFileRemoved));
    man->RegisterEventSink(cbEVT_EDITOR_SAVE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnFileSaveOrClose));
    man->RegisterEventSink(cbEVT_EDITOR_UPDATE_UI, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnFileSaveOrClose)); //May causes Problems when closing IDE?
}

void cbGit::OnRelease(bool appShutDown)
{
    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...
    Manager::Get()->RemoveAllEventSinksFor(this);

    git_libgit2_shutdown();

    for (auto it : mapOfOpenPrj_)
        delete it.second;
    mapOfOpenPrj_.clear();
}

void cbGit::BuildMenu(wxMenuBar* menuBar)
{
    int fileMenuIndex = menuBar->FindMenu( _("&File") );
    if (fileMenuIndex == wxNOT_FOUND )
        return;

    wxMenu* fileMenu = menuBar->GetMenu(fileMenuIndex);
    if (!fileMenu)
        return;
    //The application is offering its menubar for your plugin,
    //to add any menu items you want...
    //Append any items you need in the menu...
    //NOTE: Be careful in here... The application's menubar is at your disposal.
//    NotImplemented(_T("cbGit::BuildMenu()"));
}

void cbGit::BuildModuleMenu(const ModuleType type, wxMenu* menu, const FileTreeData* data)
{
    //Some library module is ready to display a pop-up menu.
    //Check the parameter \"type\" and see which module it is
    //and append any items you need in the menu...
    //TIP: for consistency, add a separator as the first item...
//    NotImplemented(_T("cbGit::BuildModuleMenu()"));
    if (type == mtProjectManager && data != 0 && data->GetKind() == FileTreeData::ftdkFile)
    {
        if (ProjectFile *prjFile = data->GetProjectFile() )
            if (cbProject *prj = prjFile->GetParentProject() )
                appendMenu(prj, prjFile, menu);
    }
    else if ((type == mtEditorManager || type == mtEditorTab) && Manager::Get()->GetEditorManager() && Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor())
    {
        if (Manager::Get()->GetProjectManager())
        {
            wxString filename = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor()->GetFilename();
            ProjectFile *prjFile;
            if (cbProject *prj = Manager::Get()->GetProjectManager()->FindProjectForFile(filename, &prjFile, false, false))
                appendMenu(prj, prjFile, menu);
        }
    }
}

void cbGit::appendMenu(cbProject *prj, ProjectFile *prjFile, wxMenu *menu)
{
    wxMenu *gitMenu = new wxMenu();
    forCommandSelectedFileName_ = prjFile->file.GetFullPath().ToStdString();
    forCommandSelectedProject_ = prj;

    if (mapOfOpenPrj_t::iterator iter = mapOfOpenPrj_.find(forCommandSelectedProject_); iter != mapOfOpenPrj_.end())
    {
        filestatemap_t* filestates = iter->second;
        if (filestatemap_t::iterator it = filestates->find(forCommandSelectedFileName_); it != filestates->end())
        {
            const cbGitFileState &stateOfFile = it->second;
            if (stateOfFile.state == cbGitFileState::modified)
            {
                EditorBase *eb = Manager::Get()->GetEditorManager()->GetEditor(prjFile->file.GetFullPath());
                if((eb && !eb->GetModified()) || !eb)
                    gitMenu->Append(ID_REVERT, "Revert", "revert to base");
                gitMenu->Append(ID_DIFF, "Diff", "diff selected file with head");
            }
        }
    }

    gitMenu->Append(ID_FORCEUPDATE, "ForceUpdate", "Force UpdateThread for Testing");
    menu->AppendSubMenu(gitMenu, "GIT");
}

bool cbGit::BuildToolBar(wxToolBar* toolBar)
{
    //The application is offering its toolbar for your plugin,
    //to add any toolbar items you want...
    //Append any items you need on the toolbar...
    NotImplemented(_T("cbGit::BuildToolBar() "));

    // return true if you add toolbar items
    return false;
}

void cbGit::OnProjectClose(CodeBlocksEvent& event)
{
    Manager::Get()->GetLogManager()->Log("cbGit::OnProjectClose");
    cbProject *prj = event.GetProject();
    if (!prj)
        return;

    if (mapOfOpenPrj_t::iterator it = mapOfOpenPrj_.find(prj); it != mapOfOpenPrj_.end())
    {
        delete it->second;
        mapOfOpenPrj_.erase(prj);
    }
}


void cbGit::StartUpdateThread(wxCommandEvent& event)
{
    UpdateThread(forCommandSelectedProject_);
}

void cbGit::OnProjectOpen(CodeBlocksEvent& event)
{
    UpdateThread(event.GetProject());
}

void cbGit::OnProjectFileAdded(CodeBlocksEvent& event)
{
    UpdateThread(event.GetProject());
}

void cbGit::OnProjectFileRemoved(CodeBlocksEvent& event)
{
    UpdateThread(event.GetProject());
}

void cbGit::OnFileSaveOrClose(CodeBlocksEvent& event)
{
    UpdateThread(event.GetProject());
}

void cbGit::OnRevert(wxCommandEvent& event)
{
    const bool toHead = event.GetId() == ID_REVERT_TO_HEAD;
    wxFileName fname(forCommandSelectedFileName_);
    git_repository *repo = NULL;

    /* Open repository, walking up from given directory to find root */
    int error = git_repository_open_ext(&repo, fname.GetPath(false).c_str(), 0, NULL);
    if (error < 0)
    {
        Manager::Get()->GetLogManager()->LogError("unable to open repo");
        return;
    }

    wxString repoPath = git_repository_path(repo);
    const wxString gitFolderName = ".git/";
    if (repoPath.EndsWith(gitFolderName))
        repoPath = repoPath.Mid(0, repoPath.length() - gitFolderName.length());
    fname.MakeRelativeTo(repoPath);

    git_checkout_options options = GIT_CHECKOUT_OPTIONS_INIT;
    options.checkout_strategy = GIT_CHECKOUT_FORCE;

    wxString fnameFullPath = fname.GetFullPath();
    const char *pathsarr[] = { fnameFullPath.c_str() };
    options.paths.strings = (char**)pathsarr;
    options.paths.count = 1;

    if (toHead)
    {
        error = git_checkout_head(repo, &options);
    }
    else
    {
        git_index *index = NULL;
        error = git_repository_index(&index, repo);
        git_index_free(index);
        if (error >= 0)
            error = git_checkout_index(repo, index, &options);
    }
    git_repository_free(repo);
    if (error < 0)
    {
        Manager::Get()->GetLogManager()->LogError("unable to checkout");
        const git_error *e = git_error_last();
        wxString str; str.Printf("Error %d/%d: %s\n", error, e->klass, e->message);
        Manager::Get()->GetLogManager()->LogError(str);
        return;
    }

    cbEditor *ed = Manager::Get()->GetEditorManager()->GetBuiltinEditor(wxString(forCommandSelectedFileName_));
    if(ed)
    {
        if (ed->GetModified())
            Manager::Get()->GetEditorManager()->CheckForExternallyModifiedFiles();
        else
            ed->Reload();
    }

    UpdateThread(forCommandSelectedProject_);
}

void cbGit::OnDiff(wxCommandEvent& event)
{
    const bool toHead = event.GetId() == ID_DIFF_TO_HEAD;

    if (!forCommandSelectedProject_->SaveAllFiles())
        return;

    int error;
    wxFileName fname(forCommandSelectedFileName_);
    git_repository *repo = NULL;

    /* Open repository, walking up from given directory to find root */
    error = git_repository_open_ext(&repo, fname.GetPath(false).c_str(), 0, NULL);
    if (error < 0)
    {
        Manager::Get()->GetLogManager()->LogError("unable to open repo");
        return;
    }

    wxString repoPath = git_repository_path(repo);
    if (repoPath.EndsWith(".git/"))
        repoPath = repoPath.Mid(0, repoPath.length()-5);
    fname.MakeRelativeTo(repoPath);

    git_checkout_options options = GIT_CHECKOUT_OPTIONS_INIT;
    options.checkout_strategy = GIT_CHECKOUT_FORCE;

    wxString fnameFullPath = fname.GetFullPath();
    const char *pathsarr[] = { fnameFullPath.c_str() };
    options.paths.strings = (char**)pathsarr;
    options.paths.count = 1;

    wxString checkout_path = repoPath + "diff";
    if (wxFile::Exists(forCommandSelectedFileName_ + ".head"))
    {
        Manager::Get()->GetLogManager()->LogError("file " + forCommandSelectedFileName_ + ".head already exists");
        return;
    }
    if (!wxFileName::Mkdir(checkout_path))
    {
        Manager::Get()->GetLogManager()->LogError("Could not create Folder" + repoPath + "diff");
        return;
    }
    options.target_directory = checkout_path;
    if (toHead)
    {
        error = git_checkout_head(repo, &options);
    }
    else
    {
        git_index *index = NULL;
        error = git_repository_index(&index, repo);
        git_index_free(index);
        if (error >= 0)
            error = git_checkout_index(repo, index, &options);
    }
    git_repository_free(repo);
    if (error < 0)
    {
        Manager::Get()->GetLogManager()->LogError("unable to checkout");
        return;
    }
    wxString checkoutfile = wxString(forCommandSelectedFileName_).AfterLast('/');
    wxRenameFile(checkout_path + "/" + checkoutfile, forCommandSelectedFileName_ + ".head");
    if (!wxFileName::Rmdir(checkout_path))
    {
        Manager::Get()->GetLogManager()->LogError("couldn't delelete temp folder " + checkout_path );
        return;
    }

    if (Manager::Get()->GetPluginManager()->FindPluginByName(_T("cbDiff")) != NULL)
    {
        PluginElement* element = Manager::Get()->GetPluginManager()->FindElementByName(_T("cbDiff"));
        // is library loaded
        if (element->library->IsLoaded())
        {
            typedef void (*cbDiffFunc) (const wxString&, const wxString&, int viewmode);
            if (cbDiffFunc difffunc = (cbDiffFunc)element->library->GetSymbol(_("DiffFiles")))
                difffunc(forCommandSelectedFileName_, forCommandSelectedFileName_ + ".head", -1);
        }
	}
    wxRemoveFile(forCommandSelectedFileName_ + ".head");

//////    std::string str = "meld "+forCommandSelectedFileName+" "+forCommandSelectedFileName+".head" +" &"; // running it in background with "&" works on commandline but here it opens a third File
//////    const char *command = str.c_str();                                                                 // So meld has to be closed to use Codeblocks again
//////    wxArrayString output;
//////    wxArrayString errors;
//////    wxExecute (command, output, errors);
}


void cbGit::OnStateScannerThread(wxCommandEvent& event)
{
    typedef cbGitStateScannerThread::prjmap_t statemap_t;

    cbGitStateScannerThread::return_t *statemap = (cbGitStateScannerThread::return_t*)event.GetClientData();
    cbProject *prj = statemap->first;
    statemap_t *map_name_state = statemap->second;
    delete statemap;

    if (!Manager::Get()->GetProjectManager()->IsProjectStillOpen(prj))
    {
        if (mapOfOpenPrj_t::iterator it = mapOfOpenPrj_.find(prj); it != mapOfOpenPrj_.end())
        {
            delete it->second;
            mapOfOpenPrj_.erase(prj);
        }
        delete map_name_state;
        return;
    }

    mapOfOpenPrj_[prj] = map_name_state;
    for (statemap_t::iterator it = map_name_state->begin(); it != map_name_state->end(); ++it)
    {
        const cbGitFileState &statusOfFile = it->second;
        std::string filepath = it->first;
        wxString wxfilepath(filepath);

        if (ProjectFile *prjfile = prj->GetFileByFilename(wxfilepath, false, true))
        {
            switch (statusOfFile.state)
            {
            case cbGitFileState::unmodified : prjfile->SetFileState(fvsVcUpToDate);      break;
            case cbGitFileState::modified   : prjfile->SetFileState(fvsVcModified);      break;
            case cbGitFileState::newfile    : prjfile->SetFileState(fvsVcNonControlled); break;
            case cbGitFileState::deleted    : prjfile->SetFileState(fvsVcMissing);       break;
            case cbGitFileState::ignored    : prjfile->SetFileState(fvsVcNonControlled); break;
            }
        }
    }
}

void cbGit::UpdateThread()
{
    ProjectManager* manager = ProjectManager::Get();
    if (!manager)
        return;

    cbProject* project = manager->GetActiveProject();
    if (!project)
        return;

    UpdateThread(project);
}

void cbGit::UpdateThread(cbProject *project)
{
    if (project)
    {
        cbGitStateScannerThread *thread = new cbGitStateScannerThread(this, project);
        thread->Create();
        thread->Run();
    }
}

