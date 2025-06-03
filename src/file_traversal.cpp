#include "file_traversal.h"
#include <iostream>

namespace dupesweep {

FileList FileTraversal::collectFiles(const FilePath& rootDir) {
    return collectFiles(rootDir, [](const FilePath&) {});
}


/*
    go through rootDir and store {path, size}
    only if the file is a regular file (ie not directory, symlink, block file etc)
    and if it's not a hidden/system file
*/
FileList FileTraversal::collectFiles(
    const FilePath& rootDir,
    const std::function<void(const FilePath&)>& progressCallback
) {
    FileList files;
    
    try {
        for(const auto& entry: fs::recursive_directory_iterator(
            rootDir,
            fs::directory_options::skip_permission_denied
        )) {
            try {
                if (entry.is_regular_file()) {
                    FilePath path = entry.path();

                    if(shouldProcessFile(path)) {
                        FileSize size = entry.file_size();
                        files.emplace_back(path, size);
                        progressCallback(path);
                    }
                }
            } catch (const fs::filesystem_error& e) {
                std::cerr << "error accessing file " << entry.path() << ": " << e.what() << "\n";
            } catch (const std::exception& e) {
                std::cerr << "error processing file " << entry.path() << ": " << e.what() << "\n";
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "error traversing directory " << rootDir << ": " << e.what() << "\n";
    }

    return files;
}

bool FileTraversal::shouldProcessFile(const FilePath& path) {
    //skip system files and hidden files
    std::string filename = path.filename().string();
    
    //linux hidden files start with '.'
    if(!filename.empty() && filename[0]=='.') {
        return false;
    }

    return true;
}

}