
#include <stdio.h>
#include <assert.h>
#include <string>
#include <algorithm>

static bool PrintFileBytes(char *filename, FILE *out) {
    FILE *file = fopen(filename, "rb");
    if (!file) return false;

    unsigned char buffer[32];
    size_t count;
    size_t totalCount = 1;
    fprintf(out, "    ");
    while (!feof(file)) {
        count = fread(buffer, 1, 32, file);

        for (int n = 0; n < count; ++n) {
            fprintf(out, "%u,", buffer[n]);
            if (totalCount % 20 == 0 && !((count != 32) && (n == count - 1))) {
                fprintf(out, "\n    ");
            }
            totalCount++;
        };
    };
    fclose(file);
    return true;
};

static unsigned GetFileSize(char *filename) {
    FILE *f = fopen(filename, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    unsigned size = (unsigned)ftell(f);
    rewind(f);
    fclose(f);
    return size;
}

static void PrintUsage(char *name) {
    printf("USAGE: %s OutputDir BinaryFile(s)\n", name);
}

static bool EndsWithSlash(const char *str) {
    size_t len = strlen(str);
    if (str[len - 1] == '/' || str[len - 1] == '\\') return true;
    return false;
}

static const char *GetFilenameAndExt(const char *filepath) {
    const char *startingFilepath = filepath;
    const char *slashPos = NULL;
    while (*filepath) {
        if (*filepath == '/' || *filepath == '\\') {
            slashPos = filepath;
        }
        filepath++;
    }
    if (slashPos == NULL) {
        // return NULL;
        return startingFilepath;
    } else {
        return slashPos + 1;
    }
}

static std::string GetName(const char *filepath) {
    const char *nameAndExt = GetFilenameAndExt(filepath);
    std::string result = nameAndExt;

    std::replace(result.begin(), result.end(), '.', '_');
    std::replace(result.begin(), result.end(), '-', '_');
    std::replace(result.begin(), result.end(), ' ', '_');

    return result;
}

static char *AddFileToDir(const char *dir, const char *file) {
    static char outputHeaderFullPath[512];
    sprintf(outputHeaderFullPath, "%s%s%s", 
            dir, 
            EndsWithSlash(dir) ? "" : "/", 
            file);
    return outputHeaderFullPath;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("binary_builder: ERROR Not enough args\n");
        PrintUsage(argv[0]);
        return 0;
    }

    // for (int i = 0; i < argc; i++) {
    //  printf("arg: %s\n", argv[i]);
    // }

    char *outputDir = argv[1];
    char **binaryFiles = &argv[2];
    int numBinaryFiles = argc - 2;
    assert(numBinaryFiles > 0);

    const char *outputHeaderName = "binary_data.h";
    const char *outputSourceName = "binary_data.cpp";
    const char *namespaceStr = "BinaryData";
    FILE *f = NULL;

    //
    // Test files exsist
    //
    bool errorFound = false;
    for (int i = 0; i < numBinaryFiles; ++i) {
        f = fopen(binaryFiles[i], "rb");
        if (!f) {
            printf("binary_builder: ERROR Could not open %s\n", binaryFiles[i]);
            errorFound = true;
        }
        fclose(f);
    }
    if (errorFound) {
        printf("binary_builder: Failed writing %s and %s\n", outputHeaderName, outputSourceName);
        return 0;
    }

    //
    // Write header
    //
    char *outputHeaderFullPath = AddFileToDir(outputDir, outputHeaderName);
    f = fopen(outputHeaderFullPath, "wt");
    if (!f) {
        printf("binary_builder: ERROR failed to open %s\n", outputHeaderFullPath);
        PrintUsage(argv[0]);
        return 0;
    }
    fprintf(f, "// This file was auto-generated\n");
    fprintf(f, "#pragma once\n\n");
    fprintf(f, "namespace %s {\n", namespaceStr);
    for (int i = 0; i < numBinaryFiles; ++i) {
        std::string fullNameInSource = GetName(binaryFiles[i]);
        fprintf(f, "    extern const unsigned char %s[%u];\n", fullNameInSource.c_str(), GetFileSize(binaryFiles[i]));
    }
    fprintf(f, "}\n");
    fclose(f);

    //
    // Write source
    //
    char *outputSourceFullPath = AddFileToDir(outputDir, outputSourceName);
    f = fopen(outputSourceFullPath, "wt");
    if (!f) {
        printf("binary_builder: ERROR failed to open %s\n", outputSourceFullPath);
        PrintUsage(argv[0]);
        return 0;
    }

    fprintf(f, "// This file was auto-generated\n");
    fprintf(f, "#include \"%s\"\n\n", outputHeaderName);
    fprintf(f, "namespace %s {\n", namespaceStr);
    for (int i = 0; i < numBinaryFiles; ++i) {
        std::string fullNameInSource = GetName(binaryFiles[i]);

        fprintf(f, "const unsigned char %s[%d] = {\n", fullNameInSource.c_str(), GetFileSize(binaryFiles[i]));
        bool result = PrintFileBytes(binaryFiles[i], f);
        fprintf(f, "\n};\n");

    }
    fprintf(f, "}\n");
    fclose(f);

    printf("binary_builder: Sucessfully written %s and %s\n", outputHeaderName, outputSourceName);
    return 1;
}


