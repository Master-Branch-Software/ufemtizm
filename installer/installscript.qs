function Component() {
    console.log("=== Script constructor called ===");
    if (component) {
        console.log("=== Component context available ===");
        component.loaded.connect(this, Component.prototype.installerLoaded);
    } else {
        console.log("=== WARNING: component null (global load?) ===");
    }
}

Component.prototype.installerLoaded = function() {
    console.log("=== installerLoaded called ===");
    installer.setDefaultPageVisible(QInstaller.LicenseCheck, true);
}

Component.prototype.createOperations = function() {
    console.log("=== createOperations called ===");
    component.createOperations();
    if (systemInfo.productType === "windows") {
        console.log("=== Windows: Adding shortcuts ===");
        component.addOperation("CreateShortcut",
                               "@TargetDir@/bin/Ufemtizm.exe",
                               "@StartMenuDir@/Ufemtizm.lnk",
                               "workingDirectory=@TargetDir@/bin",
                               "iconPath=@TargetDir@/bin/Ufemtizm.ico",
                               "description=A Qt-based timezone conversion utility");
        console.log("=== StartMenu shortcut added ===");
        component.addOperation("CreateShortcut",
                               "@TargetDir@/bin/Ufemtizm.exe",
                               "@HomeDir@/Desktop/Ufemtizm.lnk",
                               "iconPath=@TargetDir@/bin/Ufemtizm.ico");
        console.log("=== Desktop shortcut added ===");
    }
}

Component.prototype.createUndoSet = function() {
    console.log("=== createUndoSet called ===");
    component.createUndoSet();
    if (systemInfo.productType === "windows") {
        component.addUndoOperation("Delete", "@StartMenuDir@/Ufemtizm.lnk");
        component.addUndoOperation("Delete", "@HomeDir@/Desktop/Ufemtizm.lnk");
    }
}
