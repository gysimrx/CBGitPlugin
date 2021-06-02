#include <sdk.h> // Code::Blocks SDK
#include <configurationpanel.h>
#include "cbGit.h"
#include <cbproject.h>
#include <editormanager.h>
#include <cbeditor.h>

#include "cbGitStateScannerThread.h"

#include <wx/dynlib.h>
#include <logmanager.h>


#include <cppgit2/repository.hpp>
using namespace cppgit2;

// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
    PluginRegistrant<cbGit> reg(_T("cbGit"));
        auto ID_REVERT            = wxNewId();
//        auto ID_UPDATE            = wxNewId();
}
//wxDEFINE_EVENT(wxEVT_SCANNER_THREAD_UPDATE, wxCommandEvent);

// events handling
BEGIN_EVENT_TABLE(cbGit, cbPlugin)
    EVT_COMMAND(wxID_ANY, wxEVT_SCANNER_7THREAD_UPDATE, cbGit::OnStateScannerThread)
//    EVT_COMMAND(wxID_ANY, MY_NEW_TYPE, cbGit::OnStateScannerThread)
    EVT_MENU(ID_REVERT,     cbGit::OnRevert)
//    EVT_MENU(ID_UPDATE,     cbGit::OnUpdate)

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
    pm->RegisterEventSink(cbEVT_PROJECT_OPEN, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectOpen));
    pm->RegisterEventSink(cbEVT_PROJECT_FILE_ADDED, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectFileAdded));
    pm->RegisterEventSink(cbEVT_PROJECT_FILE_REMOVED, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnProjectFileRemoved));
    pm->RegisterEventSink(cbEVT_EDITOR_SAVE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnFileSaveOrClose));
    pm->RegisterEventSink(cbEVT_EDITOR_CLOSE, new cbEventFunctor<cbGit, CodeBlocksEvent>(this, &cbGit::OnFileSaveOrClose));



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
    forCommandSelectedFileNameShort = prjFile;
    forCommandSelectedProject = prj;
    gitMenu->Append(ID_REVERT, "Revert", "revert to base");
//    gitMenu->Append(ID_UPDATE, "Update", "update file in working copy");
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
//    wxMessageBox(wxString("Project Closed "));
}
void cbGit::OnProjectOpen(CodeBlocksEvent& event)
{
//    wxMessageBox(wxString("Project Opend "));
}
void cbGit::OnProjectFileAdded(CodeBlocksEvent& event)
{
//    wxMessageBox(wxString("Project File Added "));
}
void cbGit::OnProjectFileRemoved(CodeBlocksEvent& event)
{
//    wxMessageBox(wxString("Project File removed "));
}
void cbGit::OnFileSaveOrClose(CodeBlocksEvent& event)
{
//    wxMessageBox(wxString("fileSavedOrClosed "));
}

void cbGit::OnRevert(wxCommandEvent& event)
{
    wxMessageBox(wxString("Revert ")+forCommandSelectedFileName);
    ChangeFileStateVisual();

    cbGitStateScannerThread *thread = new cbGitStateScannerThread( this, forCommandSelectedProject );
    thread->Create();
    thread->Run();
}

void cbGit::ChangeFileStateVisual()
{
    if(OnlyForTesting)
        forCommandSelectedFileNameShort->SetFileState(fvsVcModified);
    else
        forCommandSelectedFileNameShort->SetFileState(fvsVcUpToDate);



//
    OnlyForTesting = !OnlyForTesting;
}

class TestThread: public wxThread
{
public:
  void *Entry()
  {
     // do something
//      wxMessageBox(wxString("entry"));
     return 0;
  }
};

void cbGit::OnStateScannerThread(wxCommandEvent& event)
{
    std::string *test;
//    test = (cbGitStateScannerThread)event.GetClientData();
    test = ( std::string*)event.GetClientData();

    if(*test == "Hallo")
         wxMessageBox(wxString("ItIs"));


//    wxMessageBox(test);
    delete test;

//    cbGitStateScannerThread::std::string threadString = (cbGitStateScannerThread)event.GetClientData();
//    cbGitStateScannerThread::string_t test2 = (cbGitStateScannerThread::string_t)event.GetClientData();
//      test = event.GetClientData();
//    test = cbGitStateScannerThread event.GetClientData();
}

//class cbGitUpdateThread : public wxThread
//{
//public:
//    cbGitUpdateThread(wxEvtHandler *handler, const cbProject &project) :
//            wxThread(wxTHREAD_DETACHED),
//            project_(project),
//            handler_(handler)
//    {
//    }
//    virtual ~cbGitUpdateThread(){}
//private:
//    const cbProject project_;
//
//    void* Entry()
//    {
////    wxMessageBox(wxString("entry"));
//       bool debuggerinseperatethread = false;
////        cbProject prj = prjFile_->GetParentProject();cbGitUpdateThread
////        wxString prjpath = project_.GetFilename();
////        std::string path = std::string(prjpath.mb_str());
////        std::string path = "/home/marx/TestCppGit/awesome_cpp";
////        auto repo = repository::open(path);
//
//
//
//        return 0;
//    }
//
//    wxEvtHandler *handler_;
//};
//void cbGit::OnUpdate(wxCommandEvent& event)
//{
//    cbGitUpdateThread *thread = new cbGitUpdateThread( this, *forCommandSelectedProject );
//    wxMessageBox(wxString("beforeStartingThread"));
////    wxThread *thread = new cbGitUpdateThread( this );
////    cbGitUpdateThread *thread = new cbGitUpdateThread( this );
//    thread->Create();
//    thread->Run();
//
//
////    wxThread *test = new TestThread;
////    test->Create();
////    test->Run();
//
//
//}





