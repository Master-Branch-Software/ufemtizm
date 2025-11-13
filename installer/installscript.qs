function Component() {
    component.loaded.connect(this, Component.prototype.installerLoaded);
}
Component.prototype.createOperations = function() {
    // Call the base createOperations (important for default operations)
    component.createOperations();
    if (systemInfo.productType === "windows") {
        // Optional: Add VC redist if needed (uncomment if you bundle it)
        // component.addOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@\\bin\\vc_redist.x64.exe", "/quiet", "/norestart");
        // component.addOperation("Delete", "@TargetDir@\\bin\\vc_redist.x64.exe");

        component.addOperation("CreateShortcut",
                               "@TargetDir@/bin/UnfuckMyTimeZoneMath.exe", // Path to the executable
                               "@StartMenuDir@/Unfuck My TimeZone Math.lnk", // Shortcut location in Start menu (user-specific; use folder if needed)
                               "workingDirectory=@TargetDir@/bin", // Working directory
                               "iconPath=@TargetDir@/bin/UnfuckMyTimeZoneMath.ico", // Path to the icon
                               "description=A Qt-based timezone conversion utility"); // Description

        component.addOperation("CreateShortcut",
                               "@TargetDir@/bin/UnfuckMyTimeZoneMath.exe",
                               "@HomeDir@/Desktop/Unfuck My TimeZone Math.lnk",
                               "iconPath=@TargetDir@/bin/UnfuckMyTimeZoneMath.ico");
    }
}
Component.prototype.installerLoaded = function() {
    // Optional: Customize installer UI (e.g., show/hide pages)
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, true);
}
Component.prototype.createUndoSet = function() {
    // Call the base createUndoSet for default uninstall operations
    component.createUndoSet();
    if (systemInfo.productType === "windows") {
        // Remove shortcuts on uninstall
        component.addUndoOperation("Delete", "@StartMenuDir@/Unfuck My TimeZone Math.lnk");
        component.addUndoOperation("Delete", "@HomeDir@/Desktop/Unfuck My TimeZone Math.lnk");
    }
}
