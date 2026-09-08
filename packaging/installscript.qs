function Component() {}

Component.prototype.createOperations = function() {
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/bin/QTdemo.exe",
                               "@StartMenuDir@/QTdemo.lnk",
                               "workingDirectory=@TargetDir@/bin");
        component.addOperation("CreateShortcut", "@TargetDir@/Uninstall.exe",
                               "@StartMenuDir@/Uninstall QTdemo.lnk");
    }
};
