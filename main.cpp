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

        void bindEventHandlers();

        void OnOpenFileDialogButton(wxCommandEvent& event);
        void OnLoadJSONFileButton(wxCommandEvent& event);
        void OnCalendarCtrl(wxCalendarEvent& event);
        void OnUpdateTaskStatusCheckbox(wxCommandEvent& event);
        void OnAddTaskButton(wxCommandEvent& event);
        void OnEditTaskButton(wxCommandEvent& event);
        void OnDeleteTaskButton(wxCommandEvent& event);
        void OnSelectassignedTaskDaysCheckbox(wxCommandEvent& event);
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
        void showAvailableTaskDaysOnTaskEditorWindow(string title);
        void showErrorMessageOnTrackingWindow(wxString errorMessage);
        void showErrorMessageOnScheduleWindow(wxString errorMessage);
        void highlightEffectiveDates();

        string getUserDataDir();
        void saveFilePath();
        string loadFilePath();

        string getUserName(cJSON* node);
        cJSON* getTaskDatabaseRecords(cJSON* node);
        cJSON* getTaskDatabaseForSelectedDate(cJSON* node);
        string getNextTaskDatabaseDate(cJSON* node);
        cJSON* getEffectiveDateTaskDatabase(cJSON* node);
        void getEffectiveDates(cJSON* node);
        void getSelectedDateTitles(cJSON* node);
        void getEffectiveDateTitles(cJSON* node);
        void getSelectedDateDailyIDs(cJSON* node);
        cJSON* getAssignedTaskDays(cJSON* node, string title);
        bool isSelectedTaskDaySameAsAssignedTaskDay(cJSON* node);
        bool isAvailableTaskDaySameAsAssignedTaskDay(string availableTaskDay, cJSON* node);
        bool matchesAssignedDay(cJSON* node);
        cJSON* getDailyRecord(cJSON* node);
        cJSON* getEntryFromDailyRecord(cJSON* node);
        cJSON* getTaskStatus(cJSON* node, string dailyID);

        void addToTaskDatabaseRecords(cJSON* node);
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
        wxWindow* taskEditorWindow = NULL;
        wxScrolledWindow* scrolledTrackingWindow = NULL;
        wxScrolledWindow* scrolledScheduleWindow = NULL;
        wxScrolledWindow* scrolledTaskEditorWindow = NULL;
        wxCalendarCtrl* trackingCalendar = NULL;
        wxCalendarCtrl* scheduleCalendar = NULL;
        int nonEffectiveCalendarClicks = 0;

        wxString userName = wxEmptyString;
        wxDateTime selectedDate = NULL;
        wxDateTime effectiveDate = NULL;
        wxStaticText* dateLabel = NULL;

        string filePath;
        cJSON* root = NULL;
        cJSON* taskDatabaseRecords = NULL;
        cJSON* schema = NULL;
        cJSON* selectedDateTaskDatabase = NULL;
        int whichTask = 0;
        cJSON* dailyRecord = NULL;
        cJSON* record = NULL;

        vector<string> selectedDateTitles;
        vector<string> dailyIDs;
        vector<wxCheckBox*> taskCheckboxes;

        vector<wxDateTime> effectiveDates;
        vector<string> effectiveDateTitles;
        vector<wxButton*> editButtons;
        vector<wxButton*> deleteButtons;

        vector<string> selectedDays;
};

enum
{
    wxID_UPDATE = 1001,
    wxID_SELECT = 1002,
    wxID_CHANGE_SELECTED_DATE = 1003,
    wxID_CHANGE_EFFECTIVE_DATE = 1004,
    wxID_LOAD = 1005,
};

wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
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
    bindEventHandlers();

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

void MyFrame::bindEventHandlers()
{
    Bind(wxEVT_CALENDAR_SEL_CHANGED, bind(&MyFrame::OnCalendarCtrl, this, placeholders::_1));

    Bind(wxEVT_CHECKBOX, bind(&MyFrame::OnUpdateTaskStatusCheckbox, this, placeholders::_1), wxID_UPDATE);
    Bind(wxEVT_CHECKBOX, bind(&MyFrame::OnSelectassignedTaskDaysCheckbox, this, placeholders::_1), wxID_SELECT);

    Bind(wxEVT_BUTTON, bind(&MyFrame::OnOpenFileDialogButton, this, placeholders::_1), wxID_OPEN);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnLoadJSONFileButton, this, placeholders::_1), wxID_LOAD);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnAddTaskButton, this, placeholders::_1), wxID_ADD);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnEditTaskButton, this, placeholders::_1), wxID_EDIT);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnDeleteTaskButton, this, placeholders::_1), wxID_DELETE);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnCancelEditsButton, this, placeholders::_1), wxID_CANCEL);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnApplyEditsButton, this, placeholders::_1), wxID_OK);
    Bind(wxEVT_BUTTON, bind(&MyFrame::OnSaveChangesButton, this, placeholders::_1), wxID_SAVE);
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

void MyFrame::OnCalendarCtrl(wxCalendarEvent& event)
{
    createScrolledTrackingWindow();
    createScrolledScheduleWindow();

    if (root)
    {
        loadAndDisplayTaskDataToTrackingWindow();
        loadAndDisplayTaskDataToScheduleWindow();
    }
    else
    {
        wxString errorMessage = "You must load a file";
        showErrorMessageOnTrackingWindow(errorMessage);
        showErrorMessageOnScheduleWindow(errorMessage);
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
    int taskNumber = effectiveDateTitles.size() + 1;
    string title = "Task " + to_string(taskNumber);

    addToTaskDatabase(selectedDateTaskDatabase, title);
    effectiveDateTitles.push_back(title);
    
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
    scheduleWindow->Hide();
    setupTaskEditorPage(editButton);
}

void MyFrame::OnDeleteTaskButton(wxCommandEvent& event)
{
    wxButton* deleteButton = (wxButton*) event.GetEventObject();

    for (int count = 0; count < deleteButtons.size(); count++)
    {
        if (deleteButton == deleteButtons[count])
        {
            deleteFromTaskDatabase(selectedDateTaskDatabase, count);

            effectiveDateTitles.erase(effectiveDateTitles.begin() + count);
            editButtons.erase(editButtons.begin() + count);
            deleteButtons.erase(deleteButtons.begin() + count);
        }
    }
    createScrolledScheduleWindow();
    showTasksOnScheduleWindow();
}

void MyFrame::OnSelectassignedTaskDaysCheckbox(wxCommandEvent& event)
{
    wxCheckBox* checkbox = (wxCheckBox*) event.GetEventObject();
    wxString label = checkbox->GetLabel();
    selectedDays.push_back(label.ToStdString());
}

void MyFrame::OnCancelEditsButton(wxCommandEvent& event)
{
    taskEditorWindow->Destroy();
    scheduleWindow->Show();
}

void MyFrame::OnApplyEditsButton(wxCommandEvent& event)
{
    applyEditsToTaskDatabase();
    taskEditorWindow->Destroy();
    scheduleWindow->Show();
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
            if (schema != NULL)
                cJSON_AddItemToArray(taskDatabaseRecords, schema);

            ofstream fout;
            fout.open(filePath);

            char* file = cJSON_Print(root);
            fout << file;

            fout.close();

            loadAndDisplayTaskDataToTrackingWindow();
            loadAndDisplayTaskDataToScheduleWindow();
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
    trackingCalendar = new wxCalendarCtrl(trackingWindow, wxEVT_CALENDAR_SEL_CHANGED, 
        wxDefaultDateTime, wxPoint(560, 50), wxDefaultSize, NULL);
        
    createScrolledTrackingWindow();

    wxButton* saveStatusButton = new wxButton(trackingWindow, wxID_SAVE, 
        wxT("Save"), wxPoint(350, 275), wxDefaultSize);
}

void MyFrame::setupSchedulePage()
{
    scheduleCalendar = new wxCalendarCtrl(scheduleWindow, wxEVT_CALENDAR_SEL_CHANGED, 
        wxDefaultDateTime, wxPoint(20, 50), wxDefaultSize, NULL);

    createScrolledScheduleWindow();

    wxButton* addTaskButton = new wxButton(scheduleWindow, wxID_ADD, wxT("Add"), wxPoint(350, 250));
    wxButton* revertButton = new wxButton(scheduleWindow, wxID_LOAD, wxT("Revert"), wxPoint(300, 275));
    wxButton* saveButton = new wxButton(scheduleWindow, wxID_SAVE, wxT("Save"), wxPoint(400, 275));
}

void MyFrame::setupTaskEditorPage(wxButton* editButton)
{
    taskEditorWindow = new wxWindow(notebook, wxID_ANY, wxDefaultPosition, wxSize(800, 350));

    taskEntryField = new wxTextCtrl(taskEditorWindow, wxID_ANY, 
        wxEmptyString, wxPoint(200, 50), wxSize(400, -1));

    createScrolledTaskEditorWindow();

    for (int count = 0; count < editButtons.size(); count++)
    {
        if (editButton == editButtons[count])
        {
            wxString task(effectiveDateTitles[count]);
            wxStaticText* editTaskLabel = new wxStaticText(taskEditorWindow, wxID_ANY, 
                (wxT("Editing \"") + task + wxT("\"")), wxPoint(250, 25));

            wxFont boldFont(wxFontInfo(16).Bold());
            editTaskLabel->SetFont(boldFont);

            taskEntryField->SetValue(effectiveDateTitles[count]);

            showAvailableTaskDaysOnTaskEditorWindow(effectiveDateTitles[count]);
        }   
    }
    wxButton* cancelButton = new wxButton(taskEditorWindow, wxID_CANCEL, wxT("Cancel"), wxPoint(300, 275));
    wxButton* okButton = new wxButton(taskEditorWindow, wxID_OK, wxT("Ok"), wxPoint(400, 275));
}

void MyFrame::createScrolledTrackingWindow()
{
    if (scrolledTrackingWindow != NULL)
        scrolledTrackingWindow->DestroyChildren();

    scrolledTrackingWindow = new wxScrolledWindow(trackingWindow, wxID_ANY, 
        wxPoint(20, 50), wxSize(525, 140), wxVSCROLL);

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
        wxPoint(255, 50), wxSize(525, 140), wxVSCROLL);

    int pixelsPerUnitX = 0;
    int pixelsPerUnitY = 10;
    int noUnitsX = 0;
    int noUnitsY = 20;

    scrolledScheduleWindow->SetScrollbars(pixelsPerUnitX, pixelsPerUnitY,
        noUnitsX, noUnitsY);
}

void MyFrame::createScrolledTaskEditorWindow()
{
   scrolledTaskEditorWindow = new wxScrolledWindow(taskEditorWindow, wxID_ANY, 
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

    effectiveDates.clear();
    getEffectiveDates(root);

    if (selectedDate.IsSameDate(effectiveDates.front()) || selectedDate.IsLaterThan(effectiveDates.front()))
    {
        scrolledTrackingWindow->Destroy();
        createScrolledTrackingWindow();

        if (userName.IsEmpty())
        {
            wxFont boldFont(wxFontInfo().Bold());

            userName = getUserName(root);

            wxStaticText* userNameLabel = new wxStaticText(trackingWindow, wxID_ANY,
                userName, wxPoint(20, 10), wxDefaultSize);

            userNameLabel->SetFont(boldFont);
        }
        selectedDateTitles.clear();
        dailyIDs.clear();
        taskCheckboxes.clear();

        cJSON* taskDatabase = getTaskDatabaseForSelectedDate(root);
        dailyRecord = getDailyRecord(root);

        getSelectedDateTitles(taskDatabase);
        getSelectedDateDailyIDs(taskDatabase);

        if (!getEntryFromDailyRecord(root))
            addEntryToDailyRecord(dailyRecord);

        record = getEntryFromDailyRecord(root);

        addMissingTasksToDailyRecordEntry(record);
        deleteExtraTasksFromDailyRecordEntry(record);
        showTasksOnTrackingWindow();

        saveFilePath();
    }
}

void MyFrame::loadAndDisplayTaskDataToScheduleWindow()
{
    effectiveDate = scheduleCalendar->GetDate();

    effectiveDates.clear();
    getEffectiveDates(root);
    highlightEffectiveDates();

    bool isEffectiveDate = false;

    for (int count = 0; count < effectiveDates.size(); count++)
    {
        if (effectiveDate == effectiveDates[count])
            isEffectiveDate = true;
    }
    if (isEffectiveDate)
    {
        scrolledScheduleWindow->Destroy();
        createScrolledScheduleWindow();

        effectiveDateTitles.clear();
        editButtons.clear();
        deleteButtons.clear();

        selectedDateTaskDatabase = getEffectiveDateTaskDatabase(root);
        getEffectiveDateTitles(selectedDateTaskDatabase);

        showTasksOnScheduleWindow();
    }
    else
    {
        taskDatabaseRecords = getTaskDatabaseRecords(root);
        addToTaskDatabaseRecords(taskDatabaseRecords);
    }
}

void MyFrame::showTasksOnTrackingWindow()
{
    int position = 0;

    for (int count = 0; count < selectedDateTitles.size(); count++)
    {
        wxString title(selectedDateTitles[count]);
        
        cJSON* status = getTaskStatus(record, dailyIDs[count]);
        
        wxCheckBox* checkbox = new wxCheckBox(scrolledTrackingWindow, wxID_UPDATE,
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

    for (int count = 0; count < effectiveDateTitles.size(); count++)
    {
        wxString title(effectiveDateTitles[count]);

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

void MyFrame::showAvailableTaskDaysOnTaskEditorWindow(string title)
{
    vector<wxString> availableTaskDays = {"Everyday", "Weekdays", "Weekends",
        "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};

    cJSON* assignedTaskDays = getAssignedTaskDays(selectedDateTaskDatabase, title);

    int position = 0;

    for (int count = 0; count < availableTaskDays.size(); count++)
    {
        wxCheckBox* checkbox = new wxCheckBox(scrolledTaskEditorWindow, wxID_SELECT, 
            availableTaskDays[count], wxPoint(0, position));

        string availableTaskDay = availableTaskDays[count].ToStdString();
        if (isAvailableTaskDaySameAsAssignedTaskDay(availableTaskDay, assignedTaskDays))
            checkbox->SetValue(true);
    
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

void MyFrame::highlightEffectiveDates()
{
    int calendarMonth = effectiveDate.GetMonth();

    for (int count = 0; count < effectiveDates.size(); count++)
    {
        int effectiveDateMonth = effectiveDates[count].GetMonth();
        int effectiveDateDay = effectiveDates[count].GetDay();
        scheduleCalendar->ResetAttr(effectiveDateDay);
    }
    for (int count = 0; count < effectiveDates.size(); count++)
    {
        int effectiveDateMonth = effectiveDates[count].GetMonth();
        int effectiveDateDay = effectiveDates[count].GetDay();

        if (calendarMonth == effectiveDateMonth)
        {
            wxCalendarDateAttr* attr = new wxCalendarDateAttr;
            attr->SetTextColour(*wxRED);
            scheduleCalendar->SetAttr(effectiveDateDay, attr);
        }
    }
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

cJSON* MyFrame::getTaskDatabaseRecords(cJSON* node)
{
    cJSON* child = node->child;
    while (child)
    {
        if (strcmp(child->string, "taskdb_records") == 0)
            return child;
        
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

cJSON* MyFrame::getEffectiveDateTaskDatabase(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (getEffectiveDateTaskDatabase(child))
                return getEffectiveDateTaskDatabase(child);

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
            else if (getEffectiveDateTaskDatabase(child))
                return getEffectiveDateTaskDatabase(child);
            
            child = child->next;
        }
    }
    return NULL;
}

void MyFrame::getEffectiveDates(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getEffectiveDates(child);
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
                effectiveDates.push_back(date);
            }
            getEffectiveDates(child);
            child = child->next;
        }
    }
}

void MyFrame::getSelectedDateTitles(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getSelectedDateTitles(child);
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
                if (isSelectedTaskDaySameAsAssignedTaskDay(child))
                {
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (strcmp(child->string, "title") == 0)
                            selectedDateTitles.push_back(child->valuestring);
                        
                        child = child->next;
                    }
                }
            }
            child = child->next;
        }
    }
}

void MyFrame::getEffectiveDateTitles(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getEffectiveDateTitles(child);
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
                effectiveDateTitles.push_back(child->valuestring);
            }
            child = child->next;
        }
    }
}

void MyFrame::getSelectedDateDailyIDs(cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            getSelectedDateDailyIDs(child);
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
                if (isSelectedTaskDaySameAsAssignedTaskDay(child))
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

cJSON* MyFrame::getAssignedTaskDays(cJSON* node, string title)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (getAssignedTaskDays(child, title))
                return getAssignedTaskDays(child, title);

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
                if (child->valuestring == title)
                {
                    cJSON* child = node->child;
                    while (child)
                    {
                        if (strcmp(child->string, "assigned_days") == 0)
                        {
                            return child;
                        }
                        child = child->next;
                    }
                }
            }
            if (getAssignedTaskDays(child, title))
                return getAssignedTaskDays(child, title);

            child = child->next;
        }
    }
    return NULL;
}

bool MyFrame::isSelectedTaskDaySameAsAssignedTaskDay(cJSON* node)
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

bool MyFrame::isAvailableTaskDaySameAsAssignedTaskDay(string availableTaskDay, cJSON* node)
{
    if (node->type == cJSON_Array)
    {
        cJSON* child = node->child;
        while (child)
        {
            if (child->valuestring == availableTaskDay)
            {
                selectedDays.push_back(availableTaskDay);
                return true;
            }
            
            child = child->next;
        }
    }
    else if (node->type == cJSON_String)
    {
        if (node->valuestring == availableTaskDay)
        {
            selectedDays.push_back(availableTaskDay);
            return true;
        }
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

void MyFrame::addToTaskDatabaseRecords(cJSON* node)
{
    nonEffectiveCalendarClicks += 1;

    if (nonEffectiveCalendarClicks > 1)
    {
        schema = cJSON_CreateObject();
        cJSON_AddItemToObject(schema, "date", cJSON_CreateString(effectiveDate.FormatDate()));
        cJSON* taskDatabase = cJSON_CreateArray();
        cJSON_AddItemToObject(schema, "taskdb", taskDatabase);
    }
}

void MyFrame::addToTaskDatabase(cJSON* node, string title)
{
    cJSON* task = cJSON_CreateObject();
    string randomDailyID = generateRandomDailyID();

    cJSON_AddItemToObject(task, "title", cJSON_CreateString(title.c_str()));
    cJSON_AddItemToObject(task, "daily_id", cJSON_CreateString(randomDailyID.c_str()));
    cJSON_AddItemToObject(task, "assigned_days", cJSON_CreateString(""));

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
    cJSON* task = cJSON_GetArrayItem(selectedDateTaskDatabase, whichTask);
    cJSON_ReplaceItemInObject(task, "title", cJSON_CreateString(title));

    if (selectedDays.size() == 1)
    {
        string assignedTaskDays = selectedDays.front();
        cJSON_ReplaceItemInObject(task, "assigned_days", cJSON_CreateString(assignedTaskDays.c_str()));
    }
    else if (selectedDays.size() > 1)
    {
        cJSON* assignedTaskDays = cJSON_CreateArray();
        for (int count = 0; count < selectedDays.size(); count++)
            cJSON_AddItemToArray(assignedTaskDays, cJSON_CreateString(selectedDays[count].c_str()));

        cJSON_ReplaceItemInObject(task, "assigned_days", assignedTaskDays);
    }
}