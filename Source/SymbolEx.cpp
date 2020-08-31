// Symbol extractor - tool for extracting symbols from verilog source files.
// Copyright (c) 2020 Stanislav Jurny (github.com/STjurny) licence MIT

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <cassert>
#include <cstdarg>
#include <PracticString.h>

using namespace std;
using namespace Practic;



// Configuration //////////////////////////////////////////////////////////////////////////////////////////////////

#define filePathEqualityMode caseInsensitive



// General Utilities //////////////////////////////////////////////////////////////////////////////////////////////////

struct ParseError {};


bool tryStringToInt(String aString, int * oValue, int aRadix)
{
    try 
    { 
        *oValue = stoi(aString.rb(), NULL, aRadix); 
        return true;
    } 
    catch (logic_error) 
    { 
        return false; 
    };
}



// Logging  ///////////////////////////////////////////////////////////////////////////////////////////////////////////

int verbosityLevel = 1;
const int maxVerbosityLevel = 5;


void consoleWrite(int aLevel, const char * aTemplate, ...)
{
    if (aLevel > verbosityLevel)
        return;

    va_list arguments;
    va_start(arguments, aTemplate);
    String text = String::formattedList(aTemplate, arguments);
    va_end(arguments);

    static bool wasWrite = false;

    if (wasWrite && aLevel == 0) 
        printf("\n");  // indent error message from previous text

    printf("%s\n", text.rb());

    wasWrite = true;
}



// File System Utilities //////////////////////////////////////////////////////////////////////////////////////////////

String extractFileNameWithoutExtension(String aFilePath)
{
    return filesystem::path(aFilePath.rb()).filename().replace_extension().string().c_str();
}


void createDirectoryPath(String aDirectory)
{
    try { 
        filesystem::create_directories(aDirectory.rb()); 
    } 
    catch (exception error) {
        throw String::formatted(
            "Can't create output directory \"%s\".\n%s", 
            aDirectory.rb(), error.what());
    }
}



// Reading and Writing String to File /////////////////////////////////////////////////////////////////////////////////

String readStringFromFile(String aFilePath)
{
    ifstream file;
    file.exceptions(ios::badbit | ios::failbit);

    try {
        file.open(aFilePath.rb());

        ostringstream stream;
        stream << file.rdbuf();

        String text = stream.str().c_str();
        return text;
    }
    catch (ifstream::failure error) {
        throw String::formatted(
            "Can not read file \"%s\".\n%s", 
            aFilePath.rb(), error.what());
    }
}


void writeStringToFile(String aFilePath, String aString)
{
    ofstream file;
    file.exceptions(ios::badbit | ios::failbit);

    try {
        file.open(aFilePath.rb(), ios::out);
        file << aString.rb();
    }
    catch (ifstream::failure error) {
        throw String::formatted(
            "Can not write file \"%s\".\n%s", 
            aFilePath.rb(), error.what());
    }
}



// Table File Name Utilities //////////////////////////////////////////////////////////////////////////////////////////

const String tableFileExtension = "txt";


String buildTableFilePath(String anOutputDirectoryPath, String aVerilogFilePath, String aTableName)
{
    String verilogFileName = extractFileNameWithoutExtension(aVerilogFilePath);
    String tableFileName = verilogFileName + '.' + aTableName + '.' + tableFileExtension;

    filesystem::path tableFilePath;
    tableFilePath.append(anOutputDirectoryPath.rb());
    tableFilePath.append(tableFileName.rb());

    return tableFilePath.string().c_str();
}


bool isTableFileName(String aTestedFileName, String aVerilogFilePath)
{
    String verilogFileName = extractFileNameWithoutExtension(aVerilogFilePath);
    assert(verilogFileName != "");

    ParsingContext context("."); 

    return 
        aTestedFileName.partCount(context) == 3 &&
        aTestedFileName.part(0, context).equals(verilogFileName, filePathEqualityMode) &&
        aTestedFileName.part(1, context) != "" &&
        aTestedFileName.part(2, context).equals(tableFileExtension, filePathEqualityMode);
}



// Verilog Number Utilities ///////////////////////////////////////////////////////////////////////////////////////////

typedef unsigned long long VerilogNumber;


#define verilogNumberMaxBitWidth (sizeof(VerilogNumber) * 8)


VerilogNumber bitWidthMask(int aBitWidth)
{
    VerilogNumber result = 0;

    for (int order = 1; order <= aBitWidth; order += 1)
    {
        result <<= 1;
        result += 1;
    }

    return result;
}


bool tryStringToVerilogNumber(String aString, VerilogNumber * oValue, int aRadix)
{
    try { 
        *oValue = stoull(aString.rb(), NULL, aRadix); 
        return true;
    } 
    catch (logic_error) { 
        return false; 
    };
}


String verilogNumberToHexString(VerilogNumber aNumber, int aDigitCount)
{
    String result = String::formatted("%llX", aNumber);
    result.padLeft(aDigitCount, '0');
    return result;
}



// Parsing Chars //////////////////////////////////////////////////////////////////////////////////////////////////////

bool skipChar(String aChars, bool aMandatory, String aText, int * ioIndex)
{
    bool found = aText.containsAnyCharAt(*ioIndex, containedIn, aChars);

    if (found)
        *ioIndex += 1;

    return found || !aMandatory;
}


bool skipChars(String aChars, bool aMandatory, String aText, int * ioIndex)
{
    int length;
    bool found = aText.containsCharsAt(*ioIndex, containedIn, aChars, &length);

    if (found)
        *ioIndex += length;

    return found || !aMandatory;
}


String radixChars(int aRadix)
{
    switch (aRadix)
    {
        case 2:  return "01";
        case 8:  return "01234567";
        case 10: return "0123456789";
        case 16: return "0123456789ABCDEFabcdef";

        default: 
            assert(false);
            return "";
    }
}



// Parsing Comments and Whitespace ////////////////////////////////////////////////////////////////////////////////////

const String whitespace = " \t";
const String newline = "\n\r";


bool skipLineCommentStart(String aText, int * ioIndex)
{
    // format: //

    static const String commentStart = "//";

    bool found = aText.containsAt(*ioIndex, commentStart);

    if (found)
        *ioIndex += commentStart.length();

    return found;
}


bool skipLineComment(String aText, int * ioIndex)
{
    // format: // ... eol

    if (!skipLineCommentStart(aText, ioIndex))
        return false;

    int length;
    aText.containsCharsAt(*ioIndex, notContainedIn, newline, &length);

    *ioIndex += length;

    skipChars(newline, false, aText, ioIndex);

    return true;
}


bool skipGeneralComment(String aText, int * ioIndex)
{
    // format: /* ... */

    static const String commentStart = "/*";
    static const String commentEnd = "*/";

    if (!aText.containsAt(*ioIndex, commentStart))
        return false;

    int endIndex = aText.indexOf(commentEnd, caseSensitive, *ioIndex);

    if (endIndex != notFound)
        *ioIndex = endIndex + commentEnd.length();

    return true;
}


void skipBlank(String aText, int * ioIndex)
{
    while (
        skipChars(whitespace + newline, true, aText, ioIndex) ||
        skipLineComment(aText, ioIndex) ||
        skipGeneralComment(aText, ioIndex)
    );
}



// Parsing Definition Header //////////////////////////////////////////////////////////////////////////////////////////

bool moveToNextLocalParam(String aText, int * ioIndex)
{
    static String localParam = "localparam";

    int index = aText.indexOf(localParam, caseSensitive, *ioIndex);

    if (index == notFound)
        return false;

    *ioIndex = index + localParam.length();

    return *ioIndex < aText.length();
}


int isTableNameChar(int aChar)
{
    return aChar > 0 && (isalnum(aChar) || aChar == '_');
}


bool readHeaderTableName(String * oTableName, String aText, int * ioIndex)
{
    int length;
    if (aText.containsCharsAtWhere(*ioIndex, isTableNameChar, true, &length))
    {
        *oTableName = aText.substringFrom(*ioIndex, length);
        *ioIndex += length;
        return true;
    }
    else
        return false;
}


bool readHeaderBitWidth(int * oBitWidth, String aText, int * ioIndex)
{
    int index = *ioIndex;

    int length;
    String bitWidthText;
    if (!aText.containsCharsAt(index, containedIn, radixChars(10), &length))
        return false;

    bitWidthText = aText.substringFrom(index, length);
    index += length;

    if (!tryStringToInt(bitWidthText, oBitWidth, 10))
        return false;

    *ioIndex = index;
    return true;
}


String readRemovingPrefix(String aText, int * ioIndex)
{
    int length;
    if (!aText.containsCharsAt(*ioIndex, notContainedIn, whitespace + newline, &length))
        return "";

    String prefix = aText.substringFrom(*ioIndex, length);
    *ioIndex += length;

    return prefix;
}


bool readHeader(String * oTableName, int * oBitWidth, String * oRemovingPrefix, String aText, int * ioIndex)
{
    // format: // $table_name : bit_width [; removing_prefix]

    int index = *ioIndex;

    skipChars(whitespace + newline, false, aText, &index);

    if (!skipLineCommentStart(aText, &index))
        return false;

    skipChars(whitespace, false, aText, &index);

    if (!skipChar("$", true, aText, &index))
        return false;

    if (!readHeaderTableName(oTableName, aText, &index))
        return false;

    skipChars(whitespace, false, aText, &index);

    if (!skipChar(":", true, aText, &index))
        return false;

    skipChars(whitespace, false, aText, &index);

    if (!readHeaderBitWidth(oBitWidth, aText, &index))
       return false;

    skipChars(whitespace, false, aText, &index);

    if (skipChar(",", true, aText, &index)) 
    {
        skipChars(whitespace, false, aText, &index);
        *oRemovingPrefix = readRemovingPrefix(aText, &index);
        skipChars(whitespace, false, aText, &index);
    }
    else
        *oRemovingPrefix = "";

    if (!skipChars(newline, true, aText, &index))
        return false;

    if (*oBitWidth < 1 || *oBitWidth > verilogNumberMaxBitWidth)
        throw String::formatted(
            "Unsupported size (%d bits) of \"%s\" (size must be from 1 to %d bits).", 
            *oBitWidth, oTableName->rb(), verilogNumberMaxBitWidth);

    *ioIndex = index;
    return true;
}



// Parsing Verilog Number /////////////////////////////////////////////////////////////////////////////////////////////

bool readNumberBitWidth(int * oBitWidth, String aText, int * ioIndex)
{
    int index = *ioIndex;

    int length;
    if (!aText.containsCharsAt(index, containedIn, radixChars(10), &length))
        return false;

    String radixText = aText.substringFrom(index, length);
    index += length;

    if (!tryStringToInt(radixText, oBitWidth, 10))
        return false;

    *ioIndex = index;
    return true;
}


bool readNumberRadix(int * oRadix, String aText, int * ioIndex)
{
    int index = *ioIndex;

    if (!aText.containsAt(index, '\''))
        return false;

    index += 1;

    if      (aText.containsAt(index, 'B', caseInsensitive)) *oRadix = 2;
    else if (aText.containsAt(index, 'O', caseInsensitive)) *oRadix = 8;
    else if (aText.containsAt(index, 'D', caseInsensitive)) *oRadix = 10;
    else if (aText.containsAt(index, 'H', caseInsensitive)) *oRadix = 16;
    else return false;

    index += 1;

    *ioIndex = index;
    return true;
}


bool readNumberValue(String * oValueText, String aText, int * ioIndex)
{
    int index = *ioIndex;

    int length;
    if (!aText.containsCharsAt(index, containedIn, radixChars(16) + '_', &length))
        return false;

    *oValueText = aText.substringFrom(index, length);
    index += length;

    oValueText->remove('_');

    *ioIndex = index;
    return true;
}


VerilogNumber readNumber(String aText, int * ioIndex)
{
    // format: <bit_widh> <'radix> <value>
    // format: <'radix> <value>
    // format: <value>

    int radix;
    int bitWidth;
    String valueText;

    int startIndex = *ioIndex;

    bool readed = 
        readNumberBitWidth(&bitWidth, aText, ioIndex) &&
        skipChars(whitespace, false, aText, ioIndex) &&
        readNumberRadix(&radix, aText, ioIndex) &&
        skipChars(whitespace, false, aText, ioIndex) &&
        readNumberValue(&valueText, aText, ioIndex);

    if (!readed) {
        *ioIndex = startIndex;
        readed = 
            readNumberRadix(&radix, aText, ioIndex) &&
            skipChars(whitespace, false, aText, ioIndex) &&
            readNumberValue(&valueText, aText, ioIndex);
        bitWidth = 32;
    }

    if (!readed) {
        *ioIndex = startIndex;
        readed = readNumberValue(&valueText, aText, ioIndex);
        bitWidth = 32;
        radix = 10;
    }

    VerilogNumber value;

    if (
        !readed || 
        bitWidth < 1 || 
        bitWidth > verilogNumberMaxBitWidth ||
        !tryStringToVerilogNumber(valueText, &value, radix)
    ) throw String::formatted(
        "Value must be non-negative integer constant with max %d bits size.", 
        verilogNumberMaxBitWidth);

    return value;
}



// Parsing Symbols ////////////////////////////////////////////////////////////////////////////////////////////////////

struct Symbol
{
    String name;
    VerilogNumber value;
    Symbol(String aName, VerilogNumber aValue): name(aName), value(aValue) {}
};


int isIdentifierStartChar(int aChar) 
{ 
    return aChar > 0 && (isalpha(aChar) || aChar == '_');
}


int isIdentifierInnerChar(int aChar) 
{ 
    return aChar > 0 && (isalnum(aChar) || aChar == '_' || aChar == '$');
}


String readIdentifier(String aText, int * ioIndex)
{
    if (aText.containsAt(*ioIndex, '\\'))
        throw String("Escaped identifiers are not supported.");

    int startIndex = *ioIndex;

    if (!aText.containsAnyCharAtWhere(*ioIndex, isIdentifierStartChar, true))
        throw String::formatted("Missing or invalid identifier.");

    *ioIndex += 1;

    int length;
    if (aText.containsCharsAtWhere(*ioIndex, isIdentifierInnerChar, true, &length))
        *ioIndex += length;

    return aText.substringBetween(startIndex, *ioIndex);
}


Symbol readSymbol(String aText, int * ioIndex)
{
    // format: identifier = value

    String name = readIdentifier(aText, ioIndex);

    skipBlank(aText, ioIndex);

    if (!skipChar("=", true, aText, ioIndex))
        throw String("Unexpected end of the definition (expected \"=\" after identifier).");

    skipBlank(aText, ioIndex);

    VerilogNumber value = readNumber(aText, ioIndex);

    return Symbol(name, value);
}


vector<Symbol> readSymbols(String aTableName, String aText, int * ioIndex)
{
    // format: symbol [,symbol] ;

    int symbolStartIndex;

    try {
        vector<Symbol> symbols;

        do {
            skipBlank(aText, ioIndex);

            symbolStartIndex = *ioIndex;
            Symbol symbol = readSymbol(aText, ioIndex);
            symbols.push_back(symbol);

            skipBlank(aText, ioIndex);
        } 
        while (skipChar(",", true, aText, ioIndex));

        if (!skipChar(";", true, aText, ioIndex))
            throw String("Unexpected end of the definition (expected \";\" after last value).");
        
        return symbols;
    }
    catch (String subError)
    {
        int lengthToEol;
        aText.containsCharsAt(symbolStartIndex, notContainedIn, newline, &lengthToEol);
        throw String::formatted(
            "Can't parse definition of \"%s\".\n"
            "Can't analyze source text \"%s\".\n"
            "%s", 
            aTableName.rb(), aText.substringFrom(symbolStartIndex, lengthToEol).rb(), subError.rb());
    }
}



// Building Symbol Table //////////////////////////////////////////////////////////////////////////////////////////////

String buildTableText(vector<Symbol> aSymbols, int aBitWidth, String aVerilogFileName, String aTableName, String aRemovingPrefix)
{
    VerilogNumber sizeMask = bitWidthMask(aBitWidth);

    int hexDigitCount = aBitWidth / 4;
    if (aBitWidth % 4) hexDigitCount += 1;

    String text;
    text.reserveCapacity(10000);

    bool wasWarning = false;

    for (auto symbol : aSymbols)
    {
        auto truncatedValue = symbol.value & sizeMask;
        String truncatedHexValue = verilogNumberToHexString(truncatedValue, hexDigitCount);

        if (truncatedValue != symbol.value) 
        {
            String hexValue = verilogNumberToHexString(symbol.value, 0);
            consoleWrite(1, "SymbolEx Warning: Value of symbol %s.%s.%s was truncated to %d bits from value %s to %s.", 
                aVerilogFileName.rb(), aTableName.rb(), symbol.name.rb(), aBitWidth, hexValue.rb(), truncatedHexValue.rb());
            wasWarning = true;
        }

        String unprefixedName = symbol.name;
        unprefixedName.removePrefix(aRemovingPrefix);
    
        if (unprefixedName == "")
        {
            consoleWrite(1, "SymbolEx Warning: Removing prefix \"%s\" shorted the name of the symbol %s.%s.%s to empty text.", 
                aRemovingPrefix.rb(), aVerilogFileName.rb(), aTableName.rb(), symbol.name.rb());
            wasWarning = true;
        }
        else
            text += truncatedHexValue + ' ' + unprefixedName + '\n';
    }

    if (wasWarning)
        consoleWrite(2, "");

    consoleWrite(5, "%s", text.rb());

    return text;
}



// Extracting Symbols /////////////////////////////////////////////////////////////////////////////////////////////////

void cleanOutputDirectory(String aVerilogFilePath, String anOutputFolderPath)
{
    for (auto entry : filesystem::directory_iterator(anOutputFolderPath.rb()))
        if (entry.is_regular_file())
        {
            String fileName = entry.path().filename().string().c_str();

            if (isTableFileName(fileName, aVerilogFilePath))
                try { 
                    consoleWrite(4, "Deleting: %s", fileName.rb());
                    filesystem::remove(entry.path()); 
                } 
                catch (exception error) {
                    throw String::formatted(
                        "Can't delete file \"%s\".\n%s", 
                        entry.path().string().c_str(), error.what());
                }
        }
}


void checkMultipleDefinition(String aTableName, unordered_set<string> * ioDefinedTables)
{
    auto item = ioDefinedTables->find(aTableName.rb());

    if (item == ioDefinedTables->end())
        ioDefinedTables->insert(aTableName.rb());
    else
        throw String::formatted(
            "Multiple definition of \"%s\".", 
            aTableName.rb());
}


void extractSymbolsFromFile(String aVerilogFilePath, String anOutputFolderPath)
{
    consoleWrite(3, "");
    consoleWrite(2, "Analyzing: %s", aVerilogFilePath.rb());

    cleanOutputDirectory(aVerilogFilePath, anOutputFolderPath);

    try {
        String verilogFileName = extractFileNameWithoutExtension(aVerilogFilePath);
        String verilogFileText = readStringFromFile(aVerilogFilePath);

        int index = 0;
        unordered_set<string> definedTables;

        while (moveToNextLocalParam(verilogFileText, &index))
        {
            String tableName; int bitWidth; String removingPrefix; 
            if (readHeader(&tableName, &bitWidth, &removingPrefix, verilogFileText, &index))
            {
                checkMultipleDefinition(tableName, &definedTables);

                consoleWrite(5, "");
                consoleWrite(3, "Extracting: %s:%d%s", tableName.rb(), bitWidth, 
                    (removingPrefix.isEmpty() ? "" : "," + removingPrefix).rb());

                auto symbols = readSymbols(tableName, verilogFileText, &index);
                auto tableText = buildTableText(symbols, bitWidth, verilogFileName, tableName, removingPrefix);

                auto tableFilePath = buildTableFilePath(anOutputFolderPath, aVerilogFilePath, tableName);
                writeStringToFile(tableFilePath, tableText);
            }
        }
    }
    catch (String subError) {
        throw String::formatted(
            "Problem when processing file \"%s\".\n%s",
            aVerilogFilePath.rb(), subError.rb());
    }
}


void extractSymbolsFromDirectory(String aDirectoryPath, String anOutputDirectoryPath)
{
    for (auto entry : filesystem::directory_iterator(aDirectoryPath.rb()))
    {
        string extension = entry.path().extension().string();
        transform(extension.begin(), extension.end(), extension.begin(), std::tolower);

        if (extension == ".v" || extension == ".sv")
            extractSymbolsFromFile(entry.path().string().c_str(), anOutputDirectoryPath);
    }
}



// Parsing Command Line ///////////////////////////////////////////////////////////////////////////////////////////////

String syntaxDescription()
{
    return String::formatted(
        "Syntax: symbolex [--verbosity 0-%d] verilog_file_or_folder [output_folder]",
        maxVerbosityLevel);
}


struct ArgumentsCursor
{
    public:
        ArgumentsCursor(int aCount, char ** anArguments)
        {
            fCount = aCount;
            fArguments = anArguments;
            fIndex = 0;
        }

        bool getArgument(String * oArgument)
        {
            bool result = fIndex < fCount;
            if (result)
                *oArgument = fArguments[fIndex];
            return result;
        }

        void moveToNextArgument()
        {
            fIndex += 1;
        }

    private:
        int fCount;
        char * * fArguments;
        int fIndex;
};


bool readVerbosityLevel(int * oLevel, ArgumentsCursor * ioCursor)
{
    String argument;
    if (!ioCursor->getArgument(&argument))
        return false;

    argument.convertTo(lowerCase);
    if (argument != "--verbosity")
        return false;

    ioCursor->moveToNextArgument();

    String valueText;
    if (!ioCursor->getArgument(&valueText))
        throw String("Verbosity level missing.");

    int level;
    if (!tryStringToInt(valueText, &level, 10) || level < 0 || level > maxVerbosityLevel)
        throw String::formatted("Verbosity level \"%s\" is invalid.", valueText.rb());

    *oLevel = level;
    ioCursor->moveToNextArgument();

    return true;
}


bool readFileSystemPath(String * oFileSystemPath, ArgumentsCursor * ioCursor)
{
    if (!oFileSystemPath->isEmpty())
        return false;

    if (!ioCursor->getArgument(oFileSystemPath))
        return false;

    ioCursor->moveToNextArgument();

    return true;
}


bool readCommandLineArguments(int aCount, char ** anArguments, 
    String * oSourcePath, 
    String * oOutputDirectoryPath,
    int * oVerbosityLevel)
{
    if (aCount < 2)
        return false;

    try {
        *oSourcePath = "";
        *oOutputDirectoryPath = "";
        *oVerbosityLevel = 1;

        ArgumentsCursor cursor(aCount, anArguments);
        cursor.moveToNextArgument();  // skip first argument (path to program file)

        while (
            readVerbosityLevel(oVerbosityLevel, &cursor) ||
            readFileSystemPath(oSourcePath, &cursor) ||
            readFileSystemPath(oOutputDirectoryPath, &cursor)
        );

        String unknownArgument;
        if (cursor.getArgument(&unknownArgument))
            throw String::formatted("Unknown argument \"%s\".", unknownArgument.rb());

        if (oSourcePath->isEmpty())
            throw String("Missing path to source verilog file or folder.");

        return true;
    }
    catch (String subError) {
        throw String::formatted(
            "Problem when reading command line arguments.\n%s\n\n%s",
            subError.rb(), syntaxDescription().rb());
    }
}



// Main ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void printProgramDescription()
{
    consoleWrite(0,
        "Symbol Extractor 1.0\n"
        "Tool for extracting symbols from verilog source files.\n"
        "Copyright (c) 2020 Stanislav Jurny (github.com/STjurny)\n"
        "\n"
        "%s",
        syntaxDescription().rb()
    );
}


void printError(String anError)
{
    consoleWrite(0, "SymbolEx Error:\n%s", anError.rb());
}


int main(int argc, char * argv[])
{
    try {
        String sourcePath;
        String outputDirectoryPath;

        if (!readCommandLineArguments(argc, argv,
            &sourcePath,
            &outputDirectoryPath,
            &verbosityLevel)) 
        {
            printProgramDescription();
            return 0;
        }

        if (!filesystem::exists(sourcePath.rb()))
            throw String::formatted("Verilog source file or folder \"%s\" not found.", sourcePath.rb());

        if (!outputDirectoryPath.isEmpty())
            createDirectoryPath(outputDirectoryPath);
        else
            outputDirectoryPath = filesystem::current_path().string().c_str();
            
        if (filesystem::is_directory(sourcePath.rb()))
            extractSymbolsFromDirectory(sourcePath, outputDirectoryPath);
        else
            extractSymbolsFromFile(sourcePath, outputDirectoryPath);

        return 0;
    }
    catch (String message) {
        printError(message);
        return 1;
    }
    catch (exception error) {
        printError(error.what());
        return 1;
    }
    catch (...) {
        printError("Unknown error.");
        return 1;
    }
}

