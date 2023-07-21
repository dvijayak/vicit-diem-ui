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
        void OnCheckbox(wxCommandEvent& event);
        void OnSaveButton(wxCommandEvent& event);
        void OnOpenFileDialogButton(wxCommandEvent& event);

        void clearWindow();

        string getName(cJSON* node);
        cJSON* getTaskDatabase(cJSON* node);
        string getNextDate(cJSON* node);
        cJSON* getTodaysRecord(cJSON* node);

        void getTitles(cJSON* node, vector<string>& titles);
        void getDailyIDs(cJSON* node, vector<string>& dailyIDs);
        bool isAssignedDay(cJSON* node);

        void addTasksToRecord(cJSON* node, vector<string> dailyIDs);
        void deleteTasksFromRecord(cJSON* node, vector<string> dailyIDs);
        void showTasks(vector<string> titles, vector<string> dailyIDs, vector<wxCheckBox*>& checkboxList, vector<cJSON*>& statusSet);

        cJSON* getStatus(cJSON* node, string dailyID);
        void updateStatus(cJSON* node, cJSON* status);

    private:
        wxDECLARE_EVENT_TABLE();

        wxTextCtrl* textField;
        wxPanel* panel;
        wxScrolledWindow* scrolledWindow;
        
        wxDateTime today = wxDateTime::Today();
        string filePath;
        cJSON* root = NULL;
        cJSON* todaysRecord;

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
       : wxFrame(NULL, wxID_ANY, title)
{
    wxBoxSizer* topLevel = new wxBoxSizer(wxVERTICAL);

    wxStaticText* label = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"), wxPoint(10, 15), wxDefaultSize, wxALIGN_LEFT);

    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(160, 10), wxSize(125, 20));
    
    textField->SetValue("/Users/joshuaparfitt/Development/many_schemas.json");

    wxButton* loadButton = new wxButton(this, wxEVT_BUTTON, 
        wxT("Load"), wxPoint(300, 10), wxDefaultSize);

    wxButton* openButton = new wxButton(this, wxID_OPEN,
        wxT("Open"), wxPoint(400, 10), wxDefaultSize);

    wxString day = today.GetWeekDayName(today.GetWeekDay(), today.Name_Full); 
    wxString date = today.FormatDate();

    wxStaticText* dateLabel = new wxStaticText(this, wxID_ANY,
        (day + wxT("\t") + date), wxPoint(180, 40), wxDefaultSize);

    panel = new wxPanel(this, wxID_ANY,
        wxPoint(20, 70), wxSize(360, 90));
        
    scrolledWindow = new wxScrolledWindow(panel, wxID_ANY, 
        wxPoint(0, 0), wxSize(360, 90), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY); 

    wxButton* saveButton = new wxButton(this, wxID_SAVE, 
        wxT("Save"), wxPoint(160, 180), wxDefaultSize);
    
    SetSizer(topLevel);
}

/////////////////////////////////////////////////////////////////////////////
// event handlers 
/////////////////////////////////////////////////////////////////////////////

void MyFrame::OnLoadButton(wxCommandEvent& event)
{
    wxString textEntry = textField->GetValue();
    filePath = textEntry.ToStdString();

    ifstream fin;
    fin.open(filePath);

    clearWindow();

    if (fin)
    {
        string line, file; 

        while (getline(fin, line))
            file += line; 
        
        root = cJSON_Parse(file.c_str());

        static int count = 0;

        if (count == 0)
        {   
            wxString name(getName(root));

            wxStaticText* nameLabel = new wxStaticText(this, wxID_ANY,
                name, wxPoint(90, 40), wxDefaultSize);
        }
        else
        {
            titles.clear();
            dailyIDs.clear();
            checkboxList.clear();
            statusSet.clear();
        }
        cJSON* taskDatabase = getTaskDatabase(root);
        todaysRecord = getTodaysRecord(root);

        getTitles(taskDatabase, titles);
        getDailyIDs(taskDatabase, dailyIDs);

        addTasksToRecord(todaysRecord, dailyIDs);
        deleteTasksFromRecord(todaysRecord, dailyIDs);
        showTasks(titles, dailyIDs, checkboxList, statusSet);

        count++;
    }
    else
    {
        wxStaticText* errorMessage = new wxStaticText(scrolledWindow, wxID_ANY, 
            wxString::Format(wxT("File \"%s\" was not found"), filePath.c_str()), wxPoint(5, 5));
    }
    fin.close();
}

void MyFrame::OnCheckbox(wxCommandEvent& event)
{
    wxCheckBox* checkbox = (wxCheckBox*) event.GetEventObject();

    for (int count = 0; count < checkboxList.size(); count++)
    {
        if (checkbox == checkboxList[count])
        {
            updateStatus(todaysRecord, statusSet[count]);
            statusSet[count] = getStatus(todaysRecord, dailyIDs[count]);
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
            clearWindow();

            wxString errorMessage = wxString::Format(wxT("File \"%s\" could not be saved because it does not exist"), filePath.c_str());

            wxStaticText* errorMessageLabel = new wxStaticText(scrolledWindow, wxID_ANY, 
                errorMessage, wxPoint(5, 5));
        }
    }
    else
    {
        clearWindow();

        wxString errorMessage("You must load a file before it can be saved");

        wxStaticText* errorMessageLabel = new wxStaticText(scrolledWindow, wxID_ANY, 
            errorMessage, wxPoint(5, 5));
    }
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

/////////////////////////////////////////////////////////////////////////////
// helper functions 
/////////////////////////////////////////////////////////////////////////////

void MyFrame::clearWindow()
{
    scrolledWindow->Destroy();

    scrolledWindow = new wxScrolledWindow(panel, wxID_ANY, 
        wxPoint(0, 0), wxSize(360, 90), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

string MyFrame::getName(cJSON* node)
{
    string name = "name";
    
    cJSON* child = node->child;
    while (child)
    {
        if (child->string == name)
            return child->valuestring;
        
        child = child->next;
    }
    return NULL;
}

cJSON* MyFrame::getTaskDatabase(cJSON* node)
{
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

                if (node->next)
                {
                    afterDate.ParseDate(child->valuestring);
                    beforeDate.ParseDate(getNextDate(node->next));
            
                    if (today.IsSameDate(afterDate) || today.IsStrictlyBetween(afterDate, beforeDate))
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
                else
                {
                    afterDate.ParseDate(child->valuestring);

                    if (today.IsSameDate(afterDate) || today.IsLaterThan(afterDate))
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
            }
            else if (getTaskDatabase(child))
                return getTaskDatabase(child);
            
            child = child->next;
        }
    }
    return NULL;
} 

string MyFrame::getNextDate(cJSON* node)
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

cJSON* MyFrame::getTodaysRecord(cJSON* node)
{
    string dailyRecord = "daily_record";

    cJSON* child = node->child;
    while (child)
    {
        if (child->string == dailyRecord)
        {
            cJSON* grandChild = child->child;
            while (grandChild)
            {
                wxDateTime date = NULL;
                date.ParseDate(grandChild->string);

                if (today.IsSameDate(date))
                    return grandChild;
                
                grandChild = grandChild->next;
            }
        }
        child = child->next;
    }
    return NULL;
}

void MyFrame::getTitles(cJSON* node, vector<string>& titles)
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

void MyFrame::getDailyIDs(cJSON* node, vector<string>& dailyIDs)
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

bool MyFrame::isAssignedDay(cJSON* node)
{
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

void MyFrame::addTasksToRecord(cJSON* node, vector<string> dailyIDs)
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

void MyFrame::deleteTasksFromRecord(cJSON* node, vector<string> dailyIDs)
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
        
        cJSON* status = getStatus(todaysRecord, dailyIDs[count]);
        
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

cJSON* MyFrame::getStatus(cJSON* node, string dailyID)
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

void MyFrame::updateStatus(cJSON* node, cJSON* status)
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