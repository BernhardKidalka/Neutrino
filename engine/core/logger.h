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
//    File: Neutrino\engine\core\logger.h
//  Author: B. Kidalka
//    Date: 2026-08-09
//
//    Lang: C++
//
// Descrip: Neutrino Logger class declaration.
//
//------------------------------------------------------------------------------------------------------
#pragma once

#include <string>

namespace Neutrino
{

    class Logger 
    {
    public:
        static void Init();
        static void Info(const std::string& msg);
        static void Warning(const std::string& msg);
        static void Error(const std::string& msg);
        static void Flush();
    };

} // namespace Neutrino
