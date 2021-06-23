#include <sdk.h> // Code::Blocks SDK
#include <configurationpanel.h>
#include "cbGit.h"
#include <cbproject.h>
#include <editormanager.h>
#include <cbeditor.h>

#include "cbGitStateScannerThread.h"

#include <wx/dynlib.h>
#include <logmanager.h>
#include <wx/dir.h>


#include <cppgit2/repository.hpp>
using namespace cppgit2;

// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<cbGit> reg(_T("cbGit"));
        auto ID_REVERT            = wxNewId();
        auto ID_FORCEUPDATE       = wxNewId();
}

// events handling
BEGIN_EVENT_TABLE(cbGit, cbPlugin)
    EVT_COMMAND(wxID_ANY, wxEVT_SCANNER_THREAD_UPDATE, cbGit::OnStateScannerThread)
    EVT_MENU(ID_REVERT,     cbGit::OnRevert)
    EVT_MENU(ID_FORCEUPDATE,     cbGit::StartUpdateThread)

    // add any events you want to handle here
END_EVENT_TABLE()

// constructor
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

// destructor
cbGit::~cbGit()
{
}

void cbGit::OnAttach()
{
    // do whatever initialization you need for your plugin
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be TRUE...
    // You should check for it in other functions, because if it
    // is FALSE, it means that the application did *not* "load"
    // (see: does not need) this plugin...
    Manager* pm = Manager::Get();

    pm->RegisterEventSink(cbEVT_PROJECT_NEW, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectOpen));
    pm->RegisterEventSink(cbEVT_PROJECT_CLOSE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectClose));
    pm->RegisterEventSink(cbEVT_PROJECT_ACTIVATE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectOpen));
    pm->RegisterEventSink(cbEVT_PROJECT_FILE_ADDED, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectFileAdded));
    pm->RegisterEventSink(cbEVT_PROJECT_FILE_REMOVED, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectFileRemoved));
    pm->RegisterEventSink(cbEVT_EDITOR_SAVE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnFileSaveOrClose));
//    pm->RegisterEventSink(cbEVT_EDITOR_CLOSE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnFileSaveOrClose)); //May causes Problems when closing IDE?



}

void cbGit::OnRelease(bool appShutDown)
{
    // do de-initialization for your plugin
    // if appShutDown is true, the plugin is unloaded because Code::Blocks is being shut down,
    // which means you must not use any of the SDK Managers
    // NOTE: after this function, the inherited member variable
    // m_IsAttached will be FALSE...
    Manager::Get()->RemoveAllEventSinksFor(this);
}

void cbGit::BuildMenu(wxMenuBar* menuBar)
{
    int fileMenuIndex = menuBar->FindMenu( _("&File") );
    if(fileMenuIndex == wxNOT_FOUND )
        return;

    wxMenu* fileMenu = menuBar->GetMenu(fileMenuIndex);
    if(!fileMenu)
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
    if ( type == mtProjectManager && data != 0 && data->GetKind() == FileTreeData::ftdkFile )
    {
        if( ProjectFile *prjFile = data->GetProjectFile() )
            if ( cbProject *prj = prjFile->GetParentProject() )
                appendMenu(prj, prjFile, menu);
    }
    else if ( (type == mtEditorManager || type == mtEditorTab) && Manager::Get()->GetEditorManager() && Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor() )
    {
        if ( Manager::Get()->GetProjectManager() )
        {
            wxString filename = Manager::Get()->GetEditorManager()->GetBuiltinActiveEditor()->GetFilename();
            ProjectFile *prjFile;
            if ( cbProject *prj = Manager::Get()->GetProjectManager()->FindProjectForFile(filename, &prjFile, false, false) )
                appendMenu(prj, prjFile, menu);
        }
    }

}

void cbGit::appendMenu(cbProject *prj, ProjectFile *prjFile, wxMenu *menu)
{

    wxMenu *gitMenu = new wxMenu();
    forCommandSelectedFileName = std::string(prjFile->file.GetFullPath().c_str());
    forCommandSelectedProject = prj;
    gitMenu->Append(ID_REVERT, "Revert", "revert to base");
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

}

void cbGit::OnProjectOpen(CodeBlocksEvent& event)       //Doesn't Work
{
    UpdateThread();
}

void cbGit::StartUpdateThread(wxCommandEvent& event)
{
    UpdateThread(forCommandSelectedProject);
}

void cbGit::OnProjectFileAdded(CodeBlocksEvent& event)
{
    UpdateThread(forCommandSelectedProject);
}

void cbGit::OnProjectFileRemoved(CodeBlocksEvent& event)
{

}
void cbGit::OnFileSaveOrClose(CodeBlocksEvent& event)
{
    cbProject* project = event.GetProject();
    UpdateThread(project);


}

void cbGit::OnRevert(wxCommandEvent& event)
{
    if(!forCommandSelectedProject->SaveAllFiles())
        return;
    std::string str = forCommandSelectedProject->GetBasePath().ToStdString();
    std::size_t found = str.find_last_of("/", str.size()-2);                                                 // exclude slash on end of string from search
                                                                                                             // To Do:
    std::string repopath = repository::discover_path(forCommandSelectedFileName,false,str.substr(0,found));  // git_expception crashes cb when no repo is found

    repopath = repopath.substr(0,repopath.size()-5);
    auto repo = repository::open(repopath);
    if(!repo.is_empty())
    {
        std::string relativefilepath = forCommandSelectedFileName.substr(repopath.size());
        status::status_type singelFile = repo.status_file(relativefilepath);
        if((singelFile == status::status_type::index_modified) || (singelFile == status::status_type::wt_modified) )
        {
            checkout::options checkoutOptions;
            const std::vector<std::string> checkoutpaths{"/" + relativefilepath};
            checkoutOptions.set_strategy(checkout::checkout_strategy::force | checkout::checkout_strategy::disable_pathspec_match);
            repo.checkout_head(checkoutOptions);
            Manager::Get()->GetEditorManager()->CheckForExternallyModifiedFiles();
            if(!forCommandSelectedProject->SaveAllFiles())
                return;
            UpdateThread(forCommandSelectedProject);
        }
    }

}

void cbGit::OnStateScannerThread(wxCommandEvent& event)
{
    typedef std::map<std::string, cbGitFileState> statemap_t ;

    cbGitFileState statusofFile;

    cbGitStateScannerThread::return_t *statemap = (cbGitStateScannerThread::return_t*)event.GetClientData();
    cbProject *prj = statemap->first;
//    wxMessageBox(wxString("Recieved thread for Project")+prj->GetBasePath());

    if(!Manager::Get()->GetProjectManager()->IsProjectStillOpen(prj))
    {
        delete statemap;
        wxMessageBox(wxString("Project ")+prj->GetBasePath()+wxString(" Is not opend"));
        return;
    }

    statemap_t *map_name_state = statemap->second;
    for (statemap_t::iterator it=map_name_state->begin(); it!=map_name_state->end(); ++it)
    {
        statusofFile = it->second;
        std::string  filepath = it->first;
        wxString wxfilepath(filepath);

        if(prj->GetFileByFilename(wxfilepath,false,true) != NULL )
        {
            ProjectFile *prjfile = prj->GetFileByFilename(wxfilepath,false,true);
            switch (statusofFile.state)
            {
                case cbGitFileState::unmodified : prjfile->SetFileState(fvsVcUpToDate);
                                                  break;
                case cbGitFileState::modified   : prjfile->SetFileState(fvsVcModified);
                                                  break;
                case cbGitFileState::newfile    : prjfile->SetFileState(fvsVcNonControlled);
                                                  break;
                case cbGitFileState::deleted    : prjfile->SetFileState(fvsVcMissing);
                                                  break;
                case cbGitFileState::ignored    : prjfile->SetFileState(fvsVcNonControlled);
                                                  break;
            }
        }

    }
    delete statemap;
}
void cbGit::UpdateThread(void)
{
    ProjectManager* manager = ProjectManager::Get();
    cbProject* project = manager->GetActiveProject();
    UpdateThread(project);
}
void cbGit::UpdateThread(cbProject *project)
{

    if(project != NULL)
    {
        cbGitStateScannerThread *thread = new cbGitStateScannerThread( this, project );
        thread->Create();
        thread->Run();
    }
}




