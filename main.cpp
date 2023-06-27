/////////////////////////////////////////////////////////////////////////////
// Name:        main.cpp
// Purpose:     UI for the vicit-diem task tracking tool
// Author:      Julian Smart
// Modified by: Joshua Parfitt
// Created:     04/01/98
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"
#include "wx/button.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "wx/statbox.h"
#include "wx/sizer.h"
#include "wx/scrolwin.h"
#include "../cJSON/cJSON.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
using namespace std;

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWidgets headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

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
    wxScrolledWindow* scrolledWindow;
};

cJSON* getTaskDatabaseRecords(cJSON* node);
cJSON* getTaskDatabase(cJSON* node);
cJSON* getDailyRecord(cJSON* node);
wxDateTime parseDate(string stringDate);

int getTotalObjectCount(cJSON* node);
int getTotalArrayCount(cJSON* node);
int getLongestArrayLength(cJSON* node);
int getShortestArrayLength(cJSON* node);
int getTotalKeyCount(cJSON* node);
void appendSuffix(cJSON* node);

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
        wxT("Enter path to JSON file"), wxPoint(10, 15), wxDefaultSize, wxALIGN_LEFT);

    // text field
    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(160, 10), wxSize(125, 20));

    // button
    wxButton* button = new wxButton(this, wxID_OK, 
        wxT("Load"), wxPoint(300, 10), wxDefaultSize);

    // text area
    textArea = new wxStaticBox(this, wxID_ANY,
        wxT(""), wxPoint(20, 55), wxSize(360, 120));

    // scroll bar
    scrolledWindow = new wxScrolledWindow(textArea, wxID_ANY, 
        wxPoint(0, 0), wxSize(400, 400), wxVSCROLL|wxHSCROLL);

    int pixelsPerUnitX = 10;
    int pixelsPerUnitY = 10;
    int noUnitsX = 200;
    int noUnitsY = 200;

    scrolledWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);     

    // get name

    // get the current date
    
    // create a status bar (by default with 1 pane only)
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
        string line, file; 

        // read the JSON file
        while (getline(fin, line))
            file += line; 
        
        // parse the JSON file
        cJSON* root = cJSON_Parse(file.c_str());

        // get the task database records
        cJSON* taskDatabaseRecords = getTaskDatabaseRecords(root);

        // get the task database corresponding to the current date
        cJSON* taskDatabase = getTaskDatabase(taskDatabaseRecords);

        // get the daily record
        cJSON* dailyRecord = getDailyRecord(root);

        // get statistics
        int totalObjectCount = getTotalObjectCount(taskDatabaseRecords);
        int totalArrayCount = getTotalArrayCount(taskDatabaseRecords);
        int longestArrayLength = getLongestArrayLength(taskDatabaseRecords);
        int shortestArrayLength = getShortestArrayLength(taskDatabaseRecords);
        int totalKeyCount = getTotalKeyCount(taskDatabaseRecords);

        // append the suffix "_PROCESSED" to each key
        appendSuffix(taskDatabaseRecords);

        // print the JSON file
        char* JSONfile = cJSON_Print(taskDatabase);

        // show statistics
        string stringStatistics = "Total Number of Objects: " + to_string(totalObjectCount) + "\n"
                                + "Total Number of Arrays: " + to_string(totalArrayCount) + "\n"
                                + "Length of Longest Array: " + to_string(longestArrayLength) + "\n"
                                + "Length of Shortest Array: " + to_string(shortestArrayLength) + "\n"
                                + "Total Number of Keys: " + to_string(totalKeyCount) + "\n"
                                + JSONfile;

        wxString wxStringStatistics(stringStatistics);
        JSONdump = new wxStaticText(scrolledWindow, wxID_ANY, wxStringStatistics, wxPoint(5, 5));
    }
    else
    {
        wxString errorMessage = wxString::Format(wxT("File %s was not found"), stringPath.c_str());
        JSONdump = new wxStaticText(scrolledWindow, wxID_ANY, errorMessage, wxPoint(5, 5));
    }
    fin.close();
}

// count the total number of objects
int getTotalObjectCount(cJSON* node)
{
    int objectCount = 0;
    if (node->type == cJSON_Object)
        objectCount++;
    
    cJSON* child = node->child;
    while (child)
    {
        objectCount += getTotalObjectCount(child);
        child = child->next;
    }
    return objectCount;
}

// count the total number of arrays
int getTotalArrayCount(cJSON* node)
{
    int arrayCount = 0;
    if (node->type == cJSON_Array)
        arrayCount++;

    cJSON* child = node->child;
    while (child)
    {
        arrayCount += getTotalArrayCount(child);
        child = child->next;
    }
    return arrayCount;
}

// get the length of the longest array
int getLongestArrayLength(cJSON* node)
{
    int longestArrayLength = 0;
    if (node->type == cJSON_Array)
    {
        int arrayLength = cJSON_GetArraySize(node);
        if (arrayLength > longestArrayLength)
            longestArrayLength = arrayLength;
    }
    
    cJSON* child = node->child;
    while (child)
    {
        int childLength = getLongestArrayLength(child);
        if (childLength > longestArrayLength)
            longestArrayLength = childLength;
        child = child->next;
    }
    return longestArrayLength;
}

// get the length of the shortest array
int getShortestArrayLength(cJSON* node)
{
    int shortestArrayLength = INT_MAX;
    if (node->type == cJSON_Array)
    {
        int arrayLength = cJSON_GetArraySize(node);
        if (arrayLength < shortestArrayLength)
            shortestArrayLength = arrayLength;
    }
    
    cJSON* child = node->child;
    while (child)
    {
        int childLength = getShortestArrayLength(child);
        if (childLength < shortestArrayLength)
            shortestArrayLength = childLength;
        child = child->next;
    }
    return shortestArrayLength;
}

// count the total number of keys
int getTotalKeyCount(cJSON* node)
{
    int totalKeyCount = 0;
    if (node->type == cJSON_Object) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            if (child->string)
                totalKeyCount++;

            totalKeyCount += getTotalKeyCount(child);
            child = child->next;
        }
    } 
    else if (node->type == cJSON_Array) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            totalKeyCount += getTotalKeyCount(child);
            child = child->next;
        }
    } 
    return totalKeyCount;
}

// append the suffix "_PROCESSED" to each key
void appendSuffix(cJSON* node)
{
    if (node->type == cJSON_Object) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            if (child->valuestring)
            {
                int size = strlen(child->valuestring) + strlen("_PROCESSED") + 1;
                child->valuestring = (char*) realloc(child->valuestring, size);
                strcat(child->valuestring, "_PROCESSED");
            }
            appendSuffix(child);
            child = child->next;
        }
    }
    else if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            appendSuffix(child);
            child = child->next;
        }
    }
}

// get the task database records
cJSON* getTaskDatabaseRecords(cJSON* node)
{
    string taskDatabaseRecords = "taskdb_records";

    if (node->type == cJSON_Array) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            getTaskDatabaseRecords(child);
            child = child->next;
        }
    } 
    else if (node->type == cJSON_Object) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            if (child->string == taskDatabaseRecords)
                return child;
            else 
                getTaskDatabaseRecords(child);
            
            child = child->next;
        }
    }
    return NULL;
}

// get the task database corresponding to the current date
cJSON* getTaskDatabase(cJSON* node)
{
    string key = "date";
    string value = "01/07/2021";
    string taskDatabase = "taskdb";

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (getTaskDatabase(child))
                return getTaskDatabase(child);

            child = child->next;
        }
    }
    else if (node->type == cJSON_Object)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (child->string == key && child->valuestring == value)
            {
                cJSON* prev = child->prev;
                cJSON* next = child->next;

                while (prev->string || next->valuestring)
                {
                    if (prev->string == taskDatabase)
                        return prev;
                    else if (next->string == taskDatabase)
                        return next;
                    
                    prev = prev->prev;
                    next = next->next;
                }
            }
            else if (getTaskDatabase(child))
                return getTaskDatabase(child);
            
            child = child->next;
        }
    }
    return NULL;
}

// get the daily record
cJSON* getDailyRecord(cJSON* node)
{
    string dailyRecord = "daily_record";

    if (node->type == cJSON_Array) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            getDailyRecord(child);
            child = child->next;
        }
    } 
    else if (node->type == cJSON_Object) 
    {
        cJSON* child = node->child;
        while (child) 
        {
            if (child->string == dailyRecord)
                return child;  
            else 
                getDailyRecord(child);
            
            child = child->next;
        }
    }
    return NULL;
}

// Path to JSON file:   /Users/joshuaparfitt/Development/many_schemas.json