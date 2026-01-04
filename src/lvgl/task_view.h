#pragma once

#if PLATFORM_CROW_PANEL
#include <lvgl.h>

/**
 * Task Viewer UI
 * 
 * Handles task viewer screen including:
 * - Task list with stats displayed in a grid
 * - Task names only (basic version)
 * - Auto-refresh of task information
 */

// Task viewer screen objects (extern - defined in task_view.cpp)
extern lv_obj_t* lvgl_taskViewScreen;
extern lv_obj_t* lvgl_taskList;  // Scrollable container for task list

/**
 * Show task viewer screen
 */
void showTaskViewer();

/**
 * Hide task viewer screen (return to main screen)
 */
void hideTaskViewer();

/**
 * Update task list UI (refresh all tasks)
 */
void updateTaskList();

/**
 * Check if task viewer screen is currently shown
 */
bool isTaskViewerShown();

#endif // PLATFORM_CROW_PANEL

