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

        string getName(cJSON* node);
        cJSON* getTaskDatabase(cJSON* node);
        string getNextDate(cJSON* node);

        void getTitles(cJSON* node);
        void getDailyIDs(cJSON* node);
        bool isAssignedDay(cJSON* node);

        void showTasks(vector<string>titles);
        cJSON* checkStatus(cJSON* node, string dailyID);
        void updateStatus(string dailyID);

    private:
        wxDECLARE_EVENT_TABLE();

        wxTextCtrl* textField;
        wxPanel* panel;
        wxScrolledWindow* scrolledWindow;
        
        wxDateTime today = wxDateTime::Today();
        string filePath;
        cJSON* root;

        vector<string> titles;
        vector<string> dailyIDs;
        vector<wxCheckBox*> checkboxList;
        vector<cJSON*> statusSet;
};

enum
{
    Minimal_Quit = wxID_EXIT,
    Minimal_About = wxID_ABOUT
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(wxEVT_BUTTON, MyFrame::OnLoadButton)
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

MyFrame::MyFrame(const wxString& title)
       : wxFrame(NULL, wxID_ANY, title)
{
    wxStaticText* label = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"), wxPoint(10, 15), wxDefaultSize, wxALIGN_LEFT);

    textField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(160, 10), wxSize(125, 20));
    
    textField->SetValue("/Users/joshuaparfitt/Development/many_schemas.json");

    wxButton* loadButton = new wxButton(this, wxEVT_BUTTON, 
        wxT("Load"), wxPoint(300, 10), wxDefaultSize);

    wxString day = today.GetWeekDayName(today.GetWeekDay(), today.Name_Full); 
    wxString date = today.FormatDate();

    wxStaticText* dayLabel = new wxStaticText(this, wxID_ANY,
        day, wxPoint(170, 40), wxDefaultSize);

    wxStaticText* dateLabel = new wxStaticText(this, wxID_ANY,
        date, wxPoint(230, 40), wxDefaultSize);

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
}

void MyFrame::OnLoadButton(wxCommandEvent& event)
{
    static int count = 0;

    wxString textEntry = textField->GetValue();
    filePath = textEntry.ToStdString();

    ifstream fin;
    fin.open(filePath);

    if (fin)
    {
        string line, file; 

        while (getline(fin, line))
            file += line; 
        
        root = cJSON_Parse(file.c_str());

        if (count == 0)
        {   
            wxString name(getName(root));

            wxStaticText* nameLabel = new wxStaticText(this, wxID_ANY,
                name, wxPoint(100, 40), wxDefaultSize);
        }
        else
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

            titles.clear();
            dailyIDs.clear();
            checkboxList.clear();
            statusSet.clear();
        }
        cJSON* taskDatabase = getTaskDatabase(root);

        getTitles(taskDatabase);
        getDailyIDs(taskDatabase);

        showTasks(titles);

        count++;
    }
    else
    {
        wxStaticText* errorMessage = new wxStaticText(scrolledWindow, wxID_ANY, 
            wxString::Format(wxT("File \"%s\" was not found"), filePath.c_str()), wxPoint(5, 5));
    }
    fin.close();
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

void MyFrame::getTitles(cJSON* node)
{
    string title = "title";
    string assignedDays = "assigned_days";

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getTitles(child);
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

void MyFrame::getDailyIDs(cJSON* node)
{
    string dailyID = "daily_id";
    string assignedDays = "assigned_days";

    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getDailyIDs(child);
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

void MyFrame::showTasks(vector<string> titles)
{
    int position = 0;

    for (int count = 0; count < titles.size(); count++)
    {
        wxString title(titles[count]);
        
        cJSON* status = checkStatus(root, dailyIDs[count]);

        if (status)
        {
            wxCheckBox* checkbox = new wxCheckBox(scrolledWindow, wxEVT_CHECKBOX,
                title, wxPoint(0, position), wxDefaultSize);

            if (status->valueint == 0)
                checkbox->SetValue(false);
            else if (status->valueint == 1)
                checkbox->SetValue(true);
        
            checkboxList.push_back(checkbox);
            statusSet.push_back(status);
        }
        position += 30;
    }
}

cJSON* MyFrame::checkStatus(cJSON* node, string dailyID)
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
                {
                    cJSON* greatGrandChild = grandChild->child;
                    while (greatGrandChild)
                    {
                        if (greatGrandChild->string == dailyID)
                            return greatGrandChild;

                        greatGrandChild = greatGrandChild->next;
                    }
                }
                grandChild = grandChild->next;
            }
        }
        child = child->next;
    }
    return NULL;
}

void MyFrame::OnCheckbox(wxCommandEvent& event)
{
    wxCheckBox* checkbox = (wxCheckBox*) event.GetEventObject();

    for (int count = 0; count < checkboxList.size(); count++)
    {
        if (checkbox == checkboxList[count])
            updateStatus(dailyIDs[count]);
    }
}

void MyFrame::updateStatus(string dailyID)
{
    for (int count = 0; count < dailyIDs.size(); count++)
    {
        if (dailyID == dailyIDs[count])
        {
            if (statusSet[count]->valueint == 0)
                cJSON_SetNumberValue(statusSet[count], 1);
            else if (statusSet[count]->valueint == 1)
                cJSON_SetNumberValue(statusSet[count], 0);
        }
    }
}

void MyFrame::OnSaveButton(wxCommandEvent& event)
{
    ofstream fout;
    fout.open(filePath);
    
    char* file = cJSON_Print(root);
    fout << file;

    fout.close();
}