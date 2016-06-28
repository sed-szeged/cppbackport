

extern "C" {
    /*
     * If an error occurred then the return value is -1. Otherwise if the file was transformed it returns the original line number.
     * If the file was untouched it returns -1. You can disable this error and get the given line number
     * back if you set pathIsCorrect to non 0.
     */
    int traceBack(char const *dbFilePath, char const *filePath, int lineNum, int pathIsCorrect = 0);
}
