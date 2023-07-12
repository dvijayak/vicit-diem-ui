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
#include "wx/checkbox.h"
#include "../cJSON/cJSON.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
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
    void OnCheckbox(wxCommandEvent& event);

private:
    // any class wishing to process wxWidgets events must use this macro
    wxDECLARE_EVENT_TABLE();
    wxStaticText* label;
    wxTextCtrl* textField;
    wxButton* button;
    wxStaticText* name;
    wxStaticText* date;
    wxStaticBox* textArea;
    wxPanel* panel;
    wxStaticText* staticText;
    wxScrolledWindow* scrolledWindow;
};

string getName(cJSON* node);
cJSON* getTaskDatabase(cJSON* node);
void getTitles(cJSON* node, vector<string>& titles);
void getDailyIDs(cJSON* node, vector<string>& dailyIDs);
void showTasks(vector<string> titles, wxStaticText* staticText, wxScrolledWindow* scrolledWindow);
string getNextDate(cJSON* node);
bool isAssignedDay(cJSON* node);

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
    EVT_BUTTON(wxEVT_BUTTON, MyFrame::OnButtonOK)
    EVT_CHECKBOX(wxEVT_CHECKBOX, MyFrame::OnCheckbox)
wxEND_EVENT_TABLE()

// create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (count.e. MyApp and
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
    label = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"), wxPoint(10, 15), wxDefaultSize, wxALIGN_LEFT);

    // text field
    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(160, 10), wxSize(125, 20));

    // button
    button = new wxButton(this, wxEVT_BUTTON, 
        wxT("Load"), wxPoint(300, 10), wxDefaultSize);

    // date
    wxDateTime wxDate = wxDateTime::Today();

    wxString wxStringDay = wxDate.GetWeekDayName(wxDate.GetWeekDay(), wxDate.Name_Full); 
    wxString wxStringDate = wxDate.FormatDate();

    date = new wxStaticText(this, wxID_ANY,
        (wxStringDay + wxT(" ") + wxStringDate), wxPoint(165, 40), wxDefaultSize);

    // panel
    panel = new wxPanel(this, wxID_ANY,
        wxPoint(20, 70), wxSize(360, 120));

    // scrolled window
    scrolledWindow = new wxScrolledWindow(panel, wxID_ANY, 
        wxPoint(0, 0), wxSize(360, 120), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);   

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

        // show the name
        string stringName = getName(root);
        wxString wxStringName(stringName);

        name = new wxStaticText(this, wxID_ANY,
           wxStringName, wxPoint(95, 40), wxDefaultSize); 

        // get the task database corresponding to the current date
        cJSON* taskDatabase = getTaskDatabase(root);

        // get the titles corresponding to the current day of the week
        vector<string> titles;
        getTitles(taskDatabase, titles);

        // get the daily IDs corresponding to the current day of the week
        vector<string> dailyIDs;
        getDailyIDs(taskDatabase, dailyIDs);

        // show tasks for the day
        showTasks(titles, staticText, scrolledWindow);
    }
    else
    {
        wxString errorMessage = wxString::Format(wxT("File %s was not found"), stringPath.c_str());
        staticText = new wxStaticText(scrolledWindow, wxID_ANY, errorMessage, wxPoint(5, 5));
    }
    fin.close();
}

// get the name
string getName(cJSON* node)
{
    string name = "name";
    
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getName(child);
            child = child->next;
        }
    }
    else if (node->type == cJSON_Object)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (child->string == name)
                return child->valuestring;

            child = child->next;
        }
    }
    return NULL;
}

// get the task database corresponding to the current date
cJSON* getTaskDatabase(cJSON* node)
{
    wxDateTime currentDate = wxDateTime::Today();
    string date = "date";
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
            if (child->string == date)
            {
                wxDateTime afterDate = NULL;
                wxDateTime beforeDate = NULL;

                afterDate.ParseDate(child->valuestring);
                beforeDate.ParseDate(getNextDate(node->next));

                if (currentDate.IsSameDate(afterDate) || currentDate.IsStrictlyBetween(afterDate, beforeDate))
                {
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (child->string == taskDatabase)
                            return child;

                        child = child->next;
                    }
                }
            }
            else if (getTaskDatabase(child))
                return getTaskDatabase(child);
            
            child = child->next;
        }
    }
    return NULL;
} 

string getNextDate(cJSON* node)
{
    string date = "date";

    cJSON* child = node->child;
    while (child)
    {
        if (child->string == date)
            return child->valuestring;
        
        child = child->next;
    }
    return NULL;
}

// get the titles corresponding to the current day of the week
void getTitles(cJSON* node, vector<string>& titles)
{
    string title = "title";
    string assignedDays = "assigned_days";

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getTitles(child, titles);
            child = child->next;
        }
    }
    else if (node->type == cJSON_Object)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (child->string == assignedDays)
            {
                if (isAssignedDay(child))
                {
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (child->string == title)
                            titles.push_back(child->valuestring);
                        
                        child = child->next;
                    }
                }
            }
            child = child->next;
        }
    }
}

// get the daily IDs corresponding to the current day of the week
void getDailyIDs(cJSON* node, vector<string>& dailyIDs)
{
    string dailyID = "daily_id";
    string assignedDays = "assigned_days";

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getDailyIDs(child, dailyIDs);
            child = child->next;
        }
    }
    else if (node->type == cJSON_Object)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (child->string == assignedDays)
            {
                if (isAssignedDay(child))
                {   
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (child->string == dailyID)
                            dailyIDs.push_back(child->valuestring);
                        
                        child = child->next;
                    }
                }
            }
            child = child->next;
        }
    }
}

bool isAssignedDay(cJSON* node)
{
    wxDateTime today = wxDateTime::Today();

    string everyday = "Everyday", weekdays = "Weekdays", weekends = "Weekends";
    
    vector<string> everydayExpansion = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    vector<string> weekdaysExpansion = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    vector<string> weekendsExpansion = {"Saturday", "Sunday"};

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (child->valuestring == everyday)
            {
                for (int count = 0; count < everydayExpansion.size(); count++)
                {
                    wxDateTime day = NULL;
                    day.ParseDate(everydayExpansion[count]);

                    if (today.IsSameDate(day))
                        return true;
                }
            }
            else if (child->valuestring == weekdays)
            {
                for (int count = 0; count < weekdaysExpansion.size(); count++)
                {
                    wxDateTime day = NULL;
                    day.ParseDate(weekdaysExpansion[count]);

                    if (today.IsSameDate(day))
                        return true;
                }
            }
            else if (child->valuestring == weekends)
            {
                for (int count = 0; count < weekendsExpansion.size(); count++)
                {
                    wxDateTime day = NULL;
                    day.ParseDate(weekendsExpansion[count]);

                    if (today.IsSameDate(day))
                        return true;
                }
            }
            else
            {
                wxDateTime day = NULL;
                day.ParseDate(child->valuestring);

                if (today.IsSameDate(day))
                    return true;
            }
            child = child->next;
        }
    }
    else
    {
        if (node->valuestring == everyday)
        {
            for (int count = 0; count < everydayExpansion.size(); count++)
            {
                wxDateTime day = NULL;
                day.ParseDate(everydayExpansion[count]);

                if (today.IsSameDate(day))
                    return true;
            }
        }
        else if (node->valuestring == weekdays)
        {
            for (int count = 0; count < weekdaysExpansion.size(); count++)
            {
                wxDateTime day = NULL;
                day.ParseDate(weekdaysExpansion[count]);

                if (today.IsSameDate(day))
                    return true;
            }
        }
        else if (node->valuestring == weekends)
        {
            for (int count = 0; count < weekendsExpansion.size(); count++)
            {
                wxDateTime day = NULL;
                day.ParseDate(weekendsExpansion[count]);

                if (today.IsSameDate(day))
                    return true;
            }
        }
        else
        {
            wxDateTime day = NULL;
            day.ParseDate(node->valuestring);

            if (today.IsSameDate(day))
                return true;
        }
    }
    return false;
}

// show the tasks for the day
void showTasks(vector<string> titles, wxStaticText* staticText, wxScrolledWindow* scrolledWindow)
{
    int position = 0;

    for (int count = 0; count < titles.size(); count++)
    {
        wxString wxStringTask(titles[count]);

        wxCheckBox* checkbox = new wxCheckBox(scrolledWindow, wxEVT_CHECKBOX,
            wxStringTask, wxPoint(0, position), wxDefaultSize);

        checkbox->SetValue(false);

        position += 30;
    }
}

void MyFrame::OnCheckbox(wxCommandEvent& event)
{
    cout << "Success ";
}

// Path to JSON file:   /Users/joshuaparfitt/Development/many_schemas.json