/**
 *  \file       Log.h
 *  \brief      Fynix main window
 *
 *  \version    1.0
 *  \date       Jan 29, 2026
 *  \author     Xavier Descarrega - DEVE embedded designs <info@deve.tech>
 *
 *  \copyright  MIT License
 *
 *              Copyright (c) 2016 Xavier Descarrega
 *
 *              Permission is hereby granted, free of charge, to any person obtaining a copy
 *              of this software and associated documentation files (the "Software"), to deal
 *              in the Software without restriction, including without limitation the rights
 *              to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *              copies of the Software, and to permit persons to whom the Software is
 *              furnished to do so, subject to the following conditions:
 *
 *              The above copyright notice and this permission notice shall be included in all
 *              copies or substantial portions of the Software.
 *
 *              THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *              IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *              FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *              AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES, OR OTHER
 *              LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *              OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *              SOFTWARE.
 *
 */
 
#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <ctime>

// Inline function to prevent multiple definition errors
inline void LOG(std::string msg) {
    // Print to console
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);

    std::cout << std::put_time(std::localtime(&now_time), "[%Y-%m-%d %H:%M:%S -") << msg << std::endl;

    // Append to exec.log
    std::ofstream errorLog("exec.log", std::ios::app);
    if (errorLog.is_open()) {
        errorLog << std::put_time(std::localtime(&now_time), "[%Y-%m-%d %H:%M:%S -") << msg << std::endl;
        errorLog.close();
    } else {
        std::cerr << "Failed to open error.log" << std::endl;
    }
}

// Macro for simplified logging
//#define LOG(msg) logMessage

#endif // LOG_H
