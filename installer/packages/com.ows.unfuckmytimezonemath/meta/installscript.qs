function Component()
{
    // Constructor
}

Component.prototype.createOperations = function()
{
    // Call default implementation to actually install the files
    component.createOperations();

    if (systemInfo.productType === "windows") {
        // Create Start Menu shortcut
        component.addOperation("CreateShortcut", 
            "@TargetDir@/bin/UnfuckMyTimeZoneMath.exe", 
            "@StartMenuDir@/Unfuck My TimeZone Math.lnk",
            "workingDirectory=@TargetDir@/bin",
            "iconPath=@TargetDir@/bin/UnfuckMyTimeZoneMath.exe",
            "iconId=0",
            "description=A timezone conversion utility");
        
        // Create Desktop shortcut (optional)
        component.addOperation("CreateShortcut", 
            "@TargetDir@/bin/UnfuckMyTimeZoneMath.exe", 
            "@DesktopDir@/Unfuck My TimeZone Math.lnk",
            "workingDirectory=@TargetDir@/bin",
            "iconPath=@TargetDir@/bin/UnfuckMyTimeZoneMath.exe",
            "iconId=0",
            "description=A timezone conversion utility");
    }
}
