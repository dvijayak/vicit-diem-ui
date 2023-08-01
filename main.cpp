/////////////////////////////////////////////////////////////////////////////
// Name:        main.cpp
// Purpose:     UI for the vicit-diem task-tracking tool
/////////////////////////////////////////////////////////////////////////////

#include "wx/wxprec.h"
#include "wx/button.h"
#include "wx/calctrl.h"
#include "wx/checkbox.h"
#include "wx/filedlg.h"
#include "wx/filename.h"
#include "wx/notebook.h"
#include "wx/scrolwin.h"
#include "wx/stattext.h"
#include "wx/stdpaths.h"
#include "wx/textctrl.h"
#include "wx/window.h"
#include "../cJSON/cJSON.h"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
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

    private:
        wxDECLARE_EVENT_TABLE();

        /////////////////////////////////////////////////////////////////////////////
        // functions
        /////////////////////////////////////////////////////////////////////////////

        void OnOpenFileDialogButton(wxCommandEvent& event);
        void OnLoadJSONFileButton(wxCommandEvent& event);
        void OnTrackingCalendar(wxCalendarEvent& event);
        void OnUpdateTaskStatusCheckbox(wxCommandEvent& event);
        void OnAddTaskButton(wxCommandEvent& event);
        void OnEditTaskButton(wxCommandEvent& event);
        void OnDeleteTaskButton(wxCommandEvent& event);
        void OnSelectAssignedDaysCheckbox(wxCommandEvent& event);
        void OnCancelEditsButton(wxCommandEvent& event);
        void OnApplyEditsButton(wxCommandEvent& event);
        void OnSaveChangesButton(wxCommandEvent& event);

        void setupTrackingPage();
        void setupSchedulePage();
        void setupTaskEditorPage(wxButton* editButton);
        void createScrolledTrackingWindow();
        void createScrolledScheduleWindow();
        void createScrolledTaskEditorWindow();
        void loadAndDisplayTaskDataToTrackingWindow();
        void loadAndDisplayTaskDataToScheduleWindow();
        void showTasksOnTrackingWindow();
        void showTasksOnScheduleWindow();
        void showAvailableTaskDaysOnTaskEditorWindow();
        void showErrorMessageOnTrackingWindow(wxString errorMessage);
        void showErrorMessageOnScheduleWindow(wxString errorMessage);

        string getUserDataDir();
        void saveFilePath();
        string loadFilePath();

        string getUserName(cJSON* node);
        cJSON* getTaskDatabaseForSelectedDate(cJSON* node);
        string getNextTaskDatabaseDate(cJSON* node);
        cJSON* getTaskDatabaseForEffectiveDate(cJSON* node);
        void getTitlesForSelectedDate(cJSON* node);
        void getTitlesForEffectiveDate(cJSON* node);
        void getDailyIDsForSelectedDate(cJSON* node);
        bool isAssignedDay(cJSON* node);
        bool matchesAssignedDay(cJSON* node);
        cJSON* getDailyRecord(cJSON* node);
        cJSON* getEntryFromDailyRecord(cJSON* node);
        cJSON* getTaskStatus(cJSON* node, string dailyID);

        void addToTaskDatabase(cJSON* node, string title);
        string generateRandomDailyID();
        void deleteFromTaskDatabase(cJSON* node, int element);
        void addEntryToDailyRecord(cJSON* node);
        void addMissingTasksToDailyRecordEntry(cJSON* node);
        void deleteExtraTasksFromDailyRecordEntry(cJSON* node);
        void updateTaskStatus(cJSON* node, string dailyID);
        void applyEditsToTaskDatabase();

        /////////////////////////////////////////////////////////////////////////////
        // variables
        /////////////////////////////////////////////////////////////////////////////

        wxTextCtrl* JSONFilePathField = NULL; 
        wxTextCtrl* taskEntryField = NULL;

        wxNotebook* notebook = NULL;
        wxWindow* trackingWindow = NULL;
        wxWindow* scheduleWindow = NULL;
        wxScrolledWindow* scrolledTrackingWindow = NULL;
        wxScrolledWindow* scrolledScheduleWindow = NULL;
        wxScrolledWindow* scrolledTaskEditorWindow = NULL;
        wxCalendarCtrl* trackingCalendar = NULL;
        wxCalendarCtrl* scheduleCalendar = NULL;

        wxString userName = wxEmptyString;
        wxDateTime selectedDate = NULL;
        wxDateTime effectiveDate = NULL;
        wxStaticText* dateLabel = NULL;

        string filePath;
        cJSON* root = NULL;
        cJSON* taskDatabase = NULL;
        int whichTask = 0;
        cJSON* dailyRecord = NULL;
        cJSON* record = NULL;

        vector<string> titlesForSelectedDate;
        vector<string> dailyIDs;
        vector<wxCheckBox*> taskCheckboxes;

        vector<string> titlesForEffectiveDate;
        vector<wxButton*> editButtons;
        vector<wxButton*> deleteButtons;
        vector<string> selectedDays;
};

enum
{
    wxID_LOAD = 1001,
    wxID_CHOOSE = 1002,
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_BUTTON(wxID_OPEN, MyFrame::OnOpenFileDialogButton)
    EVT_BUTTON(wxID_LOAD, MyFrame::OnLoadJSONFileButton)
    EVT_CALENDAR_SEL_CHANGED(wxID_ANY, MyFrame::OnTrackingCalendar)
    EVT_CHECKBOX(wxEVT_CHECKBOX, MyFrame::OnUpdateTaskStatusCheckbox)
    EVT_BUTTON(wxID_ADD, MyFrame::OnAddTaskButton)
    EVT_BUTTON(wxID_EDIT, MyFrame::OnEditTaskButton)
    EVT_BUTTON(wxID_DELETE, MyFrame::OnDeleteTaskButton)
    EVT_BUTTON(wxID_CANCEL, MyFrame::OnCancelEditsButton)
    EVT_BUTTON(wxID_APPLY, MyFrame::OnApplyEditsButton)
    EVT_CHECKBOX(wxID_CHOOSE, MyFrame::OnSelectAssignedDaysCheckbox)
    EVT_BUTTON(wxID_SAVE, MyFrame::OnSaveChangesButton)
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
       : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1000, 500))
{
    notebook = new wxNotebook(this, wxID_ANY, wxPoint(100, 75), wxSize(800, 350));

    trackingWindow = new wxWindow(notebook, wxID_ANY);
    scheduleWindow = new wxWindow(notebook, wxID_ANY);

    notebook->AddPage(trackingWindow, "Tracking");
    notebook->AddPage(scheduleWindow, "Schedule");

    wxFont boldFont(wxFontInfo().Bold());

    wxStaticText* enterFilePathLabel = new wxStaticText(this, wxID_ANY,
        wxT("Enter path to JSON file"), wxPoint(125, 15), wxDefaultSize, wxALIGN_LEFT);

    enterFilePathLabel->SetFont(boldFont);

    JSONFilePathField = new wxTextCtrl(this, wxID_ANY,
        wxEmptyString, wxPoint(275, 10), wxSize(400, -1));

    string path = loadFilePath();
    JSONFilePathField->SetValue(path);

    wxButton* openFileDialogButton = new wxButton(this, wxID_OPEN,
        wxT("Open"), wxPoint(700, 10), wxDefaultSize);

    wxButton* loadJSONFileButton = new wxButton(this, wxID_LOAD,
        wxT("Load"), wxPoint(800, 10), wxDefaultSize);

    setupTrackingPage();
    setupSchedulePage();
}

/////////////////////////////////////////////////////////////////////////////
// event handlers
/////////////////////////////////////////////////////////////////////////////

void MyFrame::OnLoadJSONFileButton(wxCommandEvent& event)
{   
    createScrolledTrackingWindow();
    createScrolledScheduleWindow();

    wxString text = JSONFilePathField->GetValue();
    filePath = text.ToStdString();

    ifstream fin;
    fin.open(filePath);

    if (fin)
    {
        string line, file; 

        while (getline(fin, line))
            file += line; 
        
        root = cJSON_Parse(file.c_str());

        loadAndDisplayTaskDataToTrackingWindow();
        loadAndDisplayTaskDataToScheduleWindow();
    }
    else
    {
        wxString errorMessage = "File \"%s\" was not found";
        showErrorMessageOnTrackingWindow(errorMessage);
        showErrorMessageOnScheduleWindow(errorMessage);
    }
    fin.close();
}

void MyFrame::OnOpenFileDialogButton(wxCommandEvent& event)
{
    wxFileDialog dialog(this);

    if (dialog.ShowModal() == wxID_OK) 
    {
        wxString path = dialog.GetPath();
        JSONFilePathField->SetValue(path);
    }
}

void MyFrame::OnTrackingCalendar(wxCalendarEvent& event)
{
    createScrolledTrackingWindow();

    if (root)
        loadAndDisplayTaskDataToTrackingWindow();
    else
    {
        wxString errorMessage = "You must load a file";
        showErrorMessageOnTrackingWindow(errorMessage);
    }
}

void MyFrame::OnUpdateTaskStatusCheckbox(wxCommandEvent& event)
{
    wxCheckBox* checkbox = (wxCheckBox*) event.GetEventObject();

    for (int count = 0; count < taskCheckboxes.size(); count++)
    {
        if (checkbox == taskCheckboxes[count])
            updateTaskStatus(record, dailyIDs[count]);
    }
}

void MyFrame::OnAddTaskButton (wxCommandEvent& event)
{
    int taskNumber = titlesForEffectiveDate.size() + 1;
    string title = "Task " + to_string(taskNumber);

    addToTaskDatabase(taskDatabase, title);
    titlesForEffectiveDate.push_back(title);
    
    createScrolledScheduleWindow();
    showTasksOnScheduleWindow();
}

void MyFrame::OnEditTaskButton(wxCommandEvent& event)
{
    wxButton* editButton = (wxButton*) event.GetEventObject();

    for (int count = 0; count < editButtons.size(); count++)
    {
        if (editButton == editButtons[count])
            whichTask = count;
    }
    setupTaskEditorPage(editButton);
}

void MyFrame::OnDeleteTaskButton(wxCommandEvent& event)
{
    wxButton* deleteButton = (wxButton*) event.GetEventObject();

    for (int count = 0; count < deleteButtons.size(); count++)
    {
        if (deleteButton == deleteButtons[count])
        {
            deleteFromTaskDatabase(taskDatabase, count);

            titlesForEffectiveDate.erase(titlesForEffectiveDate.begin() + count);
            editButtons.erase(editButtons.begin() + count);
            deleteButtons.erase(deleteButtons.begin() + count);
        }
    }
    createScrolledScheduleWindow();
    showTasksOnScheduleWindow();
}

void MyFrame::OnSelectAssignedDaysCheckbox(wxCommandEvent& event)
{
    wxCheckBox* checkbox = (wxCheckBox*) event.GetEventObject();
    wxString label = checkbox->GetLabel();
    selectedDays.push_back(label.ToStdString());
}

void MyFrame::OnCancelEditsButton(wxCommandEvent& event)
{
    scheduleWindow->DestroyChildren();
    setupSchedulePage();
    loadAndDisplayTaskDataToScheduleWindow();
}

void MyFrame::OnApplyEditsButton(wxCommandEvent& event)
{
    applyEditsToTaskDatabase();
    scheduleWindow->DestroyChildren();
    setupSchedulePage();
    loadAndDisplayTaskDataToScheduleWindow();
}

void MyFrame::OnSaveChangesButton(wxCommandEvent& event)
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
            createScrolledTrackingWindow();

            wxString errorMessage = "File \"%s\" could not be saved because it does not exist";
            showErrorMessageOnTrackingWindow(errorMessage);
        }
        fin.close();
    }
    else
    {
        createScrolledTrackingWindow();

        wxString errorMessage = "You must load a file before it can be saved";
        showErrorMessageOnTrackingWindow(errorMessage);
    }
}

/////////////////////////////////////////////////////////////////////////////
// UI setup and display functions
/////////////////////////////////////////////////////////////////////////////

void MyFrame::setupTrackingPage()
{
    trackingCalendar = new wxCalendarCtrl(trackingWindow, wxID_ANY, 
        wxDefaultDateTime, wxPoint(560, 50));
        
    createScrolledTrackingWindow();

    wxButton* saveStatusButton = new wxButton(trackingWindow, wxID_SAVE, 
        wxT("Save"), wxPoint(350, 275), wxDefaultSize);
}

void MyFrame::setupSchedulePage()
{
    scheduleCalendar = new wxCalendarCtrl(scheduleWindow, wxID_ANY, 
        wxDefaultDateTime, wxPoint(20, 50));

    createScrolledScheduleWindow();

    wxButton* addTaskButton = new wxButton(scheduleWindow, wxID_ADD, wxT("Add"), wxPoint(350, 250));
    wxButton* restoreButton = new wxButton(scheduleWindow, wxID_LOAD, wxT("Restore"), wxPoint(300, 275));
    wxButton* saveButton = new wxButton(scheduleWindow, wxID_SAVE, wxT("Save"), wxPoint(400, 275));
}

void MyFrame::setupTaskEditorPage(wxButton* editButton)
{
    scheduleWindow->DestroyChildren();

    for (int count = 0; count < editButtons.size(); count++)
    {
        if (editButton == editButtons[count])
        {
            wxString task(titlesForEffectiveDate[count]);
            wxStaticText* editTaskLabel = new wxStaticText(scheduleWindow, wxID_ANY, 
                (wxT("Editing \"") + task + wxT("\"")), wxPoint(250, 25));

            wxFont boldFont(wxFontInfo(16).Bold());
            editTaskLabel->SetFont(boldFont);
        }
    }
    taskEntryField = new wxTextCtrl(scheduleWindow, wxID_ANY, 
        wxEmptyString, wxPoint(200, 50), wxSize(400, -1));

    createScrolledTaskEditorWindow();
    showAvailableTaskDaysOnTaskEditorWindow();

    wxButton* cancelButton = new wxButton(scheduleWindow, wxID_CANCEL, wxT("Cancel"), wxPoint(300, 275));
    wxButton* applyButton = new wxButton(scheduleWindow, wxID_APPLY, wxT("Apply"), wxPoint(400, 275));
}

void MyFrame::createScrolledTrackingWindow()
{
    if (scrolledTrackingWindow != NULL)
        scrolledTrackingWindow->DestroyChildren();

    scrolledTrackingWindow = new wxScrolledWindow(trackingWindow, wxID_ANY, 
        wxPoint(20, 50), wxSize(525, 150), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledTrackingWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

void MyFrame::createScrolledScheduleWindow()
{
    if (scrolledScheduleWindow != NULL)
        scrolledScheduleWindow->DestroyChildren();

    scrolledScheduleWindow = new wxScrolledWindow(scheduleWindow, wxID_ANY, 
        wxPoint(255, 50), wxSize(525, 150), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledScheduleWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

void MyFrame::createScrolledTaskEditorWindow()
{
   scrolledTaskEditorWindow = new wxScrolledWindow(scheduleWindow, wxID_ANY, 
        wxPoint(250, 100), wxSize(300, 150), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledTaskEditorWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

void MyFrame::loadAndDisplayTaskDataToTrackingWindow()
{
    selectedDate = trackingCalendar->GetDate();

    if (userName.IsEmpty())
    {
        wxFont boldFont(wxFontInfo().Bold());

        userName = getUserName(root);

        wxStaticText* userNameLabel = new wxStaticText(trackingWindow, wxID_ANY,
            userName, wxPoint(20, 10), wxDefaultSize);

        userNameLabel->SetFont(boldFont);
    }
    titlesForSelectedDate.clear();
    dailyIDs.clear();
    taskCheckboxes.clear();

    cJSON* taskDatabase = getTaskDatabaseForSelectedDate(root);
    dailyRecord = getDailyRecord(root);

    getTitlesForSelectedDate(taskDatabase);
    getDailyIDsForSelectedDate(taskDatabase);

    if (!getEntryFromDailyRecord(root))
        addEntryToDailyRecord(dailyRecord);

    record = getEntryFromDailyRecord(root);

    addMissingTasksToDailyRecordEntry(record);
    deleteExtraTasksFromDailyRecordEntry(record);
    showTasksOnTrackingWindow();

    saveFilePath();
}

void MyFrame::loadAndDisplayTaskDataToScheduleWindow()
{
    effectiveDate = scheduleCalendar->GetDate();

    titlesForEffectiveDate.clear();
    editButtons.clear();
    deleteButtons.clear();

    taskDatabase = getTaskDatabaseForEffectiveDate(root);
    getTitlesForEffectiveDate(taskDatabase);

    showTasksOnScheduleWindow();
}

void MyFrame::showTasksOnTrackingWindow()
{
    int position = 0;

    for (int count = 0; count < titlesForSelectedDate.size(); count++)
    {
        wxString title(titlesForSelectedDate[count]);
        
        cJSON* status = getTaskStatus(record, dailyIDs[count]);
        
        wxCheckBox* checkbox = new wxCheckBox(scrolledTrackingWindow, wxEVT_CHECKBOX,
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
        taskCheckboxes.push_back(checkbox);
       
        position += 30;
    }
}

void MyFrame::showTasksOnScheduleWindow()
{
    editButtons.clear();
    deleteButtons.clear();

    int position = 0; 

    for (int count = 0; count < titlesForEffectiveDate.size(); count++)
    {
        wxString title(titlesForEffectiveDate[count]);

        wxStaticText* taskLabel = new wxStaticText(scrolledScheduleWindow, wxID_ANY, 
            title, wxPoint(0, position));

        wxButton* editButton = new wxButton(scrolledScheduleWindow, wxID_EDIT, 
            wxT("Edit"), wxPoint(300, position));

        wxButton* deleteButton = new wxButton(scrolledScheduleWindow, wxID_DELETE, 
            wxT("Delete"), wxPoint(400, position));

        editButtons.push_back(editButton);
        deleteButtons.push_back(deleteButton);

        position += 30;
    }
}

void MyFrame::showAvailableTaskDaysOnTaskEditorWindow()
{
    vector<wxString> availableTaskDays = {"Everyday", "Weekdays", "Weekends",
                                            "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
                                            "Saturday", "Sunday"};

    int position = 0;

    for (int count = 0; count < availableTaskDays.size(); count++)
    {
        wxCheckBox* checkbox = new wxCheckBox(scrolledTaskEditorWindow, wxID_CHOOSE, 
            availableTaskDays[count], wxPoint(0, position));
        
        position += 30;
    }
    int windowHeight = position;
    scrolledTaskEditorWindow->SetVirtualSize(wxSize(-1, windowHeight));
    scrolledTaskEditorWindow->SetScrollRate(0, 10);
}

void MyFrame::showErrorMessageOnTrackingWindow(wxString errorMessage)
{
    wxStaticText* errorMessageLabel = new wxStaticText(scrolledTrackingWindow, wxID_ANY, 
        wxString::Format(errorMessage, filePath.c_str()), wxPoint(5, 5));
}

void MyFrame::showErrorMessageOnScheduleWindow(wxString errorMessage)
{
    wxStaticText* errorMessageLabel = new wxStaticText(scrolledScheduleWindow, wxID_ANY, 
        wxString::Format(errorMessage, filePath.c_str()), wxPoint(5, 5));
}

/////////////////////////////////////////////////////////////////////////////
// file I/O functions
/////////////////////////////////////////////////////////////////////////////

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

void MyFrame::saveFilePath()
{
    string path = getUserDataDir();

    ofstream fout;
    fout.open(path);

    fout << filePath;
}

string MyFrame::loadFilePath()
{
    string path = getUserDataDir();
    string filePath;

    ifstream fin;
    fin.open(path);

    fin >> filePath;
    return filePath;
}

/////////////////////////////////////////////////////////////////////////////
// JSON parsing functions
/////////////////////////////////////////////////////////////////////////////

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

cJSON* MyFrame::getTaskDatabaseForSelectedDate(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (getTaskDatabaseForSelectedDate(child))
                return getTaskDatabaseForSelectedDate(child);

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
                    beforeDate.ParseDate(getNextTaskDatabaseDate(node->next));
            
                    if (selectedDate.IsSameDate(afterDate) || selectedDate.IsStrictlyBetween(afterDate, beforeDate))
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

                    if (selectedDate.IsSameDate(afterDate) || selectedDate.IsLaterThan(afterDate))
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
            else if (getTaskDatabaseForSelectedDate(child))
                return getTaskDatabaseForSelectedDate(child);
            
            child = child->next;
        }
    }
    return NULL;
} 

string MyFrame::getNextTaskDatabaseDate(cJSON* node)
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

cJSON* MyFrame::getTaskDatabaseForEffectiveDate(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (getTaskDatabaseForEffectiveDate(child))
                return getTaskDatabaseForEffectiveDate(child);

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
                wxDateTime date = NULL;
                date.ParseDate(child->valuestring);

                if (effectiveDate.IsSameDate(date))
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
            else if (getTaskDatabaseForEffectiveDate(child))
                return getTaskDatabaseForEffectiveDate(child);
            
            child = child->next;
        }
    }
    return NULL;
}

void MyFrame::getTitlesForSelectedDate(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getTitlesForSelectedDate(child);
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
                            titlesForSelectedDate.push_back(child->valuestring);
                        
                        child = child->next;
                    }
                }
            }
            child = child->next;
        }
    }
}

void MyFrame::getTitlesForEffectiveDate(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getTitlesForEffectiveDate(child);
            child = child->next;
        }
    }
    else if (node->type == cJSON_Object)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (strcmp(child->string, "title") == 0)
            {
                titlesForEffectiveDate.push_back(child->valuestring);
            }
            child = child->next;
        }
    }
}

void MyFrame::getDailyIDsForSelectedDate(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getDailyIDsForSelectedDate(child);
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
            if (matchesAssignedDay(child))
                return true;

            child = child->next;
        }
    }
    else
    {
        if (matchesAssignedDay(node))
            return true;
    }
    return false;
}

bool MyFrame::matchesAssignedDay(cJSON* node)
{
    wxString dateName = selectedDate.GetWeekDayName(selectedDate.GetWeekDay(), selectedDate.Name_Full); 

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

                if (selectedDate.IsSameDate(timestamp))
                    return grandChild;
                
                grandChild = grandChild->next;
            }
        }
        child = child->next;
    }
    return NULL;
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

/////////////////////////////////////////////////////////////////////////////
// JSON manipulation functions
/////////////////////////////////////////////////////////////////////////////

void MyFrame::addToTaskDatabase(cJSON* node, string title)
{
    cJSON* task = cJSON_CreateObject();
    string randomDailyID = generateRandomDailyID();

    cJSON_AddItemToObject(task, "title", cJSON_CreateString(title.c_str()));
    cJSON_AddItemToObject(task, "daily_id", cJSON_CreateString(randomDailyID.c_str()));

    cJSON_AddItemToArray(node, task);
}

string MyFrame::generateRandomDailyID()
{
    srand(time(nullptr));

    string randomDailyID;
    vector<string> alphabet = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", 
                                "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z"};

    for (int count = 0; count < 9; count++)
    {
        string letter = alphabet[(rand() % alphabet.size())];
        randomDailyID += letter;
    }
    return randomDailyID;
}

void MyFrame::deleteFromTaskDatabase(cJSON* node, int element)
{
    cJSON_DeleteItemFromArray(node, element);
}

void MyFrame::addEntryToDailyRecord(cJSON* node)
{
    cJSON* record = cJSON_CreateObject();

    for (int count = 0; count < dailyIDs.size(); count++)
        cJSON_AddItemToObject(record, dailyIDs[count].c_str(), cJSON_CreateNumber(0));

    cJSON_ReplaceItemInObject(record, selectedDate.Format("%m/%d/%Y"), record);

    record->next = node->child;
    node->child = record;
}

void MyFrame::addMissingTasksToDailyRecordEntry(cJSON* node)
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

void MyFrame::deleteExtraTasksFromDailyRecordEntry(cJSON* node)
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

void MyFrame::updateTaskStatus(cJSON* node, string dailyID)
{
    cJSON* child = node->child;
    while (child)
    {
        if (child->string == dailyID)
        {
            if (child->type == cJSON_Number)
            {
                if (child->valueint == 0)
                    cJSON_ReplaceItemInObject(node, child->string, cJSON_CreateString("N/A"));
                else if (child->valueint == 1)
                    cJSON_ReplaceItemInObject(node, child->string, cJSON_CreateNumber(0));
            }
            else if (child->type == cJSON_String)
            {
                if (strcmp(child->valuestring, "N/A") == 0)
                    cJSON_ReplaceItemInObject(node, child->string, cJSON_CreateNumber(1));
            }
        }
        child = child->next;
    }
}

void MyFrame::applyEditsToTaskDatabase()
{
    wxString title = taskEntryField->GetValue();
    cJSON* task = cJSON_GetArrayItem(taskDatabase, whichTask);
    cJSON_ReplaceItemInObject(task, "title", cJSON_CreateString(title));

    if (selectedDays.size() == 1)
    {
        string assignedDays = selectedDays.front();
        cJSON_ReplaceItemInObject(task, "assigned_days", cJSON_CreateString(assignedDays.c_str()));
    }
    else if (selectedDays.size() > 1)
    {
        cJSON* assignedDays = cJSON_CreateArray();
        for (int count = 0; count < selectedDays.size(); count++)
            cJSON_AddItemToArray(assignedDays, cJSON_CreateString(selectedDays[count].c_str()));

        cJSON_ReplaceItemInObject(task, "assigned_days", assignedDays);
    }
}