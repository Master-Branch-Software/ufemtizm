# UnfuckMyTimeZoneMath

A Qt6 application for visualizing and synchronizing times across multiple time zones.

## Features

- Vertical time sliders representing 24 hours with 15-minute intervals
- Editable friendly names for each time zone widget
- Day and time display (e.g., "Wed, 12:00a")
- Time zone selection dropdown with all available time zones
- 12/24-hour format toggle
- Add multiple time zone widgets
- Synchronized sliders - moving any slider updates all others
- Day offset display showing relative day differences between zones

## Building

```bash
cd ~/Documents/UnfuckMyTimeZoneMath
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./UnfuckMyTimeZoneMath
```

## Usage

1. The application starts with one time zone widget (your system time zone)
2. Use File → Add Time Zone to add more widgets
3. Move any slider to adjust the time - all widgets will update accordingly
4. Click the time zone dropdown to select different zones
5. Toggle the "24-hour format" checkbox to switch between 12 and 24-hour display
6. Edit the friendly name at the top of each widget
7. Click "Remove" to delete a widget (minimum one widget required)
