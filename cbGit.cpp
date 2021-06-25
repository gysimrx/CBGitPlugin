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
#include <stdlib.h>

using namespace cppgit2;

// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<cbGit> reg(_T("cbGit"));
        auto ID_REVERT            = wxNewId();
        auto ID_DIFF              = wxNewId();
        auto ID_FORCEUPDATE       = wxNewId();
}

// events handling
BEGIN_EVENT_TABLE(cbGit, cbPlugin)
    EVT_COMMAND(wxID_ANY, wxEVT_SCANNER_THREAD_UPDATE, cbGit::OnStateScannerThread)
    EVT_MENU(ID_REVERT,     cbGit::OnRevert)
    EVT_MENU(ID_DIFF,       cbGit::OnDiff)
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

    if(mapOfOpenPrj.count(forCommandSelectedProject) == 1)
    {
        filestatemap_t* filestates = mapOfOpenPrj[forCommandSelectedProject];
        if(filestates->count(forCommandSelectedFileName) == 1)      // File is versioned
        {
            cbGitFileState stateOfFile = filestates->at(forCommandSelectedFileName);
            if(stateOfFile.state == cbGitFileState::modified)
            {
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
    checkoutFile();

    Manager::Get()->GetEditorManager()->CheckForExternallyModifiedFiles();
    if(!forCommandSelectedProject->SaveAllFiles())
        return;
    UpdateThread(forCommandSelectedProject);


}
void cbGit::OnDiff(wxCommandEvent& event)
{
    if(!forCommandSelectedProject->SaveAllFiles())
        return;

    wxCopyFile(forCommandSelectedFileName, forCommandSelectedFileName+(".temp"),false);
    checkoutFile();
    wxCopyFile(forCommandSelectedFileName, forCommandSelectedFileName+(".head"),false);
    wxCopyFile(forCommandSelectedFileName+(".temp"), forCommandSelectedFileName ,true);
    wxRemoveFile(forCommandSelectedFileName+(".temp"));


    if(Manager::Get()->GetPluginManager()->FindPluginByName(_T("cbDiff")) != NULL)
    {
        PluginElement* element = Manager::Get()->GetPluginManager()->FindElementByName(_T("cbDiff"));
        // is library loaded
        if(element->library->IsLoaded())
        {
            typedef void (*cbDiffFunc) (const wxString&, const wxString&, int viewmode);

            cbDiffFunc difffunc = (cbDiffFunc)element->library->GetSymbol(_("DiffFiles"));
            if(difffunc != NULL)
            {
                difffunc(wxString(forCommandSelectedFileName), wxString(forCommandSelectedFileName+".head"), -1);
            }
        }
	}
	 if(!forCommandSelectedProject->SaveAllFiles())
        return;
/*
     EditorManager::CheckForExternallyModifiedFiles()(); Codeblocks checks for changes when focus is newly set to CodeBlocks
     if there are files like main.cpp and main.h in your Project Tree and you select main.cpp to diff, main.h gets reverted
*/

////    std::string str = "meld "+forCommandSelectedFileName+" "+forCommandSelectedFileName+".head" +" &"; // running it in background with "&" works on commandline but here it opens a third File
////    const char *command = str.c_str();                                                                 // So meld has to be closed to use Codeblocks again
////    wxArrayString output;
////    wxArrayString errors;
////    wxExecute (command, output, errors);

}

void cbGit::checkoutFile (void)
{
    std::string str = forCommandSelectedProject->GetBasePath().ToStdString();
    std::size_t found = str.find_last_of("/", str.size()-2);                                                 // exclude slash on end of string from search

    std::string repopath = repository::discover_path(forCommandSelectedFileName,false,str.substr(0,found));
    repopath = repopath.substr(0,repopath.size()-5);
    auto repo = repository::open(repopath);
    std::string relativefilepath = forCommandSelectedFileName.substr(repopath.size());

    checkout::options checkoutOptions;
    const std::vector<std::string> checkoutpaths{"/" + relativefilepath};
    checkoutOptions.set_strategy(checkout::checkout_strategy::force | checkout::checkout_strategy::disable_pathspec_match);
    repo.checkout_head(checkoutOptions);

}

void cbGit::OnStateScannerThread(wxCommandEvent& event)
{
    typedef std::map<std::string, cbGitFileState> statemap_t ;



    cbGitFileState statusofFile;

    cbGitStateScannerThread::return_t *statemap = (cbGitStateScannerThread::return_t*)event.GetClientData();
    cbProject *prj = statemap->first;
    mapOfOpenPrj[prj] = statemap->second;

//    wxMessageBox(wxString("Recieved thread for Project")+prj->GetBasePath());

    if(!Manager::Get()->GetProjectManager()->IsProjectStillOpen(prj))
    {
        wxMessageBox(wxString("Project ")+prj->GetBasePath()+wxString(" Is not opend"));
        mapOfOpenPrj.erase(prj);
        delete statemap;
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




