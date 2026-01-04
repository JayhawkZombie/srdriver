#include "task_view.h"
#include "lvglui.h"  // For lvgl_screen access
#include "freertos/SystemMonitorTask.h"
#include "freertos/TaskManager.h"
#include "freertos/LogManager.h"

#if PLATFORM_CROW_PANEL

// Task viewer screen objects
lv_obj_t* lvgl_taskViewScreen = nullptr;
lv_obj_t* lvgl_taskList = nullptr;

// Map to store task UI elements (task name -> container object)
#include <map>
std::map<String, lv_obj_t*> taskUIContainers;

// Forward declarations
static void createTaskViewerScreen();
static void taskViewerBackBtnEventHandler(lv_event_t* e);
static void createTaskListItem(const char* taskName);

void showTaskViewer() {
    LOG_DEBUG_COMPONENT("LVGL", "Showing task viewer screen...");
    
    // Create screen if it doesn't exist
    if (lvgl_taskViewScreen == nullptr) {
        createTaskViewerScreen();
    }
    
    // Load the task viewer screen
    lv_scr_load(lvgl_taskViewScreen);
    
    // Update task list
    updateTaskList();
    
    LOG_DEBUG_COMPONENT("LVGL", "Task viewer screen shown");
}

void hideTaskViewer() {
    if (lvgl_taskViewScreen == nullptr) {
        return;
    }
    
    // Return to main screen
    if (lvgl_screen != nullptr) {
        lv_scr_load(lvgl_screen);
        LOG_DEBUG_COMPONENT("LVGL", "Returned to main screen");
    }
}

bool isTaskViewerShown() {
    return lvgl_taskViewScreen != nullptr && lv_scr_act() == lvgl_taskViewScreen;
}

static void createTaskViewerScreen() {
    LOG_DEBUG_COMPONENT("LVGL", "Creating task viewer screen...");
    
    // Create new screen with flex layout
    lvgl_taskViewScreen = lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(lvgl_taskViewScreen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lvgl_taskViewScreen, LV_OPA_COVER, 0);
    // Make screen a flex container (column layout)
    lv_obj_set_flex_flow(lvgl_taskViewScreen, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(lvgl_taskViewScreen, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(lvgl_taskViewScreen, LV_LAYOUT_FLEX);
    
    // Create header with title and back button (fixed height)
    lv_obj_t* header = lv_obj_create(lvgl_taskViewScreen);
    lv_obj_set_size(header, LV_PCT(100), 60);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xE0E0E0), 0);
    lv_obj_set_style_border_opa(header, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(header, 10, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    
    // Title
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "Task Viewer");
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Back button
    lv_obj_t* backBtn = lv_btn_create(header);
    lv_obj_set_size(backBtn, 80, 40);
    lv_obj_align(backBtn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_bg_color(backBtn, lv_color_hex(0x808080), 0);
    lv_obj_add_event_cb(backBtn, taskViewerBackBtnEventHandler, LV_EVENT_CLICKED, nullptr);
    
    lv_obj_t* backBtnLabel = lv_label_create(backBtn);
    lv_label_set_text(backBtnLabel, "Back");
    lv_obj_center(backBtnLabel);
    
    // Task list (scrollable container) - grid layout
    lvgl_taskList = lv_obj_create(lvgl_taskViewScreen);
    lv_obj_set_width(lvgl_taskList, LV_PCT(100));
    lv_obj_set_height(lvgl_taskList, LV_PCT(100));  // Take remaining space (flex grow)
    lv_obj_set_flex_grow(lvgl_taskList, 1);  // Grow to fill available space
    lv_obj_set_style_bg_opa(lvgl_taskList, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(lvgl_taskList, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(lvgl_taskList, 8, 0);  // Padding
    lv_obj_set_style_pad_column(lvgl_taskList, 8, 0);  // Column spacing for grid
    lv_obj_set_style_pad_row(lvgl_taskList, 8, 0);  // Row spacing for grid
    // Use flex with wrap to create a grid (2 columns)
    lv_obj_set_flex_flow(lvgl_taskList, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(lvgl_taskList, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_layout(lvgl_taskList, LV_LAYOUT_FLEX);
    lv_obj_set_scroll_dir(lvgl_taskList, LV_DIR_VER);
    lv_obj_clear_flag(lvgl_taskList, LV_OBJ_FLAG_SCROLL_ELASTIC);  // No elastic scrolling
    
    LOG_DEBUG_COMPONENT("LVGL", "Task viewer screen created");
}

static void taskViewerBackBtnEventHandler(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        LOG_DEBUG_COMPONENT("LVGL", "Task viewer back button clicked");
        hideTaskViewer();
    }
}

static void createTaskListItem(const char* taskName) {
    if (!lvgl_taskList || !taskName) {
        return;
    }
    
    // Create a button/container for the task
    lv_obj_t* taskItem = lv_btn_create(lvgl_taskList);
    lv_obj_set_size(taskItem, LV_PCT(48), 60);  // 2 columns, fixed height
    lv_obj_set_style_bg_color(taskItem, lv_color_hex(0xF0F0F0), 0);
    lv_obj_set_style_border_width(taskItem, 2, 0);
    lv_obj_set_style_border_color(taskItem, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_radius(taskItem, 5, 0);
    
    // Create label with task name
    lv_obj_t* taskLabel = lv_label_create(taskItem);
    lv_label_set_text(taskLabel, taskName);
    lv_obj_set_style_text_align(taskLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(taskLabel);
    
    // Store in map for later reference
    taskUIContainers[String(taskName)] = taskItem;
}

void updateTaskList() {
    if (!lvgl_taskList) {
        return;
    }
    
    // Get task stats from SystemMonitorTask
    auto* sysMon = TaskManager::getInstance().getSystemMonitorTask();
    if (!sysMon) {
        LOG_WARN_COMPONENT("LVGL", "SystemMonitorTask not available");
        return;
    }
    
    TaskStatsCollection taskStats = sysMon->getTaskStats();
    
    // Clear existing task items
    for (auto& pair : taskUIContainers) {
        if (pair.second) {
            lv_obj_del(pair.second);
        }
    }
    taskUIContainers.clear();
    
    // Create items for each task
    for (const auto& task : taskStats.tasks) {
        createTaskListItem(task.taskName);
    }
    
    LOG_DEBUGF_COMPONENT("LVGL", "Updated task list with %d tasks", taskStats.totalTasks);
}

#endif // PLATFORM_CROW_PANEL

