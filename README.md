# UnfuckMyTimeZoneMath

A Qt6 application for visualizing and synchronizing times across multiple time zones. Perfect for coordinating meetings, calls, or events across different geographical locations.

## Features

- **Vertical time sliders** representing 24 hours with 15-minute intervals for intuitive time selection
- **Editable friendly names** for each time zone widget (e.g., "Home", "Office", "Client")
- **Clear day and time display** showing both day and time (e.g., "Wed, 12:00a")
- **Comprehensive time zone selection** with dropdown containing all available time zones
- **12/24-hour format toggle** to match your preference
- **Multiple time zone widgets** - add as many as you need
- **Synchronized sliders** - moving any slider updates all others automatically
- **Day offset display** showing relative day differences between zones (e.g., "+1 day", "-1 day")

## Requirements

- CMake 3.16 or later
- Qt6 (Core and Widgets modules)
- C++17 compatible compiler
- For .deb packaging: dpkg tools

## Installation

### Option 1: Install from .deb Package

If you have a pre-built .deb package:

```bash
sudo dpkg -i unfuck-my-timezone-math-1.0.0-Linux.deb
```

If there are missing dependencies, run:

```bash
sudo apt-get install -f
```

After installation, you can launch the application from your application menu or by running:

```bash
UnfuckMyTimeZoneMath
```

### Option 2: Build from Source

1. Clone or download the repository
2. Install dependencies:

```bash
sudo apt-get install cmake qt6-base-dev build-essential
```

3. Build the application:

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

4. Run the application:

```bash
./UnfuckMyTimeZoneMath
```

## Creating a .deb Installer

To create a distributable .deb package:

1. Build the project (if not already built):

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

2. Generate the .deb package:

```bash
cpack
```

The .deb package will be created in the `build` directory with the name:
`unfuck-my-timezone-math-1.0.0-Linux.deb`

3. (Optional) Verify the package contents:

```bash
dpkg-deb --info unfuck-my-timezone-math-1.0.0-Linux.deb
dpkg-deb --contents unfuck-my-timezone-math-1.0.0-Linux.deb
```

## Usage

### Getting Started

1. Launch the application - it starts with one time zone widget showing your system's local time zone
2. Use **File → Add Time Zone** from the menu to add additional time zone widgets
3. Each widget displays:
   - A friendly name (editable by clicking)
   - The selected time zone
   - Current day and time
   - Day offset relative to other zones
   - A vertical slider for time adjustment

### Working with Time Zones

- **Add a time zone**: File → Add Time Zone
- **Change time zone**: Click the dropdown menu at the top of any widget
- **Rename widget**: Click on the friendly name (e.g., "Time Zone 1") and type a new name
- **Adjust time**: Drag any slider up or down - all widgets update automatically to show the corresponding time
- **Remove widget**: Click the "Remove" button (minimum one widget must remain)
- **Toggle format**: Use the "24-hour format" checkbox to switch between 12 and 24-hour time display

### Understanding Day Offsets

Each widget shows a day offset indicator (e.g., "+1 day", "same day", "-1 day") that displays the relative day difference from the first widget. This helps you quickly identify when a specific time falls on a different calendar day in another time zone.

### Example Use Cases

- **Scheduling international meetings**: Add time zones for all participants to find a suitable time
- **Travel planning**: Compare your home time with your destination
- **Remote team coordination**: Keep track of your colleagues' local times
- **Customer support**: Understand when customers in different regions are active

## File Structure

```
UnfuckMyTimeZoneMath/
├── CMakeLists.txt                    # Build configuration
├── README.md                         # This file
├── main.cpp                          # Application entry point
├── mainwindow.h/cpp                  # Main window implementation
├── timezonewidget.h/cpp              # Time zone widget implementation
├── unfuck-my-timezone-math.desktop   # Desktop entry file
└── build/                            # Build directory (created during build)
```

## License

This project is open source. Feel free to modify and distribute.

## Contributing

Contributions are welcome! Feel free to submit issues or pull requests.

## Troubleshooting

### Qt6 not found

If CMake can't find Qt6, ensure it's installed:

```bash
sudo apt-get install qt6-base-dev
```

### Application won't start after installation

Check that dependencies are installed:

```bash
sudo apt-get install -f
```

### Time zones not displaying correctly

Ensure your system time zone database is up to date:

```bash
sudo apt-get install tzdata
```
