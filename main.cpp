/////////////////////////////////////////////////////////////////////////////
// Name:        main.cpp
// Purpose:     UI for the vicit-diem task tracking tool
// Author:      Julian Smart
// Modified by: Joshua Parfitt
// Created:     04/01/98
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include "wx/button.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "wx/statbox.h"
#include "../cJSON/cJSON.h"
#include <string>
#include <fstream>
using namespace std;

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
    // override base class virtuals
    // ----------------------------

    // this one is called on application startup and is a good place for the app
    // initialization (doing it here and not in the ctor allows to have an error
    // return: if OnInit() returns false, the application terminates)
    virtual bool OnInit() wxOVERRIDE;
};

// define a new frame type: this is going to be our main frame
class MyFrame : public wxFrame
{
public:
    // ctor(s)
    MyFrame(const wxString& title);

    // event handlers (these functions should _not_ be virtual)
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnButtonOK(wxCommandEvent& event);

private:
    // any class wishing to process wxWidgets events must use this macro
    wxDECLARE_EVENT_TABLE();
    wxTextCtrl* textField;
    wxStaticText* JSONdump;
    wxStaticBox* textArea;
};

// function prototypes
int getTotalObjectCount(cJSON* root);
int getTotalArrayCount(cJSON* root);
int getLongestArrayLength(cJSON* root);
int getShortestArrayLength(cJSON* root);
int getTotalKeyCount(cJSON* root);

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // menu items
    Minimal_Quit = wxID_EXIT,

    // it is important for the id corresponding to the "About" command to have
    // this standard value as otherwise it won't be handled properly under Mac
    // (where it is special and put into the "Apple" menu)
    Minimal_About = wxID_ABOUT
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------

// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(Minimal_Quit,  MyFrame::OnQuit)
    EVT_MENU(Minimal_About, MyFrame::OnAbout)
    EVT_BUTTON(wxID_OK, MyFrame::OnButtonOK)
wxEND_EVENT_TABLE()

// create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)
wxIMPLEMENT_APP(MyApp);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
    // call the base class initialization method, currently it only parses a
    // few common command-line options but it could be do more in the future
    if ( !wxApp::OnInit() )
        return false;

    // create the main application window
    MyFrame *frame = new MyFrame("vicit-diem");

    // and show it (the frames, unlike simple controls, are not shown when
    // created initially)
    frame->Show(true);

    // success: wxApp::OnRun() will be called which will enter the main message
    // loop and the application will run. If we returned false here, the
    // application would exit immediately.
    return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
{
    // create a menu bar
    wxMenu *fileMenu = new wxMenu;

    // the "About" item should be in the help menu
    wxMenu *helpMenu = new wxMenu;
    helpMenu->Append(Minimal_About, "&About\tF1", "Show about dialog");

    fileMenu->Append(Minimal_Quit, "E&xit\tAlt-X", "Quit this program");

    // now append the freshly created menu to the menu bar...
    wxMenuBar *menuBar = new wxMenuBar();
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(helpMenu, "&Help");

    // ... and attach this menu bar to the frame
    SetMenuBar(menuBar);

    // label
    wxStaticText* label = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"),
        wxPoint(10, 15), wxDefaultSize, wxALIGN_LEFT);

    // text field
    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(160, 10), wxSize(125, 20));

    // button
    wxButton* button = new wxButton(this, wxID_OK, wxT("Load"),
        wxPoint(300, 10), wxDefaultSize);

    // text area
    textArea = new wxStaticBox(this, wxID_ANY,
        wxT(""), wxPoint(20, 55), wxSize(360, 120));

    // create a status bar just for fun (by default with 1 pane only)
    CreateStatusBar(2);
    SetStatusText("Welcome to vicit-diem!");
}

// event handlers
void MyFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
    // true is to force the frame to close
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
    wxMessageBox(wxString::Format
                 (
                    "Welcome to vicit-diem!\n"
                    "\n"
                    "This is the vicit-diem app\n"
                    "running under %s.",
                    wxVERSION_STRING,
                    wxGetOsDescription()
                 ),
                 "About vicit-diem",
                 wxOK | wxICON_INFORMATION,
                 this);
}

void MyFrame::OnButtonOK(wxCommandEvent& event)
{
    wxString wxStringPath = textField->GetValue();
    string stringPath = wxStringPath.ToStdString();

    ifstream fin;
    fin.open(stringPath);

    if (fin)
    {
        // parse the JSON file
        string file, line; 
        while (getline(fin, line))
            file += line; 
        cJSON *root = cJSON_Parse(file.c_str());

        // count the total number of objects
        int totalObjectCount = getTotalObjectCount(root);

        // count the total number of arrays
        int totalArrayCount = getTotalArrayCount(root);

        // return the length of the longest array
        int longestArrayLength = getLongestArrayLength(root);

        // return the length of the shortest array
        int shortestArrayLength = getShortestArrayLength(root);

        // count the total number of keys
        int totalKeyCount = getTotalKeyCount(root);

        // Show statistics
        string stringStatistics = "Total Number of Objects: " + to_string(totalObjectCount) + "\n"
                                + "Total Number of Arrays: " + to_string(totalArrayCount) + "\n"
                                + "Length of Longest Array: " + to_string(longestArrayLength) + "\n"
                                + "Length of Shortest Array: " + to_string(shortestArrayLength) + "\n"
                                + "Total Number of Keys: " + to_string(totalKeyCount);
        wxString wxStringStatistics(stringStatistics);
        JSONdump = new wxStaticText(textArea, wxID_ANY, wxStringStatistics, wxPoint(5, 5));

        // print the contents of the JSON file
    }
    else
    {
        wxString errorMessage = wxString::Format(wxT("File %s was not found"), stringPath.c_str());
        JSONdump = new wxStaticText(textArea, wxID_ANY, errorMessage, wxPoint(5, 5));
    }
    fin.close();
}

int getTotalObjectCount(cJSON* root)
{
    int objectCount = 0;
    if (root->type == cJSON_Object)
        objectCount++;
    
    cJSON* child = root->child;
    while (child)
    {
        objectCount += getTotalObjectCount(child);
        child = child->next;
    }
    return objectCount;
}

int getTotalArrayCount(cJSON* root)
{
    int arrayCount = 0;
    if (root->type == cJSON_Array)
        arrayCount++;

    cJSON* child = root->child;
    while (child)
    {
        arrayCount += getTotalArrayCount(child);
        child = child->next;
    }
    return arrayCount;
}

int getLongestArrayLength(cJSON* root)
{
    int longestArrayLength = 0;
    if (root->type == cJSON_Array)
    {
        int arrayLength = cJSON_GetArraySize(root);
        if (arrayLength > longestArrayLength)
            longestArrayLength = arrayLength;
    }
    
    cJSON* child = root->child;
    while (child)
    {
        int childLength = getLongestArrayLength(child);
        if (childLength > longestArrayLength)
            longestArrayLength = childLength;
        child = child->next;
    }
    return longestArrayLength;
}

int getShortestArrayLength(cJSON* root)
{
    int shortestArrayLength = INT_MAX;
    if (root->type == cJSON_Array)
    {
        int arrayLength = cJSON_GetArraySize(root);
        if (arrayLength < shortestArrayLength)
            shortestArrayLength = arrayLength;
    }
    
    cJSON* child = root->child;
    while (child)
    {
        int childLength = getShortestArrayLength(child);
        if (childLength < shortestArrayLength)
            shortestArrayLength = childLength;
        child = child->next;
    }
    return shortestArrayLength;
}

int getTotalKeyCount(cJSON* root)
{
    int totalKeyCount = 0;
    if (root->type == cJSON_Object) 
    {
        cJSON* child = root->child;
        while (child) 
        {
            if (!(child->string && child->string[0] == '['))
            {
                totalKeyCount++;
            }
            totalKeyCount += getTotalKeyCount(child);
            child = child->next;
        }
    } 
    else if (root->type == cJSON_Array) 
    {
        cJSON* child = root->child;
        while (child) 
        {
            totalKeyCount += getTotalKeyCount(child);
            child = child->next;
        }
    } 
    return totalKeyCount;
}

// Path to JSON file:   /Users/joshuaparfitt/Development/basic.json
//                      /Users/joshuaparfitt/Development/task_should_be_recorded_but_wasnt.json  
//                      /Users/joshuaparfitt/Development/no_name.json       