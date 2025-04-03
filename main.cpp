#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>

struct FileMeta {
    int size;
    char name[20];
};

struct FileEntry {
    FileMeta meta;
    std::string data;
};

struct FileSystemHeader {
    int tableOffset;
    int fileCount;
    int dataOffset;
};

struct FileRecord {
    char name[20];
    int offset;
};

class FileSystem {
private:
    const std::string storage = "fs_data";
    std::string currentDir = "/";

public:
    void init() {
        std::ofstream f(storage, std::ios::binary);
        if (!f) return;
        FileSystemHeader header = {100, 0, 1000};
        f.write((char*)&header, sizeof(header));
        f.close();
    }

    FileSystemHeader getHeader() {
        FileSystemHeader header;
        std::ifstream f(storage, std::ios::binary);
        if (!f) return header;
        f.read((char*)&header, sizeof(header));
        f.close();
        return header;
    }

    void updateHeader(const FileSystemHeader& header) {
        std::ofstream f(storage, std::ios::in | std::ios::binary);
        if (!f) return;
        f.seekp(0);
        f.write((char*)&header, sizeof(header));
        f.close();
    }

    void changeDir(const std::string& path) {
        if (path == ".." && currentDir != "/") {
            size_t pos = currentDir.find_last_of("/");
            currentDir = currentDir.substr(0, pos);
            if (currentDir.empty()) currentDir = "/";
        } else {
            currentDir += (currentDir.back() == '/' ? "" : "/") + path;
        }
        std::cout << "Current directory: " << currentDir << "\n";
    }

    void writeFile(const std::string& filename) {
        std::ifstream inFile(filename, std::ios::binary);
        if (!inFile) return;
        
        FileEntry file;
        strncpy(file.meta.name, filename.c_str(), 20);
        file.data.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        file.meta.size = file.data.size();
        inFile.close();
        
        FileSystemHeader header = getHeader();
        FileRecord record;
        strncpy(record.name, file.meta.name, 20);
        record.offset = header.dataOffset;
        
        header.fileCount++;
        header.dataOffset += sizeof(file.meta) + file.meta.size;
        updateHeader(header);
        
        std::ofstream f(storage, std::ios::in | std::ios::binary);
        f.seekp(header.tableOffset + (header.fileCount - 1) * sizeof(FileRecord));
        f.write((char*)&record, sizeof(record));
        f.seekp(record.offset);
        f.write((char*)&file.meta, sizeof(file.meta));
        f.write(file.data.data(), file.data.size());
        f.close();
    }

    void readFile(const std::string& filename) {
        FileSystemHeader header = getHeader();
        std::ifstream f(storage, std::ios::binary);
        if (!f) return;
        
        std::vector<FileRecord> records(header.fileCount);
        f.seekg(header.tableOffset);
        f.read((char*)records.data(), header.fileCount * sizeof(FileRecord));
        
        FileRecord* found = nullptr;
        for (auto& r : records) {
            if (filename == r.name) {
                found = &r;
                break;
            }
        }
        if (!found) {
            std::cout << "File not found: " << filename << "\n";
            return;
        }
        
        f.seekg(found->offset);
        FileEntry file;
        f.read((char*)&file.meta, sizeof(file.meta));
        file.data.resize(file.meta.size);
        f.read(&file.data[0], file.meta.size);
        f.close();
        
        std::ofstream outFile("copy_" + filename, std::ios::binary);
        outFile << file.data;
        outFile.close();
        std::cout << "File read to copy_" << filename << "\n";
    }
};

int main() {
    FileSystem fs;
    std::string command, filename;
    
    while (true) {
        std::cout << "Enter command (init, cd, read, write, exit): ";
        std::cin >> command;
        if (command == "exit") break;
        if (command == "init") {
            fs.init();
        } else if (command == "cd") {
            std::cin >> filename;
            fs.changeDir(filename);
        } else if (command == "read") {
            std::cin >> filename;
            fs.readFile(filename);
        } else if (command == "write") {
            std::cin >> filename;
            fs.writeFile(filename);
        } else {
            std::cout << "Unknown command\n";
        }
    }
    return 0;
}
