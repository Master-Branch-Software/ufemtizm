# Testing Guide

## Manual Test Plan

### Copy Functionality with 24-Hour Format

#### Test Case 1: Copy with 24-hour format enabled
**Setup:**
1. Launch UnfuckMyTimeZoneMath
2. Add at least 2 timezone widgets
3. Enable "24-hour format" checkbox on both widgets
4. Set time to 14:30 (2:30 PM)

**Steps:**
1. Click Edit → Copy (or Ctrl+C)
2. Paste clipboard contents into a text editor

**Expected Result:**
- Times should be displayed in 24-hour format (e.g., "14:30")
- Format: `[Name], [Time in 24h], [Timezone ID]`
- Example: `Home, 14:30, America/New_York`

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case 2: Copy with 12-hour format enabled
**Setup:**
1. Launch UnfuckMyTimeZoneMath
2. Add at least 2 timezone widgets
3. Disable "24-hour format" checkbox on both widgets
4. Set time to 14:30 (2:30 PM)

**Steps:**
1. Click Edit → Copy (or Ctrl+C)
2. Paste clipboard contents into a text editor

**Expected Result:**
- Times should be displayed in 12-hour format (e.g., "2:30pm")
- Format: `[Name], [Time in 12h], [Timezone ID]`
- Example: `Home, 2:30pm, America/New_York`

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case 3: Copy with mixed format settings
**Setup:**
1. Launch UnfuckMyTimeZoneMath
2. Add 3 timezone widgets
3. Widget 1: Enable 24-hour format
4. Widget 2: Disable 24-hour format
5. Widget 3: Enable 24-hour format
6. Set time to 09:15 (9:15 AM)

**Steps:**
1. Click Edit → Copy (or Ctrl+C)
2. Paste clipboard contents into a text editor

**Expected Result:**
- Widget 1: Time in 24-hour format (e.g., "09:15")
- Widget 2: Time in 12-hour format (e.g., "9:15am")
- Widget 3: Time in 24-hour format (e.g., "09:15" or different based on timezone)
- Each line respects its individual widget's format setting

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case 4: Copy at midnight
**Setup:**
1. Launch UnfuckMyTimeZoneMath
2. Add 2 timezone widgets
3. Set time to 00:00 (midnight)
4. Test both 24-hour and 12-hour formats

**Steps:**
1. With 24-hour format enabled: Copy
2. Paste and verify shows "00:00"
3. Disable 24-hour format: Copy again
4. Paste and verify shows "12:00am"

**Expected Result:**
- 24-hour format: "00:00"
- 12-hour format: "12:00am"

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case 5: Copy at noon
**Setup:**
1. Launch UnfuckMyTimeZoneMath
2. Add 2 timezone widgets
3. Set time to 12:00 (noon)
4. Test both 24-hour and 12-hour formats

**Steps:**
1. With 24-hour format enabled: Copy
2. Paste and verify shows "12:00"
3. Disable 24-hour format: Copy again
4. Paste and verify shows "12:00pm"

**Expected Result:**
- 24-hour format: "12:00"
- 12-hour format: "12:00pm"

**Status:** ✓ Pass / ✗ Fail

---

## Regression Tests

### Existing Functionality

#### Test Case R1: Time synchronization still works
**Steps:**
1. Add 2+ timezone widgets
2. Adjust slider on one widget
3. Verify all other widgets update accordingly

**Expected Result:** All widgets remain synchronized

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case R2: Save/Load with format settings
**Steps:**
1. Create widgets with different format settings
2. Save file (Ctrl+S)
3. Close application
4. Reopen and load file
5. Verify format checkboxes match saved state
6. Copy and verify format is respected

**Expected Result:** Format settings persist across sessions

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case R3: Toolbar copy button works
**Steps:**
1. Add widgets with various format settings
2. Click toolbar Copy button
3. Paste clipboard contents

**Expected Result:** Copy respects format settings (same as menu)

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case R4: Copy dialog selection
**Steps:**
1. Add 3+ timezone widgets with distinct names
2. Press Ctrl+C
3. Verify dialog appears with all tiles checked
4. Uncheck one tile, click Copy
5. Paste and verify only checked tiles were copied

**Expected Result:** Only selected timezones appear in clipboard

**Status:** ✓ Pass / ✗ Fail

---

#### Test Case R5: Copy dialog "Don't show again"
**Steps:**
1. Press Ctrl+C, check "Don't show this again", click Copy
2. Press Ctrl+C again
3. Verify clipboard is updated without dialog appearing
4. Press Ctrl+Shift+C
5. Verify selection dialog always appears (without "Don't show" option)

**Expected Result:** Ctrl+C skips dialog after opt-out; Ctrl+Shift+C always shows dialog

**Status:** ✓ Pass / ✗ Fail

---

## Performance Tests

#### Test Case P1: Copy with many widgets
**Setup:**
1. Add 10+ timezone widgets
2. Mix of 24-hour and 12-hour formats

**Steps:**
1. Click Edit → Copy
2. Measure response time

**Expected Result:**
- Copy completes in < 1 second
- All widgets included in clipboard
- Status bar shows correct count

**Status:** ✓ Pass / ✗ Fail

---

## Testing Notes

- Date format: All tests assume current date - no specific date testing required
- The copy functionality includes: friendly name, time, and timezone ID
- Time format respects each individual widget's setting independently
- The "Don't show again" preference is stored in settings under `copy/skipDialog`
- Ctrl+Shift+C (Copy Selected) always shows the dialog without the "Don't show" option
