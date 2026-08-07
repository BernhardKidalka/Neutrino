//------------------------------------------------------------------------------------------------------
// Copyright (c) 2026 Bernhard Kidalka
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//------------------------------------------------------------------------------------------------------
//
// Project: Neutrino Engine
//    File: Neutrino\engine\core\logger.cpp
//  Author: B. Kidalka
//    Date: 2026-07-26
//
//    Lang: C++
//
// Descrip: Neutrino Logger class implementation.
//
//------------------------------------------------------------------------------------------------------

#include "logger.h"

#include <iostream>

namespace Neutrino
{

    void Logger::Init() 
    {
        std::cout << "[Neutrino Logger] initialization\n";
    }

    void Logger::Info(const std::string& msg) 
    {
        std::cout << "[INFO] " << msg << "\n";
    }
    
    void Logger::Warning(const std::string& msg) 
    {
        std::cerr << "[WARNING] " << msg << "\n";
    }

    void Logger::Error(const std::string& msg) 
    {
        std::cerr << "[ERROR] " << msg << "\n";
    }

} // namespace Neutrino
