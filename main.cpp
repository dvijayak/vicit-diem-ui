/////////////////////////////////////////////////////////////////////////////
// Name:        main.cpp
// Purpose:     UI for the vicit-diem task-tracking tool
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"
#include "wx/button.h"
#include "wx/stattext.h"
#include "wx/textctrl.h"
#include "wx/statbox.h"
#include "wx/sizer.h"
#include "wx/scrolwin.h"
#include "wx/checkbox.h"
#include "wx/filedlg.h"
#include "wx/datectrl.h"
#include "wx/calctrl.h"
#include "wx/stdpaths.h"
#include "wx/filename.h"
#include "../cJSON/cJSON.h"
#include <string>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
using namespace std;

#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

class MyApp : public wxApp
{
    public:
        virtual bool OnInit() wxOVERRIDE;
};

class MyFrame : public wxFrame
{
    public:
        MyFrame(const wxString& title);

        void OnLoadButton(wxCommandEvent& event);
        void OnOpenFileDialogButton(wxCommandEvent& event);
        void OnCalendarCtrl(wxCalendarEvent& event);
        void OnCheckbox(wxCommandEvent& event);
        void OnSaveButton(wxCommandEvent& event);

    private:
        wxDECLARE_EVENT_TABLE();

        /////////////////////////////////////////////////////////////////////////////
        // functions
        /////////////////////////////////////////////////////////////////////////////

        void loadAndDisplayTaskData(string& filePath, cJSON* node);

        string getUserDataDir();
        void saveFilePathToUserDataDir();
        string loadFilePathFromUserDataDir();

        string getUserName(cJSON* node);
        cJSON* getTaskDatabase(cJSON* node);
        string getDateForNextTaskDatabase(cJSON* node);
        cJSON* getDailyRecord(cJSON* node);
        cJSON* getEntryFromDailyRecord(cJSON* node);
        void addEntryToDailyRecord(cJSON* node);

        void getTitles(cJSON* node, vector<string>& titles);
        void getDailyIDs(cJSON* node, vector<string>& dailyIDs);
        bool isAssignedDay(cJSON* node);
        bool correspondsToAssignedDay(cJSON* node);

        void addMissingTasksToRecord(cJSON* node, vector<string> dailyIDs);
        void deleteExtraTasksFromRecord(cJSON* node, vector<string> dailyIDs);
        void showTasks(vector<string> titles, vector<string> dailyIDs, vector<wxCheckBox*>& checkboxList, vector<cJSON*>& statusSet);

        cJSON* getTaskStatus(cJSON* node, string dailyID);
        void updateTaskStatus(cJSON* node, cJSON* status);

        void resetScrolledWindow(wxPanel* panel);
        void showErrorMessage(wxString errorMessage);

        /////////////////////////////////////////////////////////////////////////////
        // variables
        /////////////////////////////////////////////////////////////////////////////

        wxTextCtrl* textField = NULL;
        wxCalendarCtrl* calendarCtrl = NULL;
        wxString userName = "";
        wxPanel* panel = NULL;
        wxScrolledWindow* scrolledWindow = NULL;
        
        wxDateTime date = NULL;
        string filePath = "";
        cJSON* root = NULL;
        cJSON* dailyRecord = NULL;
        cJSON* record = NULL;

        vector<string> titles;
        vector<string> dailyIDs;
        vector<wxCheckBox*> checkboxList;
        vector<cJSON*> statusSet;
};

/////////////////////////////////////////////////////////////////////////////
// event table 
/////////////////////////////////////////////////////////////////////////////

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(wxEVT_BUTTON, MyFrame::OnLoadButton)
    EVT_BUTTON(wxID_OPEN, MyFrame::OnOpenFileDialogButton)
    EVT_CALENDAR_SEL_CHANGED(wxID_ANY, MyFrame::OnCalendarCtrl)
    EVT_CHECKBOX(wxEVT_CHECKBOX, MyFrame::OnCheckbox)
    EVT_BUTTON(wxID_SAVE, MyFrame::OnSaveButton)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    MyFrame* frame = new MyFrame("vicit-diem");

    frame->Show(true);

    return true;
}

/////////////////////////////////////////////////////////////////////////////
// main frame
/////////////////////////////////////////////////////////////////////////////

MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(775, 250))
{
    wxStaticText* label = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"), wxPoint(25, 15), wxDefaultSize, wxALIGN_LEFT);

    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(175, 10), wxSize(125, 20));
    
    string path = loadFilePathFromUserDataDir();
    textField->SetValue(path);

    wxButton* loadButton = new wxButton(this, wxEVT_BUTTON, 
        wxT("Load"), wxPoint(325, 10), wxDefaultSize);

    wxButton* openButton = new wxButton(this, wxID_OPEN,
        wxT("Open"), wxPoint(425, 10), wxDefaultSize);
    
    calendarCtrl = new wxCalendarCtrl(this, wxID_ANY, 
        wxDefaultDateTime, wxPoint(525, 10));

    wxDateTime today = wxDateTime::Today();

    wxStaticText* dateLabel = new wxStaticText(this, wxID_ANY,
        (today.GetWeekDayName(today.GetWeekDay(), today.Name_Full) + wxT("\t") + today.Format("%m/%d/%Y")), 
        wxPoint(350, 40), wxDefaultSize);

    panel = new wxPanel(this, wxID_ANY,
        wxPoint(125, 75), wxSize(375, 75));
        
    resetScrolledWindow(panel);

    wxButton* saveButton = new wxButton(this, wxID_SAVE, 
        wxT("Save"), wxPoint(325, 175), wxDefaultSize);
}

/////////////////////////////////////////////////////////////////////////////
// event handlers 
/////////////////////////////////////////////////////////////////////////////

void MyFrame::OnLoadButton(wxCommandEvent& event)
{
    loadAndDisplayTaskData(filePath, root);
}

void MyFrame::OnOpenFileDialogButton(wxCommandEvent& event)
{
    wxFileDialog dialog(this);

    if (dialog.ShowModal() == wxID_OK) 
    {
        wxString path = dialog.GetPath();
        textField->SetValue(path);
    }
}

void MyFrame::OnCalendarCtrl(wxCalendarEvent& event)
{
    if (root)
    {
        loadAndDisplayTaskData(filePath, root);
    }
    else
    {
        resetScrolledWindow(panel);

        wxString errorMessage = "You must load a file";
        showErrorMessage(errorMessage);
    }
}

void MyFrame::OnCheckbox(wxCommandEvent& event)
{
    wxCheckBox* checkbox = (wxCheckBox*) event.GetEventObject();

    for (int count = 0; count < checkboxList.size(); count++)
    {
        if (checkbox == checkboxList[count])
        {
            updateTaskStatus(record, statusSet[count]);
            statusSet[count] = getTaskStatus(record, dailyIDs[count]);
        }
    }
}

void MyFrame::OnSaveButton(wxCommandEvent& event)
{
    if (!filePath.empty())
    {    
        ifstream fin;
        fin.open(filePath);

        if (fin)
        {
            ofstream fout;
            fout.open(filePath);

            char* file = cJSON_Print(root);
            fout << file;

            fout.close();
        }
        else
        {
            resetScrolledWindow(panel);

            wxString errorMessage = "File \"%s\" could not be saved because it does not exist";
            showErrorMessage(errorMessage);
        }
        fin.close();
    }
    else
    {
        resetScrolledWindow(panel);

        wxString errorMessage = "You must load a file before it can be saved";
        showErrorMessage(errorMessage);
    }
}

/////////////////////////////////////////////////////////////////////////////
// helper functions 
/////////////////////////////////////////////////////////////////////////////

void MyFrame::loadAndDisplayTaskData(string& filePath, cJSON* node)
{
    resetScrolledWindow(panel);

    date = calendarCtrl->GetDate();
    
    wxString text = textField->GetValue();
    filePath = text.ToStdString();

    ifstream fin;
    fin.open(filePath);

    if (fin)
    {
        string line, file; 

        while (getline(fin, line))
            file += line; 
        
        root = cJSON_Parse(file.c_str());

        if (userName.IsEmpty())
        {
            userName = getUserName(root);
            wxStaticText* userNameLabel = new wxStaticText(this, wxID_ANY,
                userName, wxPoint(125, 40), wxDefaultSize);
        }
        titles.clear();
        dailyIDs.clear();
        checkboxList.clear();
        statusSet.clear();
        
        cJSON* taskDatabase = getTaskDatabase(root);
        dailyRecord = getDailyRecord(root);

        getTitles(taskDatabase, titles);
        getDailyIDs(taskDatabase, dailyIDs);

        if (!getEntryFromDailyRecord(root))
            addEntryToDailyRecord(dailyRecord);

        record = getEntryFromDailyRecord(root);

        addMissingTasksToRecord(record, dailyIDs);
        deleteExtraTasksFromRecord(record, dailyIDs);
        showTasks(titles, dailyIDs, checkboxList, statusSet);

        saveFilePathToUserDataDir();
    }
    else
    {
        wxString errorMessage = "File \"%s\" was not found";
        showErrorMessage(errorMessage);
    }
    fin.close();
}

string MyFrame::getUserDataDir()
{
    wxStandardPaths globalStandardPathsObject = wxStandardPaths::Get();
    wxString userDataDir = globalStandardPathsObject.GetUserDataDir();
    
    bool exists = wxDirExists(userDataDir);
    if (!exists)
        wxMkdir(userDataDir);

    string path = userDataDir.ToStdString();
    path += "/filePath.txt";

    return path;
}

void MyFrame::saveFilePathToUserDataDir()
{
    string path = getUserDataDir();

    ofstream fout;
    fout.open(path);

    fout << filePath;
}

string MyFrame::loadFilePathFromUserDataDir()
{
    string path = getUserDataDir();
    string filePath;

    ifstream fin;
    fin.open(path);

    fin >> filePath;
    return filePath;
}

string MyFrame::getUserName(cJSON* node)
{
    cJSON* child = node->child;
    while (child)
    {
        if (strcmp(child->string, "name") == 0)
            return child->valuestring;
        
        child = child->next;
    }
    return NULL;
}

cJSON* MyFrame::getTaskDatabase(cJSON* node)
{
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
            if (strcmp(child->string, "date") == 0)
            {
                wxDateTime afterDate = NULL;
                wxDateTime beforeDate = NULL;

                if (node->next)
                {
                    afterDate.ParseDate(child->valuestring);
                    beforeDate.ParseDate(getDateForNextTaskDatabase(node->next));
            
                    if (date.IsSameDate(afterDate) || date.IsStrictlyBetween(afterDate, beforeDate))
                    {
                        cJSON* child = node->child;
                        while (child)
                        {
                            if (strcmp(child->string, "taskdb") == 0)
                                return child;

                            child = child->next;
                        }
                    }
                }
                else
                {
                    afterDate.ParseDate(child->valuestring);

                    if (date.IsSameDate(afterDate) || date.IsLaterThan(afterDate))
                    {
                        cJSON* child = node->child;
                        while (child)
                        {
                            if (strcmp(child->string, "taskdb") == 0)
                                return child;

                            child = child->next;
                        }
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

string MyFrame::getDateForNextTaskDatabase(cJSON* node)
{
    cJSON* child = node->child;
    while (child)
    {
        if (strcmp(child->string, "date") == 0)
            return child->valuestring;
        
        child = child->next;
    }
    return NULL;
}

cJSON* MyFrame::getDailyRecord(cJSON* node)
{
    cJSON* child = node->child;
    while (child)
    {
        if (strcmp(child->string, "daily_record") == 0)
            return child;

        child = child->next;
    }
    return NULL;
}

cJSON* MyFrame::getEntryFromDailyRecord(cJSON* node)
{
    cJSON* child = node->child;
    while (child)
    {
        if (strcmp(child->string, "daily_record") == 0)
        {
            cJSON* grandChild = child->child;
            while (grandChild)
            {
                wxDateTime timestamp = NULL;
                timestamp.ParseDate(grandChild->string);

                if (date.IsSameDate(timestamp))
                    return grandChild;
                
                grandChild = grandChild->next;
            }
        }
        child = child->next;
    }
    return NULL;
}

void MyFrame::addEntryToDailyRecord(cJSON* node)
{
    cJSON* record = cJSON_CreateObject();

    for (int count = 0; count < dailyIDs.size(); count++)
        cJSON_AddItemToObject(record, dailyIDs[count].c_str(), cJSON_CreateNumber(0));

    cJSON_ReplaceItemInObject(record, date.Format("%m/%d/%Y"), record);

    record->next = node->child;
    node->child = record;
}

void MyFrame::getTitles(cJSON* node, vector<string>& titles)
{
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

            if (strcmp(child->string, "assigned_days") == 0)
            {
                if (isAssignedDay(child))
                {
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (strcmp(child->string, "title") == 0)
                            titles.push_back(child->valuestring);
                        
                        child = child->next;
                    }
                }
            }
            child = child->next;
        }
    }
}

void MyFrame::getDailyIDs(cJSON* node, vector<string>& dailyIDs)
{
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
            if (strcmp(child->string, "assigned_days") == 0)
            {
                if (isAssignedDay(child))
                {
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (strcmp(child->string, "daily_id") == 0)
                            dailyIDs.push_back(child->valuestring);
                        
                        child = child->next;
                    }
                }
            }
            child = child->next;
        }
    }
}

bool MyFrame::isAssignedDay(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (correspondsToAssignedDay(child))
                return true;

            child = child->next;
        }
    }
    else
    {
        if (correspondsToAssignedDay(node))
            return true;
    }
    return false;
}

bool MyFrame::correspondsToAssignedDay(cJSON* node)
{
    wxString dateName = date.GetWeekDayName(date.GetWeekDay(), date.Name_Full); 

    vector<string> everyday = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    vector<string> weekdays = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    vector<string> weekends = {"Saturday", "Sunday"};

    if (strcmp(node->valuestring, "Everyday") == 0)
    {
        for (int count = 0; count < everyday.size(); count++)
        {
            wxDateTime day = NULL;
            day.ParseDate(everyday[count]);

            wxString dayName = day.GetWeekDayName(day.GetWeekDay(), day.Name_Full); 

            if (dateName == dayName)
                return true;
        }
    }
    else if (strcmp(node->valuestring, "Weekdays") == 0)
    {
        for (int count = 0; count < weekdays.size(); count++)
        {
            wxDateTime day = NULL;
            day.ParseDate(weekdays[count]);

            wxString dayName = day.GetWeekDayName(day.GetWeekDay(), day.Name_Full); 

            if (dateName == dayName)
                return true;
        }
    }
    else if (strcmp(node->valuestring, "Weekends") == 0)
    {
        for (int count = 0; count < weekends.size(); count++)
        {
            wxDateTime day = NULL;
            day.ParseDate(weekends[count]);

            wxString dayName = day.GetWeekDayName(day.GetWeekDay(), day.Name_Full); 

            if (dateName == dayName)
                return true;
        }
    }
    else
    {
        wxDateTime day = NULL;
        day.ParseDate(node->valuestring);

        wxString dayName = day.GetWeekDayName(day.GetWeekDay(), day.Name_Full); 

        if (dateName == dayName)
            return true;
    }
    return false;
}

void MyFrame::addMissingTasksToRecord(cJSON* node, vector<string> dailyIDs)
{
    for (int count = 0; count < dailyIDs.size(); count++)
    {
        bool found = false;

        cJSON* child = node->child;
        while (child)
        {
            if (dailyIDs[count] == child->string)
                found = true;

            child = child->next;
        }
        if (!found)
            cJSON_AddItemToObject(node, dailyIDs[count].c_str(), cJSON_CreateNumber(0));
    }
}

void MyFrame::deleteExtraTasksFromRecord(cJSON* node, vector<string> dailyIDs)
{
    cJSON* child = node->child;
    while (child)
    {
        bool found = false;

        for (int count = 0; count < dailyIDs.size(); count++)
        {
            if (child->string == dailyIDs[count])
                found = true;
        }
        if (!found)
            cJSON_DeleteItemFromObject(node, child->string);

        child = child->next;
    }
}

void MyFrame::showTasks(vector<string> titles, vector<string> dailyIDs, vector<wxCheckBox*>& checkboxList, vector<cJSON*>& statusSet)
{
    int position = 0;

    for (int count = 0; count < titles.size(); count++)
    {
        wxString title(titles[count]);
        
        cJSON* status = getTaskStatus(record, dailyIDs[count]);
        
        wxCheckBox* checkbox = new wxCheckBox(scrolledWindow, wxEVT_CHECKBOX,
            title, wxPoint(0, position), wxDefaultSize, wxCHK_3STATE + wxCHK_ALLOW_3RD_STATE_FOR_USER);

        if (status->valueint)
        {
            if (status->valueint == 0)
                checkbox->Set3StateValue(wxCHK_UNCHECKED);
            else if (status->valueint == 1)
                checkbox->Set3StateValue(wxCHK_CHECKED);
        }
        else if (status->valuestring)
        {
            if (strcmp(status->valuestring, "N/A") == 0)
                checkbox->Set3StateValue(wxCHK_UNDETERMINED);
        }
        checkboxList.push_back(checkbox);
        statusSet.push_back(status);
       
        position += 30;
    }
}

cJSON* MyFrame::getTaskStatus(cJSON* node, string dailyID)
{
    cJSON* child = node->child;
    while (child)
    {
        if (child->string == dailyID)
            return child;

        child = child->next;
    }
    return NULL;
}

void MyFrame::updateTaskStatus(cJSON* node, cJSON* status)
{
    if (status->type == cJSON_Number)
    {
        if (status->valueint == 0)
            cJSON_ReplaceItemInObject(node, status->string, cJSON_CreateString("N/A"));
        else if (status->valueint == 1)
            cJSON_ReplaceItemInObject(node, status->string, cJSON_CreateNumber(0));
    }
    else if (status->type == cJSON_String && strcmp(status->valuestring, "N/A") == 0)
        cJSON_ReplaceItemInObject(node, status->string, cJSON_CreateNumber(1));
}

void MyFrame::resetScrolledWindow(wxPanel* panel)
{
    if (scrolledWindow != NULL)
        scrolledWindow->Destroy();

    scrolledWindow = new wxScrolledWindow(panel, wxID_ANY, 
        wxPoint(0, 0), wxSize(375, 75), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

void MyFrame::showErrorMessage(wxString errorMessage)
{
    wxStaticText* errorMessageLabel = new wxStaticText(scrolledWindow, wxID_ANY, 
        wxString::Format(errorMessage, filePath.c_str()), wxPoint(5, 5));
}