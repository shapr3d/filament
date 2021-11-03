//#include <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#include <AppKit/Appkit.h>
#include <iostream>

#include <viewer/MacosCustomFileDialogs.h>

bool SD_MacOpenFileDialog(char* browsedFile, const char* formats) {
    std::cout << "macOS Open File" << std::endl;
    
    NSOpenPanel* openPanel = [NSOpenPanel openPanel];
    openPanel.allowedFileTypes = @[ [NSString stringWithCString:formats encoding:NSUTF8StringEncoding] ];
    NSModalResponse response = [openPanel runModal];
    
    if (response == NSModalResponseOK) {
        NSURL* theDoc = [[openPanel URLs] objectAtIndex:0];
        strcpy(browsedFile, theDoc.fileSystemRepresentation); // TODO insecure af
        return true;
    } else {
        return false;
    }
}

bool SD_MacSaveFileDialog(char* browsedFile, const char* formats) {
    std::cout << "macOS Save File" << std::endl;
    
    NSSavePanel* savePanel = [NSSavePanel savePanel];
//    savePanel.allowedFileTypes = @[ [UTType typeWithIdentifier:@"public.json"] ];
    NSModalResponse response = [savePanel runModal];
    
    if (response == NSModalResponseOK) {
        NSURL* theDoc = savePanel.URL;
        strcpy(browsedFile, theDoc.fileSystemRepresentation); // TODO insecure af
        return true;
    } else {
        return false;
    }
}
