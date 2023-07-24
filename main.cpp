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
#include <wx/datectrl.h>
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

        void paintWindow(wxPanel* panel);
        void saveFilePath(string filePath);
        string loadFilePath();

        string getName(cJSON* node);
        cJSON* getTaskDatabase(cJSON* node);
        string getNextDate(cJSON* node);
        cJSON* getRecordAccount(cJSON* node);

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
        wxDatePickerCtrl* datePicker;
        wxPanel* panel;
        wxScrolledWindow* scrolledWindow;
        
        wxDateTime date;
        string filePath;
        cJSON* root = NULL;
        cJSON* recordAccount;

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
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(650, 250))
{
    wxStaticText* label = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"), wxPoint(25, 15), wxDefaultSize, wxALIGN_LEFT);

    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(175, 10), wxSize(125, 20));
    
    string path = loadFilePath();
    textField->SetValue(path);

    wxButton* loadButton = new wxButton(this, wxEVT_BUTTON, 
        wxT("Load"), wxPoint(325, 10), wxDefaultSize);

    wxButton* openButton = new wxButton(this, wxID_OPEN,
        wxT("Open"), wxPoint(425, 10), wxDefaultSize);
    
    datePicker = new wxDatePickerCtrl(this, wxID_APPLY, 
        wxDefaultDateTime, wxPoint(525, 10));

    wxDateTime today = wxDateTime::Today();

    wxString todaysName = today.GetWeekDayName(today.GetWeekDay(), today.Name_Full); 
    wxString todaysDate = today.FormatDate();

    wxStaticText* dateLabel = new wxStaticText(this, wxID_ANY,
        (todaysName + wxT("\t") + todaysDate), wxPoint(350, 40), wxDefaultSize);

    panel = new wxPanel(this, wxID_ANY,
        wxPoint(125, 75), wxSize(375, 75));
        
    paintWindow(panel);

    wxButton* saveButton = new wxButton(this, wxID_SAVE, 
        wxT("Save"), wxPoint(275, 175), wxDefaultSize);
}

/////////////////////////////////////////////////////////////////////////////
// event handlers 
/////////////////////////////////////////////////////////////////////////////

void MyFrame::OnLoadButton(wxCommandEvent& event)
{
    date = datePicker->GetValue();

    wxString text = textField->GetValue();
    filePath = text.ToStdString();

    saveFilePath(filePath);

    ifstream fin;
    fin.open(filePath);

    scrolledWindow->Destroy();
    paintWindow(panel);

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
                name, wxPoint(125, 40), wxDefaultSize);
        }
        else
        {
            titles.clear();
            dailyIDs.clear();
            checkboxList.clear();
            statusSet.clear();
        }
        cJSON* taskDatabase = getTaskDatabase(root);
        recordAccount = getRecordAccount(root);

        getTitles(taskDatabase, titles);
        getDailyIDs(taskDatabase, dailyIDs);

        addTasksToRecord(recordAccount, dailyIDs);
        deleteTasksFromRecord(recordAccount, dailyIDs);
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
            updateStatus(recordAccount, statusSet[count]);
            statusSet[count] = getStatus(recordAccount, dailyIDs[count]);
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
            scrolledWindow->Destroy();
            paintWindow(panel);

            wxString errorMessage = wxString::Format(wxT("File \"%s\" could not be saved because it does not exist"), filePath.c_str());

            wxStaticText* errorMessageLabel = new wxStaticText(scrolledWindow, wxID_ANY, 
                errorMessage, wxPoint(5, 5));
        }
    }
    else
    {
        scrolledWindow->Destroy();
        paintWindow(panel);

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

void MyFrame::paintWindow(wxPanel* panel)
{
    scrolledWindow = new wxScrolledWindow(panel, wxID_ANY, 
        wxPoint(0, 0), wxSize(375, 75), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

void MyFrame::saveFilePath(string filePath)
{
    ofstream fout;
    fout.open("filePath.txt");
    fout << filePath;
}

string MyFrame::loadFilePath()
{
    string filePath;

    ifstream fin;
    fin.open("filePath.txt");

    fin >> filePath;
    return filePath;
}

string MyFrame::getName(cJSON* node)
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
                    beforeDate.ParseDate(getNextDate(node->next));
            
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

string MyFrame::getNextDate(cJSON* node)
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

cJSON* MyFrame::getRecordAccount(cJSON* node)
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
    wxString dateName = date.GetWeekDayName(date.GetWeekDay(), date.Name_Full); 

    vector<string> everyday = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
    vector<string> weekdays = {"Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    vector<string> weekends = {"Saturday", "Sunday"};

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (strcmp(child->valuestring, "Everyday") == 0)
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
            else if (strcmp(child->valuestring, "Weekdays") == 0)
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
            else if (strcmp(child->valuestring, "Weekends") == 0)
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
                day.ParseDate(child->valuestring);

                wxString dayName = day.GetWeekDayName(day.GetWeekDay(), day.Name_Full); 

                if (dateName == dayName)
                    return true;
            }
            child = child->next;
        }
    }
    else
    {
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
        
        cJSON* status = getStatus(recordAccount, dailyIDs[count]);
        
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