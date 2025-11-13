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
            "@TargetDir@/UnfuckMyTimeZoneMath.exe", 
            "@StartMenuDir@/Unfuck My TimeZone Math.lnk",
            "workingDirectory=@TargetDir@",
            "description=A timezone conversion utility");
        
        // Create Desktop shortcut (optional)
        component.addOperation("CreateShortcut", 
            "@TargetDir@/UnfuckMyTimeZoneMath.exe", 
            "@DesktopDir@/Unfuck My TimeZone Math.lnk",
            "workingDirectory=@TargetDir@",
            "description=A timezone conversion utility");
    }
}
