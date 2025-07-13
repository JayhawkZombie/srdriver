#pragma once
#include "FileParser.h"

// Example usage for your dad - just like iostream!

/*
// Reading from a file (data.txt contains: "42 3.14 Hello 123")
FileParser input("data.txt", FileParser::Mode::READ);
int x;
float y;
String message;
int z;

input >> x >> y >> message >> z;  // Just like iostream!
// x = 42, y = 3.14, message = "Hello", z = 123

// Writing to a file
FileParser output("output.txt", FileParser::Mode::WRITE);
output << "The answer is: " << x << " and " << y << "\n";

// Appending to a file
FileParser append("log.txt", FileParser::Mode::APPEND);
append << "New entry: " << millis() << "\n";

// Check if file is good
if (input.good()) {
    // File opened successfully
}

// Close when done
input.close();
output.close();
append.close();
*/ 