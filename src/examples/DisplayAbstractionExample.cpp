#include "../hal/DisplayBuilder.h"
#include "../hal/DisplayRenderer.h"
#include "../hal/DisplayManager.h"

// Example showing different abstraction levels

/**
 * Example 1: Using DisplayBuilder (Declarative approach)
 * 
 * This is the most user-friendly approach - you describe what you want
 * and the builder handles the layout and styling.
 */
void exampleDisplayBuilder(SSD1306_Display& display) {
    DisplayBuilder builder(display);
    
    // Create a status card
    builder
        .card(10, 10, 108, 44)
            .title("System Status")
            .text("All systems operational")
            .progress(85)
        .end()
        .icon(120, 10, "wifi")
        .text(2, 60, "Uptime: 1h 23m")
        .render();
}

/**
 * Example 2: Using DisplayRenderer (Imperative approach)
 * 
 * More control over exact positioning and drawing
 */
void exampleDisplayRenderer(SSD1306_Display& display) {
    DisplayRenderer renderer(display);
    
    // Create layouts
    auto mainLayout = renderer.createLayout(0, 0, 128, 64);
    auto cardLayout = renderer.createLayout(10, 10, 108, 44);
    auto statusLayout = renderer.createLayout(2, 60, 126, 4);
    
    // Draw components
    renderer.drawCard(cardLayout, "System Status", "All systems operational");
    renderer.drawProgressBar(cardLayout, 85);
    renderer.drawStatusBar("Uptime: 1h 23m", "WiFi Connected");
}

/**
 * Example 3: Using DisplayTemplate (Template approach)
 * 
 * Pre-built templates for common use cases
 */
void exampleDisplayTemplate(SSD1306_Display& display) {
    DisplayTemplate template_(display);
    
    // Use pre-built templates
    template_.renderStatusScreen("System Status", "All systems operational", 85);
    
    // Or switch to a menu
    std::vector<String> menuItems = {"Settings", "System", "About"};
    template_.renderMenuScreen("Main Menu", menuItems, 0);
}

/**
 * Example 4: Using DisplayPreset (Quick preset approach)
 * 
 * Ultra-simple for common scenarios
 */
void exampleDisplayPreset(SSD1306_Display& display) {
    DisplayPreset preset(display);
    
    // Quick presets
    preset.centered("Hello World!");
    preset.progressBar(75);
    preset.loadingSpinner();
    preset.errorMessage("Connection failed");
    preset.successMessage("Upload complete!");
}

/**
 * Example 5: Using DisplayManager (Full framework approach)
 * 
 * Complete screen management with widgets and animations
 */
void exampleDisplayManager(SSD1306_Display& display) {
    DisplayManager manager(display);
    
    // Create screens
    auto statusScreen = std::make_shared<StatusScreen>();
    auto menuScreen = std::make_shared<MenuScreen>();
    auto animationScreen = std::make_shared<AnimationScreen>();
    
    // Add screens to manager
    manager.addScreen(statusScreen);
    manager.addScreen(menuScreen);
    manager.addScreen(animationScreen);
    
    // Create widgets
    auto progressWidget = std::make_shared<ProgressBarWidget>("progress", 10, 30, 108, 8);
    auto textWidget = std::make_shared<TextWidget>("status", 10, 45, "Running", 1);
    
    // Add widgets
    manager.addWidget(progressWidget);
    manager.addWidget(textWidget);
    
    // Show different screens
    manager.showScreen("Status");
    manager.nextScreen();  // Show menu
    manager.nextScreen();  // Show animation
}

/**
 * Example 6: Real-world application - Smart Home Dashboard
 */
void exampleSmartHomeDashboard(SSD1306_Display& display) {
    DisplayBuilder builder(display);
    
    builder
        .card(5, 5, 118, 54)
            .title("Smart Home")
            .row()
                .icon("temp")
                .text("22°C")
                .space(10)
                .icon("humidity")
                .text("45%")
            .end()
            .separator()
            .row()
                .icon("light")
                .text("Living Room")
                .progress(75)
            .end()
            .row()
                .icon("lock")
                .text("Front Door")
            .end()
        .end()
        .statusBar("WiFi", "2:30 PM")
        .render();
}

/**
 * Example 7: Real-world application - Music Player
 */
void exampleMusicPlayer(SSD1306_Display& display) {
    DisplayTemplate template_(display);
    
    // Show current track
    std::vector<String> trackInfo = {
        "Artist: The Beatles",
        "Album: Abbey Road",
        "Track: Come Together"
    };
    
    template_.renderInfoScreen("Now Playing", trackInfo);
    
    // Show progress
    template_.renderProgressScreen("Come Together", 45, "2:34 / 5:30");
}

/**
 * Example 8: Real-world application - Game Interface
 */
void exampleGameInterface(SSD1306_Display& display) {
    DisplayPreset preset(display);
    
    // Game grid (e.g., Snake game)
    std::vector<std::vector<bool>> grid = {
        {false, false, true, false},
        {false, false, true, false},
        {false, false, true, false},
        {false, false, false, false}
    };
    
    preset.gameGrid(4, 4, grid);
    
    // Show score
    preset.gameScore(1250, 2500);
}

/**
 * Example 9: Real-world application - Sensor Monitor
 */
void exampleSensorMonitor(SSD1306_Display& display) {
    DisplayRenderer renderer(display);
    
    // Create chart data
    std::vector<uint8_t> sensorData = {20, 25, 30, 28, 35, 32, 38};
    
    // Draw chart
    auto chartLayout = renderer.createLayout(10, 20, 108, 30);
    renderer.drawLineChart(chartLayout, sensorData, true);
    
    // Draw sensor info
    renderer.drawInfoCard(chartLayout, "Temperature", {"Current: 32°C", "Avg: 28°C", "Max: 38°C"});
}

/**
 * Example 10: Real-world application - System Monitor
 */
void exampleSystemMonitor(SSD1306_Display& display) {
    DisplayBuilder builder(display);
    
    builder
        .card(5, 5, 118, 54)
            .title("System Monitor")
            .row()
                .text("CPU:")
                .progress(65)
                .text("65%")
            .end()
            .row()
                .text("MEM:")
                .progress(45)
                .text("45%")
            .end()
            .row()
                .text("UPTIME:")
                .text("1d 2h 15m")
            .end()
            .row()
                .text("TEMP:")
                .text("42°C")
                .icon("warning")
            .end()
        .end()
        .statusBar("WiFi", "2.4GHz")
        .render();
}

/**
 * Example 11: Animation showcase
 */
void exampleAnimationShowcase(SSD1306_Display& display, uint32_t frame) {
    DisplayRenderer renderer(display);
    
    // Fade in effect
    auto fadeLayout = renderer.createLayout(10, 10, 108, 44);
    renderer.drawFadeIn(fadeLayout, frame, 60);
    
    // Animated progress bar
    auto progressLayout = renderer.createLayout(10, 30, 108, 8);
    renderer.drawAnimatedProgress(progressLayout, 75, frame);
    
    // Loading spinner
    auto spinnerLayout = renderer.createLayout(60, 45, 8, 8);
    renderer.drawLoadingSpinner(spinnerLayout, frame);
}

/**
 * Example 12: Interactive menu system
 */
void exampleInteractiveMenu(SSD1306_Display& display, int selectedItem) {
    DisplayTemplate template_(display);
    
    std::vector<String> menuItems = {
        "Settings",
        "System Info", 
        "Network",
        "Sensors",
        "Games",
        "About"
    };
    
    template_.renderMenuScreen("Main Menu", menuItems, selectedItem);
}

/**
 * Example 13: Data visualization
 */
void exampleDataVisualization(SSD1306_Display& display) {
    DisplayRenderer renderer(display);
    
    // Multiple charts
    std::vector<uint8_t> tempData = {20, 22, 25, 23, 28, 26, 30};
    std::vector<uint8_t> humidityData = {45, 48, 50, 47, 52, 49, 55};
    
    auto tempLayout = renderer.createLayout(5, 10, 58, 20);
    auto humidityLayout = renderer.createLayout(65, 10, 58, 20);
    
    renderer.drawBarChart(tempLayout, tempData, false);
    renderer.drawBarChart(humidityLayout, humidityData, false);
    
    // Labels
    renderer.drawCenteredText(tempLayout, "Temp");
    renderer.drawCenteredText(humidityLayout, "Humidity");
}

/**
 * Example 14: Notification system
 */
void exampleNotificationSystem(SSD1306_Display& display, uint32_t frame) {
    DisplayTemplate template_(display);
    
    // Show notification
    template_.renderNotificationScreen("New Message", "Hello from SRDriver!", frame);
    
    // Or show multiple notifications
    std::vector<String> notifications = {
        "WiFi connected",
        "New sensor data",
        "System update available"
    };
    
    template_.renderListScreen("Notifications", notifications, 0);
}

/**
 * Example 15: Theme switching
 */
void exampleThemeSwitching(SSD1306_Display& display) {
    DisplayRenderer renderer(display);
    
    // Apply different themes
    DisplayTheme defaultTheme = DisplayTheme::getDefaultTheme();
    DisplayTheme minimalTheme = DisplayTheme::getMinimalTheme();
    DisplayTheme compactTheme = DisplayTheme::getCompactTheme();
    
    // Use theme in rendering
    auto layout = renderer.createLayout(10, 10, 108, 44);
    renderer.setTextStyle(defaultTheme.titleSize);
    renderer.drawCard(layout, "Themed Card", "This uses a custom theme");
} 